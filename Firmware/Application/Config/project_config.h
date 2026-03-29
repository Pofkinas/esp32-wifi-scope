#ifndef APPLICATION_CONFIG_PROJECT_CONFIG_H_
#define APPLICATION_CONFIG_PROJECT_CONFIG_H_

//=============================================================================
// MPU
//-----------------------------------------------------------------------------

#define MCU_ESP32_S3

//=============================================================================
// FEATURE FLAGS
//-----------------------------------------------------------------------------
/// Uncomment the flags for the project peripherals and modules:

/// -- GPIO                    // Enable GPIO functionality
#define ENABLE_GPIO

/// -- TIMER                   // Enable TIMER functionality
#define ENABLE_TIMER

/// -- UART                    // Enable UART functionality
#define ENABLE_UART

/// -- PWM                     // Enable PWM functionality
#define ENABLE_PWM

/// -- ADC                     // Enable ADC functionality
#define ENABLE_ADC

/// -- DEBUG UART              // Enable Debug UART functionality
#define ENABLE_UART_DEBUG

/// -- CLI                     // Enable Command Line Interface (CLI) over UART
#define ENABLE_CLI
#define ENABLE_DEFAULT_CMD
#define ENABLE_CUSTOM_CMD

/// -- CMD                     // Enable Command API modules
#define ENABLE_CMD
#define ENABLE_CMD_HELPER

/// -- LEDs                    // Enable LED functionality
#define ENABLE_LED
#define ENABLE_PWM_LED

/// -- I/Os                    // Enable Input/Output functionality
#define ENABLE_IO

/// -- EXTI                    // Enable EXTI functionality
#define ENABLE_EXTI

/// Utilities
#define ENABLE_COLOUR

/// Misc
#define ENABLE_CAPTURE
#define ENABLE_VOLTAGE

//=============================================================================
// SYSTEM CONFIGURATION
//-----------------------------------------------------------------------------
/// System clock frequency (Hz)

#define SYSTEM_CLOCK_HZ 240000000UL

//=============================================================================
// THREAD CONFIGURATION
//-----------------------------------------------------------------------------

#define CLI_APP_THREAD_STACK_SIZE (256 * 8)
#define CLI_APP_THREAD_PRIORITY eTaskPriority_Normal

#define LED_APP_THREAD_STACK_SIZE (256 * 8)
#define LED_APP_THREAD_PRIORITY eTaskPriority_Normal

#define UART_API_THREAD_STACK_SIZE (256 * 6)
#define UART_API_THREAD_PRIORITY eTaskPriority_Normal

#define IO_API_THREAD_STACK_SIZE (256 * 8)
#define IO_API_THREAD_PRIORITY eTaskPriority_Normal

#define CAPTURE_API_THREAD_STACK_SIZE (256 * 36)
#define CAPTURE_API_THREAD_PRIORITY eTaskPriority_Normal

#define MAIN_TEST_THREAD_STACK_SIZE (256 * 36)
#define MAIN_TEST_THREAD_PRIORITY eTaskPriority_Normal

//=============================================================================
// UART CONFIGURATION
//-----------------------------------------------------------------------------

#if defined(ENABLE_UART)
#define UART0 eUart_Debug
#define UART_0_BAUDRATE eBaudrate_115200

#define UART_API_MESSAGE_QUEUE_CAPACITY 10
#define UART_API_MESSAGE_QUEUE_PUT_TIMEOUT 0U

#if defined(ENABLE_UART_DEBUG)
#define DEBUG_UART UART0

#define DEBUG_DELIMITER "\r\n"
#define DEBUG_MESSAGE_SIZE 512
#define UART_DEBUG_BUFFER_CAPACITY 512

#define DEBUG_MESSAGE_TIMEOUT 1000
#define DEBUG_MUTEX_TIMEOUT 0U
#endif /* ENABLE_UART_DEBUG */
#endif /* ENABLE_UART */

//=============================================================================
// LED CONFIGURATION
//-----------------------------------------------------------------------------

#if defined(ENABLE_LED)
/// Blink Maximum time (s)
#define MAX_BLINK_TIME 59
/// Blink frequency limits (Hz)
#define MIN_BLINK_FREQUENCY 20
#define MAX_BLINK_FREQUENCY 100
#endif /* ENABLE_LED */

#if defined(ENABLE_PWM_LED)
// Pulsing Maximum time (s)
#define MAX_PULSING_TIME 59
// Pulsing frequency limits (Hz)
#define MAX_PULSE_FREQUENCY 500
// #define MAX_DUTY_CYCLE 65535
// #define MIN_PULSE_FREQUENCY (MAX_DUTY_CYCLE / 1000)
#define MIN_PULSE_FREQUENCY 10
#endif /* ENABLE_PWM_LED */

#if defined(ENABLE_LED) || defined(ENABLE_PWM_LED)
#define LED_MESSAGE_QUEUE_CAPACITY 10
#define LED_MESSAGE_QUEUE_TIMEOUT portMAX_DELAY
#define LED_MESSAGE_QUEUE_PUT_TIMEOUT 0U
#endif /* ENABLE_LED || ENABLE_PWM_LED */

//=============================================================================
// DEBUG
//-----------------------------------------------------------------------------

#if defined(ENABLE_UART_DEBUG)
// Custom debug flags
#define DEBUG_MAIN
#define CUSTOM_CLI_CMD_HANDLERS

// APP layer debug flags
#define DEBUG_CLI_APP
#define DEBUG_DEFAULT_CMD
#define DEBUG_CUSTOM_CMD
#define DEBUG_LED_APP

// API layer debug flags
#define DEBUG_CMD_API
#define DEBUG_CMD_API_HELPER
#define DEBUG_UART_API
#define DEBUG_IO_API
#define DEBUG_LED_API
#define DEBUG_CAPTURE_API
#endif /* ENABLE_UART_DEBUG */

//=============================================================================
// MISCELLANEOUS
//-----------------------------------------------------------------------------

#if defined(ENABLE_CLI)
#define CLI_COMMAND_MESSAGE_CAPACITY 20
#define RESPONSE_MESSAGE_CAPACITY 128
#define CMD_SEPARATOR ","
#endif /* ENABLE_CLI */

#if defined(ENABLE_DEFAULT_CMD) || defined(ENABLE_CUSTOM_CMD)
#define CMD_SEPARATOR ","
#define CMD_SEPARATOR_LENGTH (sizeof(CMD_SEPARATOR) - 1)
#endif /* ENABLE_DEFAULT_CMD || ENABLE_CUSTOM_CMD */

#define HEAP_API_MUTEX_TIMEOUT 0U

#define BYTE 8
#define BASE_10 10
#define MAX_PID_DT 0.5f // Maximum dt for PID update to avoid large jumps

//=============================================================================
// CUSTOM FLAGS
//-----------------------------------------------------------------------------

#endif /* APPLICATION_CONFIG_PROJECT_CONFIG_H_ */
