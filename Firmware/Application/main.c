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

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

#define STARTSTOP_BUTTON_WAIT_TIMEOUT 50U

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
/* clang-format on */

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

static EventGroupHandle_t g_start_button_event = NULL;
static StaticEventGroup_t g_start_button_event_buffer = {0};

static TaskHandle_t g_test_thread = NULL;
static StackType_t g_test_thread_stack[STACK_SIZE_IN_BYTES(MAIN_TEST_THREAD_STACK_SIZE)] = {0};
static StaticTask_t g_test_thread_buffer = {0};

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

static void Main_Init(void);
static void Main_TestThread(void *pvParameters);

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

    // Test IO
    g_start_button_event = xEventGroupCreateStatic(&g_start_button_event_buffer);
    if (!IO_API_Init(eIo_Button, g_start_button_event)) {
        TRACE_ERR("IO_API_Init failed\n");

        init_success = false;
    }

    if (!IO_API_Start()) {
        TRACE_ERR("IO_API_Start failed\n");

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
    while (1) {
        if (BUTTON_TRIGGERED_EVENT == xEventGroupWaitBits(g_start_button_event, BUTTON_TRIGGERED_EVENT, pdTRUE, pdFALSE, pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS))) {
            TRACE_INFO("Button pressed!\n");

            if (!LED_API_Toggle(eLed_Led)) {
                TRACE_ERR("Failed to toggle LED\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

int app_main(void) {
    Main_Init();

    vTaskDelete(NULL);

    return 0;
}
