#ifndef APPLICATION_CONFIG_UART_CONFIG_H_
#define APPLICATION_CONFIG_UART_CONFIG_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "framework_config.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "baudrate.h"
#include "gpio_config.h"
#include "freertos_typedefs.h"

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"

/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

typedef enum eUart {
    eUart_First = 0, 
    eUart_Debug = eUart_First,
    eUart_Last
} eUart_t;

typedef struct sUartDesc { 
    uart_port_t uart_num;
    eBaudrate_t baud;
    uart_word_length_t data_bits;
    uart_parity_t parity;
    uart_stop_bits_t stop_bits;
    uart_hw_flowcontrol_t flow_control;
    uint8_t rx_flow_control_thresh;
    uart_sclk_t clock;
    bool allow_pd;
    uart_mode_t mode;
    gpio_num_t tx_pin;
    gpio_num_t rx_pin;
    gpio_num_t rts_pin;
    gpio_num_t cts_pin;
    size_t tx_buffer_size;
    size_t rx_buffer_size;
    size_t ring_buffer_capacity;
} sUartDesc_t;

typedef struct sUartApiConst {
    size_t buffer_capacity;
} sUartApiConst_t;

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

extern const sTaskDesc_t g_fsm_thread_attributes;

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

bool UART_Config_IsCorrectUart (const eUart_t uart);
const sUartDesc_t *UART_Config_GetUartDesc (const eUart_t uart);
const sUartApiConst_t *UART_Config_GetUartApiConst (const eUart_t uart);

#endif /* APPLICATION_CONFIG_UART_CONFIG_H_ */
