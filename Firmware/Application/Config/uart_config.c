/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "uart_config.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

/* clang-format off */
static const sUartDesc_t g_static_uart_lut[eUart_Last] = {
    [eUart_Debug] = {
        .uart_num = UART_NUM_0,
        .baud = UART_0_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_control = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_control_thresh = 0,
        .clock = UART_SCLK_APB,
        .allow_pd = false,
        .mode = UART_MODE_UART,
        .tx_pin = GPIO_NUM_43,
        .rx_pin = GPIO_NUM_44,
        .rts_pin = GPIO_NUM_NC, // Not used
        .cts_pin = GPIO_NUM_NC, // Not used
        .tx_buffer_size = DEBUG_MESSAGE_SIZE,
        .rx_buffer_size = 256,
        .ring_buffer_capacity = UART_DEBUG_BUFFER_CAPACITY
    }
};

static const sUartApiConst_t g_static_uart_api_lut[eUart_Last] = {
    [eUart_Debug] = {
        .buffer_capacity = UART_DEBUG_BUFFER_CAPACITY
    }
};
/* clang-format on */

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/* clang-format off */
const sTaskDesc_t g_fsm_thread_attributes = {
    .name = "UART_API",
    .stack_size = UART_API_THREAD_STACK_SIZE,
    .priority = eTaskPriority_Normal
};
/* clang-format on */

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

bool UART_Config_IsCorrectUart(const eUart_t uart) {
    return (uart >= eUart_First) && (uart < eUart_Last);
}

const sUartDesc_t *UART_Config_GetUartDesc(const eUart_t uart) {
    if (!UART_Config_IsCorrectUart(uart)) {
        return NULL;
    }

    return &g_static_uart_lut[uart];
}

const sUartApiConst_t *UART_Config_GetUartApiConst(const eUart_t uart) {
    if (!UART_Config_IsCorrectUart(uart)) {
        return NULL;
    }

    return &g_static_uart_api_lut[uart];
}
