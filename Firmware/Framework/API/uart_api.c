/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "uart_api.h"

#if defined(ENABLE_UART)
#include "debug_api.h"
#include "heap_api.h"
#include "uart_driver.h"
#include "gpio_driver.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos_typedefs.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

typedef enum eState {
    eState_First,
    eState_Uninitialized = eState_First,
    eState_Setup,
    eState_Collect,
    eState_Flush,
    eState_Last
} eState_t;

typedef struct sUartApiDynamic {
    eState_t current_state;
    SemaphoreHandle_t mutex;
    StaticSemaphore_t mutex_buffer;
    QueueHandle_t message_queue;
    sMessage_t message_queue_storage[UART_API_MESSAGE_QUEUE_CAPACITY];
    StaticQueue_t message_queue_buffer;
    sMessage_t message;
    char *delimiter;
    size_t delimiter_length;
} sUartDynamic_t;

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

#if defined(DEBUG_UART_API)
CREATE_MODULE_NAME (UART_API)
#else
CREATE_MODULE_NAME_EMPTY
#endif /* DEBUG_UART_API */

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

static TaskHandle_t g_fsm_thread = NULL;
static StackType_t g_fsm_thread_stack[STACK_SIZE_IN_BYTES(UART_API_THREAD_STACK_SIZE)] = {0};
static StaticTask_t g_fsm_thread_buffer = {0};

static sUartApiConst_t g_static_uart_lut[eUart_Last] = {0};
static sUartDynamic_t g_dynamic_uart_lut[eUart_Last] = {0};

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

static void UART_API_GetDataReceived (const eUart_t uart);
static void UART_API_FsmThread (void *arg);
static bool UART_API_IsDelimiterReceived (const eUart_t uart);
static void UART_API_BufferIncrement (const eUart_t uart);

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

static void UART_API_GetDataReceived (const eUart_t uart) {
    if (NULL == g_fsm_thread) {
        return;
    }
    
    if (!UART_Config_IsCorrectUart(uart)) {
        return;
    }

    #if defined(MCU_ESP32_S3)
    xTaskNotifyGiveIndexed(g_fsm_thread, 0);
    #endif /* MCU_ESP32_S3 */
    #if defined(MCU_STM32FXX)
    vTaskNotifyGiveIndexedFromISR(g_fsm_thread, 0, NULL);
    #endif /* MCU_STM32FXX */

    return;
}

// TODO: Separate uart polling and parsing into different files 
static void UART_API_FsmThread (void *arg) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        for (eUart_t uart = eUart_First; uart < eUart_Last; uart++) {
            if (eState_Uninitialized == g_dynamic_uart_lut[uart].current_state) {
                continue;
            }

            switch (g_dynamic_uart_lut[uart].current_state) {
                case eState_Setup: {
                    g_dynamic_uart_lut[uart].message.data = Heap_API_Calloc(g_static_uart_lut[uart].buffer_capacity, sizeof(char));
                    
                    if (NULL == g_dynamic_uart_lut[uart].message.data) {
                        TRACE_WRN("FsmThread: Failed to allocate buffer for UART [%d]\n", uart);
                        
                        continue;
                    }
                    
                    g_dynamic_uart_lut[uart].message.size = 0;
                    g_dynamic_uart_lut[uart].current_state = eState_Collect;
                    /* fall through */
                }
                case eState_Collect: {
                    uint8_t received_byte = 0;

                    while (UART_Driver_ReceiveByte(uart, &received_byte)) {
                        g_dynamic_uart_lut[uart].message.data[g_dynamic_uart_lut[uart].message.size] = received_byte;

                        UART_API_BufferIncrement(uart);

                        if (!UART_API_IsDelimiterReceived(uart)) {
                            continue;
                        }

                        g_dynamic_uart_lut[uart].message.size -= g_dynamic_uart_lut[uart].delimiter_length;
                        g_dynamic_uart_lut[uart].message.data[g_dynamic_uart_lut[uart].message.size] = '\0';

                        g_dynamic_uart_lut[uart].current_state = eState_Flush;

                        break;
                    }

                    if (eState_Flush != g_dynamic_uart_lut[uart].current_state) {
                        continue;
                    }
                    /* fall through */
                }
                case eState_Flush: {
                    if (pdTRUE != xQueueSend(g_dynamic_uart_lut[uart].message_queue, &g_dynamic_uart_lut[uart].message, UART_API_MESSAGE_QUEUE_PUT_TIMEOUT)) {
                        TRACE_ERR("FsmThread: Failed to put message in queue for UART [%d]\n", uart);
                        
                        continue;
                    }

                    g_dynamic_uart_lut[uart].current_state = eState_Setup;
                } break;
                default: {  
                } break;
            }
        }

        taskYIELD();
    }
}

static void UART_API_BufferIncrement (const eUart_t uart) {
    g_dynamic_uart_lut[uart].message.size++;

    if (g_dynamic_uart_lut[uart].message.size >= g_static_uart_lut[uart].buffer_capacity) {
        g_dynamic_uart_lut[uart].message.size = 0;
    }

    return;
}

static bool UART_API_IsDelimiterReceived (const eUart_t uart) {
    if (g_dynamic_uart_lut[uart].message.size < g_dynamic_uart_lut[uart].delimiter_length) {
        return false;
    }
    
    if (g_dynamic_uart_lut[uart].delimiter[g_dynamic_uart_lut[uart].delimiter_length - 1] != g_dynamic_uart_lut[uart].message.data[g_dynamic_uart_lut[uart].message.size - 1]) {
        return false;
    } 
    
    size_t start_pos = g_dynamic_uart_lut[uart].message.size - g_dynamic_uart_lut[uart].delimiter_length;

    return (0 == memcmp(&g_dynamic_uart_lut[uart].message.data[start_pos], g_dynamic_uart_lut[uart].delimiter, g_dynamic_uart_lut[uart].delimiter_length));
}

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

bool UART_API_Init (const eUart_t uart, const eBaudrate_t baudrate, const char *delimiter) {
    if (!UART_Config_IsCorrectUart(uart)) {
        return false;
    }

    if (eState_Uninitialized != g_dynamic_uart_lut[uart].current_state) {
        return true;
    }

    if ((baudrate < eBaudrate_First) || (baudrate >= eBaudrate_Last)) {
        return false;
    }

    if (NULL == delimiter) {
        return false;
    }

    if (!GPIO_Driver_InitAllPins()) {
        return false;
    }

    const sUartApiConst_t *api_const = UART_Config_GetUartApiConst(uart);

    if (NULL == api_const) {
        return false;
    }

    g_static_uart_lut[uart] = *api_const;
    
    if (!UART_Driver_Init(uart, baudrate, UART_API_GetDataReceived)) {
        return false;
    }

    g_dynamic_uart_lut[uart].mutex = xSemaphoreCreateRecursiveMutexStatic(&g_dynamic_uart_lut[uart].mutex_buffer);
    
    if (NULL == g_dynamic_uart_lut[uart].mutex) {
        return false;
    }

    g_dynamic_uart_lut[uart].message_queue = xQueueCreateStatic(UART_API_MESSAGE_QUEUE_CAPACITY, sizeof(sMessage_t), (uint8_t*)&g_dynamic_uart_lut[uart].message_queue_storage[0], &g_dynamic_uart_lut[uart].message_queue_buffer);

    if (NULL == g_dynamic_uart_lut[uart].message_queue) {
        return false;
    }

    g_dynamic_uart_lut[uart].delimiter_length = strlen(delimiter);
    g_dynamic_uart_lut[uart].delimiter = Heap_API_Calloc((g_dynamic_uart_lut[uart].delimiter_length + 1), sizeof(char));

    if (NULL == g_dynamic_uart_lut[uart].delimiter) {
        return false;
    }

    memcpy(g_dynamic_uart_lut[uart].delimiter, delimiter, g_dynamic_uart_lut[uart].delimiter_length + 1);

    g_dynamic_uart_lut[uart].current_state = eState_Setup;

    if (NULL == g_fsm_thread) {
        g_fsm_thread = xTaskCreateStatic(UART_API_FsmThread, g_fsm_thread_attributes.name, STACK_SIZE_IN_BYTES(g_fsm_thread_attributes.stack_size), NULL, g_fsm_thread_attributes.priority, g_fsm_thread_stack, &g_fsm_thread_buffer);
    }

    if (NULL == g_fsm_thread) {
        return false;
    }

    return true;
}

// TODO: Revork UART_API_Send to use asynchronous sending with message queues and FSM thread + mutiple messages in buffer
bool UART_API_Send (const eUart_t uart, const sMessage_t message, const uint32_t timeout) {
    if (!UART_Config_IsCorrectUart(uart)) {
        return false;
    }

    if (eState_Uninitialized == g_dynamic_uart_lut[uart].current_state) {
        return false;
    }
    
    if (pdTRUE != xSemaphoreTakeRecursive(g_dynamic_uart_lut[uart].mutex, timeout)) {
        return false;
    }

    if (!UART_Driver_Send(uart, (uint8_t*) message.data, message.size)) {  
        xSemaphoreGiveRecursive(g_dynamic_uart_lut[uart].mutex);
        
        return false;
    }

    xSemaphoreGiveRecursive(g_dynamic_uart_lut[uart].mutex);

    return true;
}

bool UART_API_Receive (const eUart_t uart, sMessage_t *message, const uint32_t timeout) {
    if (!UART_Config_IsCorrectUart(uart)) {
        TRACE_ERR("Receive: Incorrect UART type [%d]\n", uart);
        
        return false;
    }

    if (eState_Uninitialized == g_dynamic_uart_lut[uart].current_state) {
        TRACE_ERR("Receive: UART [%d] not initialized\n", uart);
        
        return false;
    }

    if (NULL == message) {
        TRACE_ERR("Receive: Message pointer is NULL for UART [%d]\n", uart);
        
        return false;
    }

    if (pdTRUE != xQueueReceive(g_dynamic_uart_lut[uart].message_queue, message, timeout)) {
        TRACE_ERR("Receive: Failed to get message from queue for UART [%d]\n", uart);
        
        return false;
    }

    return true;
}

#endif /* ENABLE_UART */
