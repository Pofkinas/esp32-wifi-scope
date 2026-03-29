#ifndef APPLICATION_CONFIG_LED_CONFIG_H_
#define APPLICATION_CONFIG_LED_CONFIG_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "framework_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "freertos_types.h"

#if defined(ENABLE_LED)
#include "gpio_config.h"
#endif /* ENABLE_LED */

#if defined(ENABLE_PWM_LED)
#include "pwm_config.h"
#endif /* ENABLE_PWM_LED */

/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

#define PULSE_TIMER_FREQUENCY 10

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

#if defined(ENABLE_LED)
typedef enum eLed {
    eLed_First = 0,
    eLed_Led = eLed_First,
    eLed_Last
} eLed_t;
#endif /* ENABLE_LED */

#if defined(ENABLE_PWM_LED)
typedef enum eLedPwm {
    eLedPwm_First = 0,
    eLedPwm_PulseLed = eLedPwm_First,
    eLedPwm_Last
} eLedPwm_t;
#endif /* ENABLE_PWM_LED */

#if defined(ENABLE_LED)
typedef struct sLedDesc {
    eGpio_t led_pin;
    bool is_inverted;
    char *blink_timer_name;
} sLedDesc_t;
#endif /* ENABLE_LED */

#if defined(ENABLE_PWM_LED)
typedef struct sLedPwmDesc {
    ePwm_t pwm_device;
    char *pulse_timer_name;
} sLedPwmDesc_t;
#endif /* ENABLE_PWM_LED */

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

extern const sTaskDesc_t g_led_thread_attributes;

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

#if defined(ENABLE_LED)
bool LED_Config_IsCorrectLed(const eLed_t led);
const sLedDesc_t *LED_Config_GetLedDesc(const eLed_t led);
#endif /* ENABLE_LED */

#if defined(ENABLE_PWM_LED)
bool LED_Config_IsCorrectPwmLed(const eLedPwm_t led);
const sLedPwmDesc_t *LED_Config_GetPwmLedDesc(const eLedPwm_t led);
#endif /* ENABLE_PWM_LED */

#endif /* APPLICATION_CONFIG_LED_CONFIG_H_ */
