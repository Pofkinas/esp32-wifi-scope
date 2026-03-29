/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "framework_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "cli_app.h"
#include "led_app.h"
#include "debug_api.h"

#include "freertos_types.h"
#include "message.h"
#include "io_api.h"
#include "uart_api.h"
#include "capture_api.h"
#include "voltage_api.h"
#include "adc_config.h"
#include "ring_buffer.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

#define STARTSTOP_BUTTON_WAIT_TIMEOUT 50U

#define MUTEX_TIMEOUT 0U
#define UART_TX_TIMEOUT 0U

#define CAPTURE_BUFFER_SIZE 4096
#define DATA_TO_CAPTURE 2048
#define CAPTURE_DONE_EVENT 0x02U

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

static EventGroupHandle_t g_main_event = NULL;
static StaticEventGroup_t g_main_event_buffer = {0};
static bool g_is_started = false;

static TaskHandle_t g_test_thread = NULL;
static StackType_t g_test_thread_stack[STACK_SIZE_IN_BYTES(MAIN_TEST_THREAD_STACK_SIZE)] = {0};
static StaticTask_t g_test_thread_buffer = {0};

static SemaphoreHandle_t g_mutex = NULL;
static StaticSemaphore_t g_mutex_buffer = {0};

static sRingBuffer_Handle_t g_capture_buffer = NULL;
static uint32_t g_captured_data[DATA_TO_CAPTURE] = {0};
static char g_tx_buffer[DATA_TO_CAPTURE * 4] = {0};

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

static void Main_Init(void);
static void Main_TestThread(void *pvParameters);
static void Main_CaptureNotifyCallback(const eCaptureDevice_t device, const eCaptureEvent_t event);

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

#if defined(DEBUG_MAIN)
CREATE_MODULE_NAME(MAIN)
#else
CREATE_MODULE_NAME_EMPTY
#endif /* DEBUG_MAIN */

/* clang-format off */
static const sTaskDesc_t g_test_thread_attributes = {
    .name = "Test",
    .stack_size = MAIN_TEST_THREAD_STACK_SIZE,
    .priority = MAIN_TEST_THREAD_PRIORITY
};

static sAdcConfig_t g_adc1_channel_0_desc = {
    .adc = eAdc_1,
    .channel = eAdcChannel_0
};

static const sCaptureDesc_t g_capture_desc[eCaptureDevice_Last] = {
    [eCaptureDevice_Adc1ch0] = {
        .type = eCaptureType_Adc,
        .device_data = &g_adc1_channel_0_desc,
        .data_size = sizeof(uint32_t),
        .process_callback = Voltage_API_Process,
        .process_context = &g_adc1_channel_0_desc,
        .notify_callback = Main_CaptureNotifyCallback
    }
};
/* clang-format on */

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

static void Main_Init(void) {
    bool init_success = true;

    if (!CLI_APP_Init(UART_0_BAUDRATE)) {
        init_success = false;
    }

    // Test LED
    if (!LED_APP_Init()) {
        TRACE_ERR("LED_APP_Init failed\n");

        init_success = false;
    }

    g_main_event = xEventGroupCreateStatic(&g_main_event_buffer);
    // Test IO
    if (!IO_API_Init(eIo_Button, g_main_event)) {
        TRACE_ERR("IO_API_Init failed\n");

        init_success = false;
    }

    if (!IO_API_Start()) {
        TRACE_ERR("IO_API_Start failed\n");

        init_success = false;
    }

    // Test Capture
    g_capture_buffer = Ring_Buffer_Init(CAPTURE_BUFFER_SIZE, g_capture_desc[eCaptureDevice_Adc1ch0].data_size);

    if (NULL == g_capture_buffer) {
        TRACE_ERR("Failed to initialize capture buffer\n");

        init_success = false;
    }

    if (!Capture_API_Init(eCaptureDevice_Adc1ch0, &g_capture_desc[eCaptureDevice_Adc1ch0], g_capture_buffer)) {
        TRACE_ERR("Capture_API_Init failed\n");

        init_success = false;
    }

    if (!Voltage_API_Init(eAdc_1)) {
        TRACE_ERR("Voltage_API_Init failed\n");

        init_success = false;
    }

    g_mutex = xSemaphoreCreateRecursiveMutexStatic(&g_mutex_buffer);

    if (NULL == g_mutex) {
        TRACE_ERR("Failed to create mutex\n");

        init_success = false;
    }

    if (!init_success) {
        while (1) {}
    }

    g_test_thread = xTaskCreateStatic(Main_TestThread, g_test_thread_attributes.name, STACK_SIZE_IN_BYTES(g_test_thread_attributes.stack_size), NULL, g_test_thread_attributes.priority, g_test_thread_stack, &g_test_thread_buffer);

    if (NULL == g_test_thread) {
        TRACE_ERR("Failed to create test thread\n");

        while (1) {}
    }

    TRACE_INFO("Start OK\n");

    return;
}

static void Main_TestThread(void *pvParameters) {
    EventBits_t event = 0;

    while (1) {
        event = xEventGroupWaitBits(g_main_event, BUTTON_TRIGGERED_EVENT | CAPTURE_DONE_EVENT, pdTRUE, pdFALSE, portMAX_DELAY);

        if (BUTTON_TRIGGERED_EVENT & event) {
            g_is_started = !g_is_started;

            TRACE_INFO("Button pressed!\n");

            if (g_is_started) {
                if (!LED_API_TurnOn(eLed_Led)) {
                    TRACE_ERR("Failed to turn on LED\n");
                }

                if (!Capture_API_Start(eCaptureDevice_Adc1ch0, DATA_TO_CAPTURE)) {
                    TRACE_ERR("Failed to start capture\n");
                }
            } else {
                if (!LED_API_TurnOff(eLed_Led)) {
                    TRACE_ERR("Failed to turn off LED\n");
                }

                if (!Capture_API_Stop(eCaptureDevice_Adc1ch0)) {
                    TRACE_ERR("Failed to stop capture\n");
                }

                if (!Capture_API_ClearBuffer(eCaptureDevice_Adc1ch0)) {
                    TRACE_ERR("Failed to clear capture buffer\n");
                }
            }
        }

        if (g_is_started && (event & CAPTURE_DONE_EVENT)) {
            if (pdTRUE != xSemaphoreTakeRecursive(g_mutex, MUTEX_TIMEOUT)) {
                TRACE_ERR("TestThread: Failed to acquire capture mutex\n");

                continue;
            }

            if (!Ring_Buffer_PopBulk(g_capture_buffer, &g_captured_data, DATA_TO_CAPTURE)) {
                TRACE_ERR("TestThread: Failed to pop data from capture buffer\n");

                xSemaphoreGiveRecursive(g_mutex);

                continue;
            }

            xSemaphoreGiveRecursive(g_mutex);

            size_t offset = 0;
            for (size_t i = 0; i < DATA_TO_CAPTURE; i++) {
                offset += snprintf(g_tx_buffer + offset, sizeof(g_tx_buffer) - offset, "%lu\n", g_captured_data[i]);
            }

            sMessage_t message = {.data = g_tx_buffer, .size = offset};

            if (pdTRUE != xSemaphoreTakeRecursive(g_mutex, MUTEX_TIMEOUT)) {
                TRACE_ERR("TestThread: Failed to acquire capture mutex\n");

                continue;
            }

            UART_API_Send(UART0, message, UART_TX_TIMEOUT);

            xSemaphoreGiveRecursive(g_mutex);
        }
    }
}

static void Main_CaptureNotifyCallback(const eCaptureDevice_t device, const eCaptureEvent_t event) {
    switch (event) {
        case eCaptureEvent_CaptureDone: {
            xEventGroupSetBits(g_main_event, CAPTURE_DONE_EVENT);
            TRACE_INFO("Capture done for device [%d]\n", device);
        } break;
        case eCaptureEvent_Overflow: {
            TRACE_WRN("Capture overflow for device [%d]\n", device);
        } break;
        default: {
            TRACE_WRN("Unknown capture event [%d] for device [%d]\n", event, device);
        } break;
    }

    return;
}

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

int app_main(void) {
    Main_Init();

    vTaskDelete(NULL);

    return 0;
}
