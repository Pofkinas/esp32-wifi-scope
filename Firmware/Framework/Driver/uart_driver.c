/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "uart_driver.h"

#if defined(ENABLE_UART)
#include <stdio.h>
#include <stdint.h>
#include "ring_buffer.h"

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos_typedefs.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

#define UART_MUTEX_TIMEOUT 0U

#define UART_FEEDER_THREAD_NAME "UART%d_Feeder"
#define UART_FEEDER_THREAD_STACK_SIZE (256 * 8)
#define UART_FEEDER_BUFFER_SIZE 32

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

typedef enum eUartState {
    eUartState_First = 0,
    eUartState_Default = eUartState_First,
    eUartState_Initialized,
    eUartState_Last
} eUartState_t;

typedef struct sUartDriver {
    eUart_t uart;
    eUartState_t state;
    RingBuffer_Handle ring_buffer;
    TaskHandle_t feeder_thread;
    StackType_t thread_stack[STACK_SIZE_IN_BYTES(UART_FEEDER_THREAD_STACK_SIZE)];
    StaticTask_t thread_buffer;
    SemaphoreHandle_t mutex;
    StaticSemaphore_t mutex_buffer;
    data_received_callback_t data_received_callback;
} sUartDriver_t;

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

static sUartDesc_t g_uart_lut[eUart_Last] = {0};
static sUartDriver_t g_uart_dynamic_lut[eUart_Last] = {0};

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

static bool UART_Driver_CreateFeederThread(const eUart_t uart, sUartDriver_t *driver);
static void UART_Driver_Feeder(void *pvParameters);

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

static bool UART_Driver_CreateFeederThread(const eUart_t uart, sUartDriver_t *driver) {
    if (!UART_Config_IsCorrectUart(uart)) {
        return false;
    }

    if (NULL == driver) {
        return false;
    }

    sTaskDesc_t thread_attributes = {0};
    char feeder_thread_name[16] = {0};

    snprintf(feeder_thread_name, sizeof(feeder_thread_name), UART_FEEDER_THREAD_NAME, uart);
    thread_attributes.name = feeder_thread_name;
    thread_attributes.stack_size = UART_FEEDER_THREAD_STACK_SIZE;
    thread_attributes.priority = eTaskPriority_Normal;

    driver->feeder_thread = xTaskCreateStatic(UART_Driver_Feeder, thread_attributes.name, STACK_SIZE_IN_BYTES(thread_attributes.stack_size), &driver->uart, thread_attributes.priority, driver->thread_stack, &driver->thread_buffer);

    if (NULL == driver->feeder_thread) {
        return false;
    }

    return true;
}

static void UART_Driver_Feeder(void *pvParameters) {
    if (NULL == pvParameters) {
        return;
    }

    const eUart_t uart = *((const eUart_t *) pvParameters);

    if (!UART_Config_IsCorrectUart(uart)) {
        return;
    }

    uint8_t rx_data = {0};
    int length = 0;

    while (1) {
        length = uart_read_bytes(g_uart_lut[uart].uart_num, &rx_data, sizeof(rx_data), portMAX_DELAY);

        if (length <= 0) {
            continue;
        }

        if (pdTRUE != xSemaphoreTakeRecursive(g_uart_dynamic_lut[uart].mutex, UART_MUTEX_TIMEOUT)) {
            return;
        }

        Ring_Buffer_Push(g_uart_dynamic_lut[uart].ring_buffer, rx_data);

        xSemaphoreGiveRecursive(g_uart_dynamic_lut[uart].mutex);

        if ((ESP_OK == uart_get_buffered_data_len(g_uart_lut[uart].uart_num, (size_t *) &length)) && (length > 0)) {
            continue;
        }

        if (NULL != g_uart_dynamic_lut[uart].data_received_callback) {
            g_uart_dynamic_lut[uart].data_received_callback(uart);
        }

        taskYIELD();
    }
}

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

bool UART_Driver_Init(const eUart_t uart, const eBaudrate_t baudrate, data_received_callback_t data_received_callback) {
    if (!UART_Config_IsCorrectUart(uart)) {
        return false;
    }

    if ((baudrate < eBaudrate_First) || (baudrate >= eBaudrate_Last)) {
        return false;
    }

    if (NULL == data_received_callback) {
        return false;
    }

    if (eUartState_Initialized == g_uart_dynamic_lut[uart].state) {
        return true;
    }

    uart_config_t uart_init_struct = {0};

    const sUartDesc_t *uart_desc = UART_Config_GetUartDesc(uart);

    if (NULL == uart_desc) {
        return false;
    }

    g_uart_lut[uart] = *uart_desc;

    uart_init_struct.baud_rate = (const int) Baudrate_GetValue((eBaudrate_Default == baudrate) ? g_uart_lut[uart].baud : baudrate);
    uart_init_struct.data_bits = g_uart_lut[uart].data_bits;
    uart_init_struct.parity = g_uart_lut[uart].parity;
    uart_init_struct.stop_bits = g_uart_lut[uart].stop_bits;
    uart_init_struct.flow_ctrl = g_uart_lut[uart].flow_control;
    uart_init_struct.rx_flow_ctrl_thresh = g_uart_lut[uart].rx_flow_control_thresh;
    uart_init_struct.source_clk = g_uart_lut[uart].clock;
    uart_init_struct.flags.allow_pd = g_uart_lut[uart].allow_pd;

    if ((g_uart_lut[uart].rx_buffer_size <= UART_HW_FIFO_LEN(g_uart_lut[uart].uart_num)) || ((g_uart_lut[uart].tx_buffer_size != 0) && (g_uart_lut[uart].tx_buffer_size <= UART_HW_FIFO_LEN(g_uart_lut[uart].uart_num)))) {
        return false;
    }

    if (ESP_OK != uart_driver_install(g_uart_lut[uart].uart_num, g_uart_lut[uart].rx_buffer_size, g_uart_lut[uart].tx_buffer_size, 0, NULL, 0)) {
        return false;
    }

    if (ESP_OK != uart_param_config(g_uart_lut[uart].uart_num, &uart_init_struct)) {
        return false;
    }

    if (ESP_OK != uart_set_pin(g_uart_lut[uart].uart_num, g_uart_lut[uart].tx_pin, g_uart_lut[uart].rx_pin, g_uart_lut[uart].rts_pin, g_uart_lut[uart].cts_pin)) {
        return false;
    }

    if (ESP_OK != uart_set_mode(g_uart_lut[uart].uart_num, g_uart_lut[uart].mode)) {
        return false;
    }

    g_uart_dynamic_lut[uart].uart = uart;
    g_uart_dynamic_lut[uart].ring_buffer = Ring_Buffer_Init(g_uart_lut[uart].ring_buffer_capacity);

    if (NULL == g_uart_dynamic_lut[uart].ring_buffer) {
        return false;
    }

    g_uart_dynamic_lut[uart].mutex = xSemaphoreCreateRecursiveMutexStatic(&g_uart_dynamic_lut[uart].mutex_buffer);

    if (NULL == g_uart_dynamic_lut[uart].mutex) {
        return false;
    }

    if (!UART_Driver_CreateFeederThread(g_uart_dynamic_lut[uart].uart, &g_uart_dynamic_lut[uart])) {
        return false;
    }

    g_uart_dynamic_lut[uart].state = eUartState_Initialized;
    g_uart_dynamic_lut[uart].data_received_callback = data_received_callback;

    return true;
}

bool UART_Driver_Send(const eUart_t uart, uint8_t *data, const size_t size) {
    if (!UART_Config_IsCorrectUart(uart)) {
        return false;
    }

    if ((NULL == data) || (0 == size)) {
        return false;
    }

    if (eUartState_Default == g_uart_dynamic_lut[uart].state) {
        return false;
    }

    int sent_bytes = uart_write_bytes(g_uart_lut[uart].uart_num, data, size);

    return (sent_bytes == size);
}

bool UART_Driver_ReceiveByte(const eUart_t uart, uint8_t *data) {
    if (!UART_Config_IsCorrectUart(uart)) {
        return false;
    }

    if (NULL == data) {
        return false;
    }

    if (eUartState_Default == g_uart_dynamic_lut[uart].state) {
        return false;
    }

    if (pdTRUE != xSemaphoreTakeRecursive(g_uart_dynamic_lut[uart].mutex, UART_MUTEX_TIMEOUT)) {
        return false;
    }

    bool is_received = Ring_Buffer_Pop(g_uart_dynamic_lut[uart].ring_buffer, data);

    xSemaphoreGiveRecursive(g_uart_dynamic_lut[uart].mutex);

    return is_received;
}

#endif /* ENABLE_UART */
