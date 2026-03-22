/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "cli_app.h"

#if defined(ENABLE_CLI)

#include <ctype.h>
#include "default_cli_lut.h"
#include "cmd_api.h"
#include "uart_api.h"
#include "heap_api.h"
#include "debug_api.h"
#include "message.h"
#include "error_messages.h"

#include "freertos/FreeRTOS.h"
#include "freertos_typedefs.h"

#if defined(ENABLE_CUSTOM_CMD)
#include "custom_cli_lut.h"
#endif

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

#if defined(DEBUG_CLI_APP)
CREATE_MODULE_NAME(CLI_APP)
#else
CREATE_MODULE_NAME_EMPTY
#endif /* DEBUG_CLI_APP */

/* clang-format off */
static const sTaskDesc_t g_cli_thread_attributes = {
    .name = "CLI_APP",
    .stack_size = CLI_APP_THREAD_STACK_SIZE,
    .priority = eTaskPriority_Normal
};
/* clang-format on */

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

static bool g_is_initialized = false;

static TaskHandle_t g_cli_thread = NULL;
static StackType_t g_cli_app_thread_stack[STACK_SIZE_IN_BYTES(CLI_APP_THREAD_STACK_SIZE)] = {0};
static StaticTask_t g_cli_app_thread_buffer = {0};

static char g_response_buffer[RESPONSE_MESSAGE_CAPACITY];

static sMessage_t g_command = {.data = NULL, .size = 0};
static sMessage_t g_response = {.data = g_response_buffer, .size = RESPONSE_MESSAGE_CAPACITY};

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

static void CLI_APP_Thread(void *pvParameters);

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

static void CLI_APP_Thread(void *pvParameters) {
    while (1) {
        if (UART_API_Receive(DEBUG_UART, &g_command, portMAX_DELAY)) {
            eErrorCode_t error_code = eErrorCode_NOTFOUND;

#if defined(ENABLE_DEFAULT_CMD)
            error_code = CMD_API_FindCommand(g_command, &g_response, g_default_cmd_lut, eCliDefaultCmd_Last);
#endif /* ENABLE_DEFAULT_CMD */

#if defined(ENABLE_CUSTOM_CMD)
            if (eErrorCode_NOTFOUND == error_code) {
                error_code = CMD_API_FindCommand(g_command, &g_response, g_custom_cmd_lut, eCliCustomCmd_Last);
            }
#endif /* ENABLE_CUSTOM_CMD */

            if ((eErrorCode_OK != error_code) && (NULL != g_response.data)) {
                TRACE_WRN("%s", g_response.data);
            }

            Heap_API_Free(g_command.data);
        }
    }
}

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

bool CLI_APP_Init(const eBaudrate_t baudrate) {
    if (g_is_initialized) {
        return true;
    }

    if ((baudrate < eBaudrate_First) || (baudrate >= eBaudrate_Last)) {
        return false;
    }

    if (!Heap_API_Init()) {
        return false;
    }

    if (!Debug_API_Init(baudrate)) {
        return false;
    }

    g_cli_thread = xTaskCreateStatic(CLI_APP_Thread, g_cli_thread_attributes.name, STACK_SIZE_IN_BYTES(g_cli_thread_attributes.stack_size), NULL, g_cli_thread_attributes.priority, g_cli_app_thread_stack, &g_cli_app_thread_buffer);

    if (NULL == g_cli_thread) {
        return false;
    }

    g_is_initialized = true;

    return g_is_initialized;
}

#endif /* ENABLE_CLI */
