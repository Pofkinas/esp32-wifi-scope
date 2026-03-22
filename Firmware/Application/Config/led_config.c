/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "led_config.h"

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
#if defined(ENABLE_LED)
static const sLedDesc_t g_led_static_lut[eLed_Last] = {
    [eLed_Led] = {
        .led_pin = eGpio_Led,
        .is_inverted = false,
        .blink_timer_name = "Onboard_LED"
    }
};
#endif /* ENABLE_LED */

#if defined(ENABLE_PWM_LED)
static const sLedPwmDesc_t g_pwm_led_static_lut[eLedPwm_Last] = {
    [eLedPwm_PulseLed] = {
        .pwm_device = ePwm_PulseLed,
        .pulse_timer_name = "Pulse_LED",
    }
};
#endif /* ENABLE_PWM_LED */
/* clang-format on */

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

const sTaskDesc_t g_led_thread_attributes = {
    .name = "LED_APP",
    .stack_size = LED_THREAD_STACK_SIZE,
    .priority = eTaskPriority_Normal
};

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

#if defined(ENABLE_LED)
bool LED_Config_IsCorrectLed (const eLed_t led) {
    return (led >= eLed_First) && (led < eLed_Last);
}

const sLedDesc_t *LED_Config_GetLedDesc (const eLed_t led) {
    if (!LED_Config_IsCorrectLed(led)) {
        return NULL;
    }

    return &g_led_static_lut[led];
}
#endif /* ENABLE_LED */

#if defined(ENABLE_PWM_LED)
bool LED_Config_IsCorrectPwmLed (const eLedPwm_t led) {
    return (led >= eLedPwm_First) && (led < eLedPwm_Last);
}

const sLedPwmDesc_t *LED_Config_GetPwmLedDesc (const eLedPwm_t led) {
    if (!LED_Config_IsCorrectPwmLed(led)) {
        return NULL;
    }

    return &g_pwm_led_static_lut[led];
}
#endif /* ENABLE_PWM_LED */
