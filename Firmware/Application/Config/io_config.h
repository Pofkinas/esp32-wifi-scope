#ifndef APPLICATION_CONFIG_IO_CONFIG_H_
#define APPLICATION_CONFIG_IO_CONFIG_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "framework_config.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "gpio_config.h"
#include "freertos_typedefs.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

#define IO_API_POLL_PERIOD_MS 50U

#define BUTTON_TRIGGERED_EVENT 0x01U
#define BUTTON_DEBOUNCE_MS 10U

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

typedef enum eIo {
    eIo_First = 0,
    eIo_Button = eIo_First,
    eIo_Last
} eIo_t;

typedef enum eActiveState {
    eActiveState_First = 0,
    eActiveState_Low = eActiveState_First,
    eActiveState_High,
    eActiveState_Both,
    eActiveState_Last
} eActiveState_t;

typedef struct sExtiDesc {
    gpio_num_t gpio_num;
    gpio_mode_t trigger_interrupt;
} sExtiDesc_t;

typedef struct sIoDesc {
    eGpio_t gpio_pin;
    eActiveState_t active_state;
    EventBits_t triggered_flag;
    bool is_debounce_enable;
    uint16_t debounce_period_ms;
    char *debounce_timer_name;
    bool is_exti;
    uint16_t default_press_period_ms;
    uint16_t long_press_period_ms;
} sIoDesc_t;

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

extern const sTaskDesc_t g_io_thread_attributes;

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

bool IO_Config_IsCorrectIo (const eIo_t io_device);
const sExtiDesc_t *IO_Config_GetExtiDesc (const eIo_t io_device);
const sIoDesc_t *IO_Config_GetIoDesc (const eIo_t io_device);

#endif /* APPLICATION_CONFIG_IO_CONFIG_H_ */
