/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "pwm_driver.h"

#if defined(ENABLE_PWM)
#include "driver/ledc.h"

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

static sPwmOcDesc_t g_oc_pwm_lut[ePwm_Last] = {0};
static ledc_channel_config_t channel_oc_struct[ePwm_Last] = {0};

static bool g_is_all_device_init = false;
static bool g_is_device_enabled[ePwm_Last] = {false};

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

bool PWM_Driver_InitAllDevices(void) {
    if (g_is_all_device_init) {
        return true;
    }

    g_is_all_device_init = true;

    ledc_timer_config_t timer_config_struct = {0};

    for (ePwm_t device = ePwm_First; device < ePwm_Last; device++) {
        const sPwmOcDesc_t *desc = PWM_Config_GetPwmOcDesc(device);

        if (NULL == desc) {
            g_is_all_device_init = false;
            return false;
        }

        g_oc_pwm_lut[device] = *desc;

        timer_config_struct.speed_mode = g_oc_pwm_lut[device].speed_mode;
        timer_config_struct.duty_resolution = g_oc_pwm_lut[device].duty_resolution;
        timer_config_struct.timer_num = g_oc_pwm_lut[device].timer_num;
        timer_config_struct.freq_hz = g_oc_pwm_lut[device].freq_hz;
        timer_config_struct.clk_cfg = g_oc_pwm_lut[device].clock_config;

        channel_oc_struct[device].gpio_num = g_oc_pwm_lut[device].gpio_num;
        channel_oc_struct[device].speed_mode = g_oc_pwm_lut[device].speed_mode;
        channel_oc_struct[device].channel = g_oc_pwm_lut[device].channel;
        channel_oc_struct[device].intr_type = g_oc_pwm_lut[device].interrupt_type;
        channel_oc_struct[device].timer_sel = g_oc_pwm_lut[device].timer_num;
        channel_oc_struct[device].duty = g_oc_pwm_lut[device].initial_duty;
        channel_oc_struct[device].hpoint = g_oc_pwm_lut[device].hpoint;
        channel_oc_struct[device].sleep_mode = g_oc_pwm_lut[device].sleep_mode;
        channel_oc_struct[device].flags.output_invert = g_oc_pwm_lut[device].invert_output;

        if (ESP_OK != ledc_timer_config(&timer_config_struct)) {
            g_is_all_device_init = false;
        }

        ledc_stop(g_oc_pwm_lut[device].speed_mode, g_oc_pwm_lut[device].channel, 0);

        g_is_device_enabled[device] = false;
    }

    return g_is_all_device_init;
}

bool PWM_Driver_EnableDevice(const ePwm_t device) {
    if (!PWM_Config_IsCorrectPwmDevice(device)) {
        return false;
    }

    if (!g_is_all_device_init) {
        return false;
    }

    if (g_is_device_enabled[device]) {
        return true;
    }

    if (ESP_OK != ledc_channel_config(&channel_oc_struct[device])) {
        return false;
    }

    g_is_device_enabled[device] = true;

    return true;
}

bool PWM_Driver_DisableDevice(const ePwm_t device) {
    if (!PWM_Config_IsCorrectPwmDevice(device)) {
        return false;
    }

    if (!g_is_all_device_init) {
        return false;
    }

    if (!g_is_device_enabled[device]) {
        return true;
    }

    ledc_stop(g_oc_pwm_lut[device].speed_mode, g_oc_pwm_lut[device].channel, 0);

    g_is_device_enabled[device] = false;

    return true;
}

bool PWM_Driver_ChangeDutyCycle(const ePwm_t device, const uint32_t value) {
    if (!PWM_Config_IsCorrectPwmDevice(device)) {
        return false;
    }

    if (!g_is_device_enabled[device]) {
        return false;
    }

    if (value > PWM_Driver_GetDeviceTimerResolution(device)) {
        return false;
    }

    if (ESP_OK != ledc_set_duty(g_oc_pwm_lut[device].speed_mode, g_oc_pwm_lut[device].channel, value)) {
        return false;
    }

    if (ESP_OK != ledc_update_duty(g_oc_pwm_lut[device].speed_mode, g_oc_pwm_lut[device].channel)) {
        return false;
    }

    return true;
}

uint32_t PWM_Driver_GetDeviceTimerResolution(const ePwm_t device) {
    if (!PWM_Config_IsCorrectPwmDevice(device)) {
        return 0;
    }

    if (!g_is_all_device_init) {
        return 0;
    }

    return ((1ULL << g_oc_pwm_lut[device].duty_resolution) - 1);
}

#endif /* ENABLE_PWM */
