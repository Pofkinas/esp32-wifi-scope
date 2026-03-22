/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "gpio_driver.h"

#if defined(ENABLE_GPIO)
#include "driver/gpio.h"

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

static sGpioDesc_t g_gpio_lut[eGpio_Last] = {0};

static bool g_is_all_pin_initialized = false;
static bool g_gpio_curent_state[eGpio_Last] = {0};

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

bool GPIO_Driver_InitAllPins (void) {
    if (g_is_all_pin_initialized) {
        return true;
    }

    g_is_all_pin_initialized = true;

    gpio_config_t gpio_init_struct = {0};

    for (eGpio_t pin = eGpio_First; pin < eGpio_Last; pin++) {
        const sGpioDesc_t *desc = GPIO_Config_GetGpioDesc(pin);
        
        if (NULL == desc) {
            return false;
        }
        
        g_gpio_lut[pin] = *desc;

        gpio_init_struct.pin_bit_mask = (1ULL << g_gpio_lut[pin].gpio_num);
        gpio_init_struct.mode = g_gpio_lut[pin].mode;
        gpio_init_struct.pull_up_en = g_gpio_lut[pin].pull_up;
        gpio_init_struct.pull_down_en = g_gpio_lut[pin].pull_down;
        
        if (ESP_OK != gpio_config(&gpio_init_struct)) {
            g_is_all_pin_initialized = false;
        }
    }

    return g_is_all_pin_initialized;
}

bool GPIO_Driver_WritePin (const eGpio_t gpio_pin, const bool pin_state) {
    if (!g_is_all_pin_initialized) {
        return false;
    }
    
    if (!GPIO_Config_IsCorrectGpio(gpio_pin)) {
        return false;
    }

    if (!((GPIO_MODE_OUTPUT | GPIO_MODE_OUTPUT_OD) & g_gpio_lut[gpio_pin].mode)) {
        return false;
    }

    g_gpio_curent_state[gpio_pin] = pin_state;

    return (ESP_OK == gpio_set_level(g_gpio_lut[gpio_pin].gpio_num, g_gpio_curent_state[gpio_pin]));
}

bool GPIO_Driver_ReadPin (const eGpio_t gpio_pin, bool *pin_state) {
    if (!g_is_all_pin_initialized) {
        return false;
    }
    
    if (!GPIO_Config_IsCorrectGpio(gpio_pin)) {
        return false;
    }

    if (NULL == pin_state) {
        return false;
    }

    if (!((GPIO_MODE_OUTPUT | GPIO_MODE_OUTPUT_OD) & g_gpio_lut[gpio_pin].mode)) {
        *pin_state = gpio_get_level(g_gpio_lut[gpio_pin].gpio_num);
    } else {
        *pin_state = g_gpio_curent_state[gpio_pin];
    }
    
    return true;
}

bool GPIO_Driver_TogglePin (const eGpio_t gpio_pin) {
    if (!g_is_all_pin_initialized) {
        return false;
    }

    if (!GPIO_Config_IsCorrectGpio(gpio_pin)) {
        return false;
    }

    g_gpio_curent_state[gpio_pin] = !g_gpio_curent_state[gpio_pin];

    return (ESP_OK == gpio_set_level(g_gpio_lut[gpio_pin].gpio_num, g_gpio_curent_state[gpio_pin]));
}

bool GPIO_Driver_ResetPin (const eGpio_t gpio_pin) {
    if (!g_is_all_pin_initialized) {
        return false;
    }
    
    if (!GPIO_Config_IsCorrectGpio(gpio_pin)) {
        return false;
    }

    g_gpio_curent_state[gpio_pin] = false;

    return (ESP_OK == gpio_reset_pin(g_gpio_lut[gpio_pin].gpio_num));
}

#endif /* ENABLE_GPIO */
