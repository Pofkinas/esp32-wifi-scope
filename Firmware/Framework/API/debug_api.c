/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "debug_api.h"

#if defined(ENABLE_UART_DEBUG)

#include "uart_api.h"
#include "message.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

static char g_debug_message_buffer[DEBUG_MESSAGE_SIZE] = {0};
static bool g_is_initialized = false;

static SemaphoreHandle_t g_debug_api_mutex = NULL;
static StaticSemaphore_t g_debug_api_mutex_buffer = {0};

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

bool Debug_API_Init(const eBaudrate_t baudrate) {
    if (g_is_initialized) {
        return true;
    }

    if ((baudrate < eBaudrate_First) || (baudrate >= eBaudrate_Last)) {
        return false;
    }

    g_debug_api_mutex = xSemaphoreCreateRecursiveMutexStatic(&g_debug_api_mutex_buffer);

    if (NULL == g_debug_api_mutex) {
        return false;
    }

    g_is_initialized = UART_API_Init(DEBUG_UART, baudrate, DEBUG_DELIMITER);

    return g_is_initialized;
}

bool Debug_API_Print(const eTraceLevel_t trace_level, const char *file_trace, const char *file_name, const size_t line_number, const char *format, ...) {
    if ((trace_level < eTraceLevel_First) || (trace_level >= eTraceLevel_Last)) {
        return false;
    }

    if ((NULL == file_trace) || (NULL == format) || (NULL == file_name)) {
        return false;
    }

    if (pdTRUE != xSemaphoreTakeRecursive(g_debug_api_mutex, DEBUG_MUTEX_TIMEOUT)) {
        return false;
    }

    sMessage_t debug_message = {.data = NULL, .size = 0};
    size_t message_length = 0;
    size_t written = 0;

    va_list arguments;

    debug_message.data = g_debug_message_buffer;

    switch (trace_level) {
        case eTraceLevel_Info: {
            written = snprintf(debug_message.data, DEBUG_MESSAGE_SIZE, "[%s.INF] ", file_trace);
        } break;
        case eTraceLevel_Warning: {
            written = snprintf(debug_message.data, DEBUG_MESSAGE_SIZE, "[%s.WRN] ", file_trace);
        } break;
        case eTraceLevel_Error: {
            written = snprintf(debug_message.data, DEBUG_MESSAGE_SIZE, "[%s.ERR] (file: %s, line: %d) ", file_trace, file_name, line_number);
        } break;
        default: {
            written = 0;
        } break;
    }

    if (written <= 0) {
        xSemaphoreGiveRecursive(g_debug_api_mutex);
        return false;
    }

    message_length = (size_t) written;

    va_start(arguments, format);

    written = vsnprintf((debug_message.data + message_length), (DEBUG_MESSAGE_SIZE - message_length), format, arguments);

    va_end(arguments);

    if (written <= 0) {
        xSemaphoreGiveRecursive(g_debug_api_mutex);
        return false;
    }

    message_length += (size_t) written;

    debug_message.size = message_length;
    bool is_sent = UART_API_Send(DEBUG_UART, debug_message, DEBUG_MESSAGE_TIMEOUT);

    xSemaphoreGiveRecursive(g_debug_api_mutex);

    return is_sent;
}

#endif /* ENABLE_UART_DEBUG */
