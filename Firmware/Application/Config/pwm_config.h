#ifndef APPLICATION_CONFIG_PWM_CONFIG_H_
#define APPLICATION_CONFIG_PWM_CONFIG_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "framework_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "driver/ledc.h"

/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

typedef enum ePwm {
    ePwm_First = 0,
    ePwm_PulseLed = ePwm_First,
    ePwm_Last
} ePwm_t;

typedef struct sPwmOcDesc {
    gpio_num_t gpio_num;
    ledc_timer_t timer_num;
    ledc_channel_t channel;
    ledc_mode_t speed_mode;
    uint32_t freq_hz;
    ledc_timer_bit_t duty_resolution;
    ledc_clk_cfg_t clock_config;
    ledc_intr_type_t interrupt_type;
    uint32_t initial_duty;
    int hpoint;
    ledc_sleep_mode_t sleep_mode;
    bool invert_output;
} sPwmOcDesc_t;

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

bool PWM_Config_IsCorrectPwmDevice(const ePwm_t pwm_channel);
const sPwmOcDesc_t *PWM_Config_GetPwmOcDesc(const ePwm_t pwm_channel);

#endif /* APPLICATION_CONFIG_PWM_CONFIG_H_ */
