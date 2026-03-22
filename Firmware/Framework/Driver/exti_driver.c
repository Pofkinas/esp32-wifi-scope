/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "exti_driver.h"

#if defined(ENABLE_EXTI)
#include "io_config.h"
#include "gpio_config.h"

#include "driver/gpio.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

typedef enum eExtiStatus {
    eExtiStatus_First = 0,
    eExtiStatus_Default = eExtiStatus_First,
    eExtiStatus_Disabled,
    eExtiStatus_Enabled,
    eExtiStatus_Last
} eExtiStatus_t;

typedef struct sExtiDynamic {
    gpio_num_t gpio_num;
    eExtiStatus_t status;
    void (*callback) (void *context);
    void *callback_context;
} sExtiDynamic_t;

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

static const gpio_int_type_t g_exti_trigger_type_lut[eExtiTrigger_Last] = {
    [eExtiTrigger_Rising] = GPIO_INTR_POSEDGE,
    [eExtiTrigger_Falling] = GPIO_INTR_NEGEDGE,
    [eExtiTrigger_RisingFalling] = GPIO_INTR_ANYEDGE
};

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

bool is_exti_init = false;
static sExtiDesc_t g_exti_lut[eIo_Last] = {0};
static sExtiDynamic_t g_dynamic_exti_lut[eIo_Last] = {0};

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

static void EXTIx_IRQHandler (void *arg);

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

static void IRAM_ATTR EXTIx_IRQHandler (void *arg)  {
    sExtiDynamic_t *device = (sExtiDynamic_t *) arg;

    if (NULL == device) {
        return;
    }

    if ((NULL != device->callback) && (NULL != device->callback_context)) {
        device->callback(device->callback_context);
    }

    return;
}

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

bool Exti_Driver_InitDevice (const eIo_t exti_device, exti_callback_t exti_callback, void *callback_context) {
    if (!IO_Config_IsCorrectIo(exti_device)) {
        return false;
    }

    if (NULL == exti_callback) {
        return false;
    }

    if (eExtiStatus_Default != g_dynamic_exti_lut[exti_device].status) {
        return true;
    }

    const sExtiDesc_t *desc = IO_Config_GetExtiDesc(exti_device);

    if (NULL == desc) {
        return false;
    }

    g_exti_lut[exti_device] = *desc;

    g_dynamic_exti_lut[exti_device].gpio_num = g_exti_lut[exti_device].gpio_num;
    g_dynamic_exti_lut[exti_device].status = eExtiStatus_Disabled;
    g_dynamic_exti_lut[exti_device].callback = exti_callback;
    g_dynamic_exti_lut[exti_device].callback_context = callback_context;

    if (ESP_OK != gpio_set_intr_type(g_dynamic_exti_lut[exti_device].gpio_num, g_exti_lut[exti_device].trigger_interrupt)) {
        return false;
    }

    if (ESP_OK != gpio_intr_disable(g_dynamic_exti_lut[exti_device].gpio_num)) {
        return false;
    }

    if (!is_exti_init) {
        if (ESP_OK != gpio_install_isr_service(ESP_INTR_FLAG_IRAM)) {
            return false;
        }
        is_exti_init = true;
    }

    if (ESP_OK != gpio_isr_handler_add(g_dynamic_exti_lut[exti_device].gpio_num, EXTIx_IRQHandler, &g_dynamic_exti_lut[exti_device])) {
        return false;
    }

    return true;
}

bool Exti_Driver_DisableIt (const eIo_t exti_device) {
    if (!IO_Config_IsCorrectIo(exti_device)) {
        return false;
    }

    if (eExtiStatus_Default == g_dynamic_exti_lut[exti_device].status) {
        return false;
    }

    if (ESP_OK != gpio_intr_disable(g_dynamic_exti_lut[exti_device].gpio_num)) {
        return false;
    }

    g_dynamic_exti_lut[exti_device].status = eExtiStatus_Disabled;

    return true;
}

bool Exti_Driver_EnableIt (const eIo_t exti_device) {
    if (!IO_Config_IsCorrectIo(exti_device)) {
        return false;
    }

    if (eExtiStatus_Default == g_dynamic_exti_lut[exti_device].status) {
        return false;
    }

    if (ESP_OK != gpio_intr_enable(g_dynamic_exti_lut[exti_device].gpio_num)) {
        return false;
    }

    g_dynamic_exti_lut[exti_device].status = eExtiStatus_Enabled;

    return true;
}

bool Exti_Driver_SetTriggerType (const eIo_t exti_device, const eExtiTrigger_t trigger_type) {
    if (!IO_Config_IsCorrectIo(exti_device)) {
        return false;
    }

    if (eExtiStatus_Default == g_dynamic_exti_lut[exti_device].status) {
        return false;
    }

    if (ESP_OK != gpio_set_intr_type(g_dynamic_exti_lut[exti_device].gpio_num, g_exti_trigger_type_lut[trigger_type])) {
        return false;
    }

    return true;
}

bool Exti_Driver_ClearFlag (const eIo_t exti_device) {
    // Note: ESP32 GPIO interrupt flags are automatically cleared when the interrupt is handled
    return true;
}

#endif /* ENABLE_EXTI */
