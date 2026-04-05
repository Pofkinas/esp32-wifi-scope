/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "oscilloscope_app.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "capture_api.h"
#include "voltage_api.h"
#include "io_api.h"
#include "led_api.h"
#include "uart_api.h"
#include "debug_api.h"
#include "adc_config.h"
#include "ring_buffer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos_types.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

#define MUTEX_TIMEOUT 0U
#define HANDLE_DATA_MUTEX_TIMEOUT 2U
#define FRAME_TIMER_TIMEOUT 0U
#define UART_TX_TIMEOUT 0U

#define CAPTURE_ELEMENTS 256
#define CAPTURE_BUFFER_SIZE 8192

#define CAPTURE_DONE_EVENT 0x00000002U
#define FRAME_DONE_EVENT 0x00000004U
#define DATA_OVERFLOW_EVENT 0x00000008U

#define CAPTURE_DEVICE eCaptureDevice_Adc1ch0
#define EVENT_ALL_BITS ((EventBits_t) 0x00FFFFFFU)
#define START_STOP_EVENT BUTTON_TRIGGERED_EVENT

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

typedef enum eOscilloscopeState {
    eOscilloscopeState_First = 0,
    eOscilloscopeState_Default = eOscilloscopeState_First,
    eOscilloscopeState_Initialized,
    eOscilloscopeState_Paused,
    eOscilloscopeState_Running,
    eOscilloscopeState_Last
} eOscilloscopeState_t;

typedef struct sFrameData {
    size_t frame_number;
    uint32_t timestamp;
    uint32_t total_bytes;
    uint8_t data[FRAME_SIZE * MAX_FRAMES];
} sFrameData_t;

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

static void Oscilloscope_Thread(void *pvParameters);
static void Oscilloscope_HandleStartStopEvent(void);
static void Oscilloscope_HandleCaptureDoneEvent(void);
static void Oscilloscope_SendFrames(sFrameData_t *const frame_data, char *const header);

static void Oscilloscope_CaptureNotifyCallback(const eCaptureDevice_t device, const eCaptureEvent_t event);
static void Oscilloscope_FrameTimerCallback(TimerHandle_t timer);

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

#if defined(DEBUG_OSCILLOSCOPE_APP)
CREATE_MODULE_NAME(OSCILLOSCOPE_APP)
#else
CREATE_MODULE_NAME_EMPTY
#endif /* DEBUG_OSCILLOSCOPE_APP */

/* clang-format off */
static const sTaskDesc_t g_oscilloscope_thread_attributes = {
    .name = "Oscilloscope",
    .stack_size = OSCILLOSCOPE_THREAD_STACK_SIZE,
    .priority = OSCILLOSCOPE_THREAD_PRIORITY
};

static const sAdcConfig_t g_adc1_channel_0_desc = {
    .adc = eAdc_1,
    .channel = eAdcChannel_0
};

static const sCaptureDesc_t g_adc1ch0_capture = {
    .type = eCaptureType_Adc,
    .device_data = &g_adc1_channel_0_desc,
    .data_size = sizeof(uint16_t),
    .process_callback = Voltage_API_Process,
    .process_context = &g_adc1_channel_0_desc,
    .notify_callback = Oscilloscope_CaptureNotifyCallback
};
/* clang-format on */

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

static eOscilloscopeState_t g_oscilloscope_state = eOscilloscopeState_First;

static TaskHandle_t g_oscilloscope_thread = NULL;
static StackType_t g_oscilloscope_thread_stack[STACK_SIZE_IN_BYTES(OSCILLOSCOPE_THREAD_STACK_SIZE)] = {0};
static StaticTask_t g_oscilloscope_thread_buffer = {0};

static EventGroupHandle_t g_event = NULL;
static StaticEventGroup_t g_event_buffer = {0};

static SemaphoreHandle_t g_mutex = NULL;
static StaticSemaphore_t g_mutex_buffer = {0};

static TimerHandle_t g_frame_timer;
static StaticTimer_t g_frame_timer_buffer;

static sRingBuffer_Handle_t g_capture_buffer = NULL;
static sFrameData_t g_frame = {0};

static char g_frame_header[FRAME_HEADER_SIZE] = {0};

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

static void Oscilloscope_Thread(void *pvParameters) {
    EventBits_t event = 0;

    while (1) {
        event = xEventGroupWaitBits(g_event, EVENT_ALL_BITS, pdTRUE, pdFALSE, portMAX_DELAY);

        if (START_STOP_EVENT & event) {
            Oscilloscope_HandleStartStopEvent();

            continue;
        }

        if (DATA_OVERFLOW_EVENT & event) {
            TRACE_WRN("Data overflow event received\n");

            continue;
        }

        if (eOscilloscopeState_Running != g_oscilloscope_state) {
            continue;
        }

        if (CAPTURE_DONE_EVENT & event) {
            Oscilloscope_HandleCaptureDoneEvent();
        }

        if (FRAME_DONE_EVENT & event) {
            Oscilloscope_SendFrames(&g_frame, g_frame_header);
        }
    }
}

static void Oscilloscope_HandleStartStopEvent(void) {
    switch (g_oscilloscope_state) {
        case eOscilloscopeState_Initialized: {
            if (!Oscilloscope_APP_Start()) {
                TRACE_ERR("HandleStartStopEvent: Failed to start oscilloscope app\n");
            }
        } break;
        case eOscilloscopeState_Running: {
            if (!Oscilloscope_APP_Stop()) {
                TRACE_ERR("HandleStartStopEvent: Failed to stop oscilloscope app\n");
            }
        } break;
        case eOscilloscopeState_Paused: {
            if (!Oscilloscope_APP_Start()) {
                TRACE_ERR("HandleStartStopEvent: Failed to start oscilloscope app\n");
            }
        } break;
        default: {
            TRACE_WRN("HandleStartStopEvent: Button press event in unexpected state [%d]\n", g_oscilloscope_state);
        } break;
    }

    TRACE_INFO("Button pressed!\n");
}

static void Oscilloscope_HandleCaptureDoneEvent(void) {
    if (pdTRUE != xSemaphoreTakeRecursive(g_mutex, HANDLE_DATA_MUTEX_TIMEOUT)) {
        TRACE_ERR("HandleCaptureDoneEvent: Failed to acquire mutex\n");

        return;
    }

    // TODO: Currently capturing predefined number of bytes, but it may be better change this at runtime
    if (!Ring_Buffer_PopBulk(g_capture_buffer, &g_frame.data[g_frame.total_bytes % FRAME_SIZE], CAPTURE_ELEMENTS)) {
        TRACE_ERR("HandleCaptureDoneEvent: Failed to pop data from buffer\n");

        xSemaphoreGiveRecursive(g_mutex);

        return;
    }

    g_frame.total_bytes += CAPTURE_ELEMENTS * g_adc1ch0_capture.data_size;

    xSemaphoreGiveRecursive(g_mutex);

    return;
}

static void Oscilloscope_SendFrames(sFrameData_t *const frame_data, char *const header) {
    if (pdTRUE != xSemaphoreTakeRecursive(g_mutex, HANDLE_DATA_MUTEX_TIMEOUT)) {
        TRACE_ERR("SendFrames: Failed to acquire mutex\n");

        return;
    }

    if (frame_data->total_bytes == 0) {
        xSemaphoreGiveRecursive(g_mutex);

        return;
    }

    frame_data->timestamp = xTaskGetTickCount();
    snprintf(header, FRAME_HEADER_SIZE, "%lu%s%u%s%lu%s", frame_data->timestamp, FRAME_HEADER_SEPARATOR, frame_data->frame_number, FRAME_HEADER_SEPARATOR, frame_data->total_bytes, FRAME_HEADER_DELIMITER);

    sMessage_t message = {.data = header, .size = strlen(header)};

    UART_API_Send(UART0, message, UART_TX_TIMEOUT);

    size_t complete_frames = frame_data->total_bytes / FRAME_SIZE;
    size_t partial_bytes = frame_data->total_bytes % FRAME_SIZE;

    message.size = FRAME_SIZE;

    for (size_t frame = 0; frame < complete_frames; frame++) {
        message.data = (char *) &frame_data->data[frame * FRAME_SIZE];

        UART_API_Send(UART0, message, UART_TX_TIMEOUT);
    }

    if (partial_bytes > 0) {
        message.data = (char *) &frame_data->data[complete_frames * FRAME_SIZE];
        message.size = partial_bytes;

        UART_API_Send(UART0, message, UART_TX_TIMEOUT);
    }

    frame_data->frame_number++;
    frame_data->total_bytes = 0;

    xSemaphoreGiveRecursive(g_mutex);

    return;
}

static void Oscilloscope_CaptureNotifyCallback(const eCaptureDevice_t device, const eCaptureEvent_t event) {
    switch (event) {
        case eCaptureEvent_CaptureDone: {
            xEventGroupSetBits(g_event, CAPTURE_DONE_EVENT);

            // TRACE_INFO("Capture done for device [%d]\n", device);
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

static void Oscilloscope_FrameTimerCallback(TimerHandle_t timer) {
    xEventGroupSetBits(g_event, FRAME_DONE_EVENT);
}

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

bool Oscilloscope_APP_Init(void) {
    if (eOscilloscopeState_Default != g_oscilloscope_state) {
        return true;
    }

    if (!LED_API_Init()) {
        TRACE_ERR("Init: LED_API_Init failed\n");

        return false;
    }

    g_event = xEventGroupCreateStatic(&g_event_buffer);

    if (NULL == g_event) {
        TRACE_ERR("Init: Failed to create event group\n");

        return false;
    }

    if (!IO_API_Init(eIo_Button, g_event)) {
        TRACE_ERR("Init: IO_API_Init failed\n");

        return false;
    }

    g_capture_buffer = Ring_Buffer_Init(CAPTURE_BUFFER_SIZE, g_adc1ch0_capture.data_size);

    if (NULL == g_capture_buffer) {
        TRACE_ERR("Init: Failed to initialize capture buffer\n");

        return false;
    }

    if (!Capture_API_Init(CAPTURE_DEVICE, &g_adc1ch0_capture, g_capture_buffer)) {
        TRACE_ERR("Init: Capture_API_Init failed\n");

        return false;
    }

    if (!Voltage_API_Init(g_adc1_channel_0_desc.adc)) {
        TRACE_ERR("Init: Voltage_API_Init failed\n");

        return false;
    }

    g_mutex = xSemaphoreCreateRecursiveMutexStatic(&g_mutex_buffer);

    if (NULL == g_mutex) {
        TRACE_ERR("Init: Failed to create mutex\n");

        return false;
    }

    g_frame_timer = xTimerCreateStatic("FrameTimer", configTICK_RATE_HZ / FRAMES_PER_SECOND, pdTRUE, NULL, Oscilloscope_FrameTimerCallback, &g_frame_timer_buffer);

    if (NULL == g_frame_timer) {
        TRACE_ERR("Init: Failed to create frame timer\n");

        return false;
    }

    g_oscilloscope_thread = xTaskCreateStatic(Oscilloscope_Thread, g_oscilloscope_thread_attributes.name, STACK_SIZE_IN_BYTES(g_oscilloscope_thread_attributes.stack_size), NULL, g_oscilloscope_thread_attributes.priority, g_oscilloscope_thread_stack, &g_oscilloscope_thread_buffer);

    if (NULL == g_oscilloscope_thread) {
        TRACE_ERR("Init: Failed to create thread\n");

        return false;
    }

    if (!IO_API_Start()) {
        TRACE_ERR("Init: IO_API_Start failed\n");

        return false;
    }

    g_oscilloscope_state = eOscilloscopeState_Initialized;

    return true;
}

bool Oscilloscope_APP_Start(void) {
    if ((eOscilloscopeState_Initialized != g_oscilloscope_state) && (eOscilloscopeState_Paused != g_oscilloscope_state)) {
        TRACE_ERR("Start: App not in correct state [%d]\n", g_oscilloscope_state);

        return false;
    }

    if (pdTRUE != xSemaphoreTakeRecursive(g_mutex, MUTEX_TIMEOUT)) {
        TRACE_ERR("Start: Failed to acquire mutex\n");

        return false;
    }

    if (!LED_API_TurnOn(eLed_Led)) {
        TRACE_ERR("Start: Failed to turn on LED\n");

        xSemaphoreGiveRecursive(g_mutex);

        return false;
    }

    if (!Capture_API_Start(CAPTURE_DEVICE, FRAME_SIZE)) {
        TRACE_ERR("Start: Failed to start capture\n");

        xSemaphoreGiveRecursive(g_mutex);

        return false;
    }

    if (!xTimerStart(g_frame_timer, FRAME_TIMER_TIMEOUT)) {
        TRACE_ERR("Start: Failed to start frame timer\n");

        xSemaphoreGiveRecursive(g_mutex);

        return false;
    }

    g_oscilloscope_state = eOscilloscopeState_Running;
    g_frame.frame_number = 0;
    g_frame.total_bytes = 0;
    g_frame.timestamp = 0;

    xSemaphoreGiveRecursive(g_mutex);

    return true;
}

bool Oscilloscope_APP_Pause(void) {
    if (eOscilloscopeState_Running != g_oscilloscope_state) {
        TRACE_ERR("Pause: App not running\n");

        return false;
    }

    if (pdTRUE != xSemaphoreTakeRecursive(g_mutex, MUTEX_TIMEOUT)) {
        TRACE_ERR("Pause: Failed to acquire mutex\n");

        return false;
    }

    if (!xTimerStop(g_frame_timer, FRAME_TIMER_TIMEOUT)) {
        TRACE_ERR("Pause: Failed to stop frame timer\n");

        xSemaphoreGiveRecursive(g_mutex);

        return false;
    }

    if (!Capture_API_Stop(CAPTURE_DEVICE)) {
        TRACE_ERR("Pause: Failed to stop capture\n");

        xSemaphoreGiveRecursive(g_mutex);

        return false;
    }

    g_oscilloscope_state = eOscilloscopeState_Paused;

    xSemaphoreGiveRecursive(g_mutex);

    return true;
}

bool Oscilloscope_APP_Stop(void) {
    if (eOscilloscopeState_Running != g_oscilloscope_state) {
        TRACE_ERR("Stop: App not running\n");

        return false;
    }

    if (pdTRUE != xSemaphoreTakeRecursive(g_mutex, MUTEX_TIMEOUT)) {
        TRACE_ERR("Stop: Failed to acquire mutex\n");

        return false;
    }

    if (!xTimerStop(g_frame_timer, FRAME_TIMER_TIMEOUT)) {
        TRACE_ERR("Stop: Failed to stop frame timer\n");

        xSemaphoreGiveRecursive(g_mutex);

        return false;
    }

    if (!LED_API_TurnOff(eLed_Led)) {
        TRACE_ERR("Stop: Failed to turn off LED\n");

        xSemaphoreGiveRecursive(g_mutex);

        return false;
    }

    if (!Capture_API_Stop(CAPTURE_DEVICE)) {
        TRACE_ERR("Stop: Failed to stop capture\n");

        xSemaphoreGiveRecursive(g_mutex);

        return false;
    }

    if (!Capture_API_ClearBuffer(CAPTURE_DEVICE)) {
        TRACE_ERR("Stop: Failed to clear capture buffer\n");

        xSemaphoreGiveRecursive(g_mutex);

        return false;
    }

    g_oscilloscope_state = eOscilloscopeState_Initialized;

    xSemaphoreGiveRecursive(g_mutex);

    return true;
}
