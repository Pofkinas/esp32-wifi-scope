#ifndef SOURCE_UTILITY_FRAMEWORK_CONFIG_H_
#define SOURCE_UTILITY_FRAMEWORK_CONFIG_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#ifndef PROJECT_CONFIG_H
#include "example_config.h"
#error "PROJECT_CONFIG_H not defined. Add -DPROJECT_CONFIG_H=\"project_config.h\" to your build."
#endif /* PROJECT_CONFIG_H */

#include PROJECT_CONFIG_H

/**********************************************************************************************************************
 * Project configuration analysis
 *********************************************************************************************************************/

#if (defined(ENABLE_UART) || defined(ENABLE_PWM) || defined(ENABLE_LED) || defined(ENABLE_PWM_LED)\
    || defined(ENABLE_IO) || defined(ENABLE_EXTI)) && !defined(ENABLE_GPIO)
#error "At least one peripheral or module requires GPIO to be enabled."
#endif /* (ENABLE_UART || ENABLE_PWM || ENABLE_LED || ENABLE_PWM_LED || ENABLE_IO || ENABLE_EXTI ||\
        ) && !ENABLE_GPIO */

#if defined(ENABLE_UART_DEBUG) && !defined(ENABLE_UART)
#error "DEBUG_UART requires UART to be enabled."
#endif /* ENABLE_UART_DEBUG && !ENABLE_UART */

#if defined(ENABLE_CLI) && (!defined(DEBUG_UART) || !defined(ENABLE_UART_DEBUG))
#error "CLI requires DEBUG_UART to be enabled."
#endif /* ENABLE_CLI && (!DEBUG_UART || !ENABLE_UART_DEBUG) */

#if defined(ENABLE_CLI) && !defined(ENABLE_DEFAULT_CMD) && !defined(ENABLE_CUSTOM_CMD)
#error "CLI requires DEFAULT_CMD or CUSTOM_CMD to be enabled."
#endif /* ENABLE_CLI && (!ENABLE_DEFAULT_CMD || !ENABLE_CUSTOM_CMD) */

#if defined(ENABLE_PWM_LED) && (!defined(ENABLE_PWM) || !defined(ENABLE_TIMER))
#error "PWM_LED requires PWM and TIMER to be enabled."
#endif /* ENABLE_PWM_LED && (!ENABLE_PWM || !ENABLE_TIMER) */

#if defined(ENABLE_PWM) && !defined(ENABLE_TIMER)
#error "PWM requires TIMER to be enabled."
#endif /* ENABLE_PWM && !ENABLE_TIMER */

#if defined(ENABLE_CUSTOM_CMD) && !defined(ENABLE_CLI)
#error "CUSTOM_CMD requires ENABLE_CLI to be defined."
#endif /* CUSTOM_CMD && !ENABLE_CLI */

#endif /* SOURCE_UTILITY_FRAMEWORK_CONFIG_H_ */
