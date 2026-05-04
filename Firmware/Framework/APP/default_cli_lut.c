/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "default_cli_lut.h"

#if defined(ENABLE_DEFAULT_CMD)

#include "cli_cmd.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

#define DEFINE_CMD(command_string) .command = command_string, .command_length = sizeof(command_string) - 1

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/* clang-format off */
sCmdDesc_t g_default_cmd_lut[eCliDefaultCmd_Last] = {
#if defined(ENABLE_LED)
    [eCliDefaultCmd_Led_Set] = {
        DEFINE_CMD("led_set:"),
        .handler = CLI_CMD_Led_Set
        /* e. g. led_set:<eLed_t> */
    },
    [eCliDefaultCmd_Led_Reset] = {
        DEFINE_CMD("led_reset:"),
        .handler = CLI_CMD_Led_Reset
        /* e. g. led_reset:<eLed_t> */
    },
    [eCliDefaultCmd_Led_Toggle] = {
        DEFINE_CMD("led_toggle:"),
        .handler = CLI_CMD_Led_Toggle
        /* e. g. led_toggle:<eLed_t> */
    },
    [eCliDefaultCmd_Led_BlinkCount] = {
        DEFINE_CMD("led_blink_cnt:"),
        .handler = CLI_CMD_Led_BlinkCount
        /* e. g. led_blink_count:<eLed_t>, <count>, <frequency_hz> */
    },
    [eCliDefaultCmd_Led_BlinkDuration] = {
        DEFINE_CMD("led_blink_dur:"),
        .handler = CLI_CMD_Led_BlinkDuration
        /* e. g. led_blink_duration:<eLed_t>, <duration_ms>, <frequency_hz> */
    },
    [eCliDefaultCmd_Led_StopBlink] = {
        DEFINE_CMD("led_blink_stop:"),
        .handler = CLI_CMD_Led_StopBlink
        /* e. g. led_stop_blink:<eLed_t> */
    },
#endif /* ENABLE_LED */

#if defined(ENABLE_PWM_LED)
    [eCliDefaultCmd_Pwm_LedSetBrightness] = {
        DEFINE_CMD("led_setb:"),
        .handler = CLI_CMD_Pwm_LedSetBrightness
        /* e. g. led_setb:<eLedPwm_t>, <duty_cycle> */
    },
    [eCliDefaultCmd_Pwm_LedPulse] = {
        DEFINE_CMD("led_pulse:"),
        .handler = CLI_CMD_Pwm_LedPulse
        /* e. g. led_pulse:<eLedPwm_t>, <pulse_time>, <pulse_frequency> */
    },
#endif /* ENABLE_PWM_LED */

// TODO: Really need WiFi APP thread here to deal with this, especially for connection management
#if defined(ENABLE_WIFI)
    [eCliCustomCmd_WifiConnect] = {
        DEFINE_CMD("wifi_connect"),
        .handler = Custom_CLI_CMD_WifiConnect
        /* e. g. wifi_connect or wifi_connect:<ssid>, <password> */
    },
    [eCliCustomCmd_WifiDisconnect] = {
        DEFINE_CMD("wifi_disconnect"),
        .handler = Custom_CLI_CMD_WifiDisconnect
        /* e. g. wifi_disconnect */
    },
    [eCliCustomCmd_WifiStatus] = {
        DEFINE_CMD("wifi_status"),
        .handler = Custom_CLI_CMD_WifiStatus
        /* e. g. wifi_status */
    },
#endif /* ENABLE_WIFI */
    [eCliDefaultCmd_RgbToHsv] = {
        DEFINE_CMD("rgb:"),
        .handler = CLI_CMD_Led_RgbToHsv
        /* e. g. rgb:<r>, <g>, <b> */
    },
    [eCliDefaultCmd_HsvToRgb] = {
        DEFINE_CMD("hsv:"),
        .handler = CLI_CMD_Led_HsvToRgb
        /* e. g. hsv:<h>, <s>, <v> */
    }
    // TODO: Add VL53L0X calibration
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

#endif /* ENABLE_DEFAULT_CMD */
