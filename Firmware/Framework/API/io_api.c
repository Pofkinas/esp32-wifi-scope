/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "io_api.h"

#if defined(ENABLE_IO)
#include <stdio.h>
#include "debug_api.h"
#include "exti_driver.h"
#include "gpio_driver.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos_typedefs.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

#define DEFAULT_TIMER_TICKS 1U

#define MUTEX_TIMEOUT 0U
#define TIMER_TIMEOUT 0U

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

typedef enum eIoState {
    eIoState_First = 0,
    eIoState_Default = eIoState_First,
    eIoState_Init,
    eIoState_Started,
    eIoState_Last
} eIoState_t;

typedef enum eIoDeviceState {
    eIoDeviceState_First = 0,
    eIoDeviceState_Default = eIoDeviceState_First,
    eIoDeviceState_Init,
    eIoDeviceState_DebouncePress,
    eIoDeviceState_Pressed,
    eIoDeviceState_DebounceRelease,
    eIoDeviceState_Last
} eIoDeviceState_t;

typedef struct sIoDynamic {
    eIo_t device;
    eIoDeviceState_t state;
    eExtiTrigger_t press_trigger;
    eExtiTrigger_t release_trigger;
    TimerHandle_t debounce_timer;
    StaticTimer_t debounce_timer_buffer;
    SemaphoreHandle_t mutex;
    StaticSemaphore_t mutex_buffer;
    EventGroupHandle_t event;
    sIoEvent_t event_data;
} sIoDynamic_t;

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

#if defined(DEBUG_IO_API)
CREATE_MODULE_NAME(IO_API)
#else
CREATE_MODULE_NAME_EMPTY
#endif /* DEBUG_IO_API */

/* clang-format off */
static const eExtiTrigger_t g_press_trigger_lut[eActiveState_Last] = {
    [eActiveState_Low] = eExtiTrigger_Falling,
    [eActiveState_High] = eExtiTrigger_Rising,
    [eActiveState_Both] = eExtiTrigger_RisingFalling
};
/* clang-format on */

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

static TaskHandle_t g_io_thread = NULL;
static StackType_t g_io_thread_stack[STACK_SIZE_IN_BYTES(IO_THREAD_STACK_SIZE)] = {0};
static StaticTask_t g_io_thread_buffer = {0};

static eIoState_t g_io_state = eIoState_Default;
static bool g_has_polled_io = false;
static sIoDesc_t g_static_io_desc_lut[eIo_Last] = {0};
static sIoDynamic_t g_dynamic_io_lut[eIo_Last] = {0};

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

static void IO_API_PollThread(void *pvParameters);
static void IO_API_ExtiTriggered(void *context);
static bool IO_API_StartDebounceTimer(const eIo_t device, const bool is_from_isr);
static void IO_API_DebounceTimerCallback(TimerHandle_t xTimer);
static bool IO_API_IsGpioStateCorrect(const eIo_t device, const sIoDynamic_t *device_data);
static bool IO_API_IsGpioTriggered(const bool io_state, const eExtiTrigger_t trigger_state);
static uint32_t IO_API_GetDetectedPressType(const eIo_t device, const sIoDynamic_t *device_data);
static void IO_API_NotifyNonDebounce(const eIo_t device, sIoDynamic_t *const device_data, const bool is_from_isr);
static bool IO_API_ConfigureTriggers(const eActiveState_t active_state, sIoDynamic_t *device);

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

static void IO_API_PollThread(void *pvParameters) {
    eIo_t device;

    while (1) {
        for (device = eIo_First; device < eIo_Last; device++) {
            if (g_static_io_desc_lut[device].is_exti) {
                continue;
            }

            if (g_static_io_desc_lut[device].is_debounce_enable) {
                if ((eIoDeviceState_DebouncePress == g_dynamic_io_lut[device].state) || (eIoDeviceState_DebounceRelease == g_dynamic_io_lut[device].state)) {
                    continue;
                }
            }

            if (!IO_API_IsGpioStateCorrect(device, &g_dynamic_io_lut[device])) {
                continue;
            }

            if (g_static_io_desc_lut[device].is_debounce_enable) {
                IO_API_StartDebounceTimer(device, false);
            } else {
                if (pdTRUE != xSemaphoreTakeRecursive(g_dynamic_io_lut[device].mutex, MUTEX_TIMEOUT)) {
                    continue;
                }

                g_dynamic_io_lut[device].state = (eIoDeviceState_Init == g_dynamic_io_lut[device].state) ? eIoDeviceState_Pressed : eIoDeviceState_Init;

                IO_API_NotifyNonDebounce(device, &g_dynamic_io_lut[device], false);

                xSemaphoreGiveRecursive(g_dynamic_io_lut[device].mutex);

                TRACE_INFO("IO_Thread: IO [%d] triggered\n", device);
            }
        }

        // TODO: redefine pdMS_TO_TICKS with guard 0/1, and compilation error if arg time is less than configTICK_RATE_HZ
        vTaskDelay(pdMS_TO_TICKS(IO_API_POLL_PERIOD_MS));
    }
}

#if defined(MCU_ESP32_S3)
static void IRAM_ATTR IO_API_ExtiTriggered(void *context) {
#else
static void IO_API_ExtiTriggered(void *context) {
#endif /* MCU_ESP32_S3 */
    if (eIoState_Started != g_io_state) {
        return;
    }

    sIoDynamic_t *device = (sIoDynamic_t *) context;

    if (NULL == device) {
        return;
    }

    if (eIoDeviceState_Init == device->state) {
        device->event_data.detected_press_type = ePressType_None;
        device->event_data.pressed_tick = 0;
    }

    if (!g_static_io_desc_lut[device->device].is_debounce_enable) {
        device->state = (eIoDeviceState_Init == device->state) ? eIoDeviceState_Pressed : eIoDeviceState_Init;

        IO_API_NotifyNonDebounce(device->device, device, true);

        return;
    }

    Exti_Driver_DisableIt(device->device);

    if (!IO_API_StartDebounceTimer(device->device, true)) {
        Exti_Driver_EnableIt(device->device);

        return;
    }

    return;
}

static bool IO_API_StartDebounceTimer(const eIo_t device, const bool is_from_isr) {
    if (!IO_Config_IsCorrectIo(device)) {
        return false;
    }

    if (eIoDeviceState_Default == g_dynamic_io_lut[device].state) {
        return false;
    }

    eIoDeviceState_t next_state = (eIoDeviceState_Init == g_dynamic_io_lut[device].state) ? eIoDeviceState_DebouncePress : eIoDeviceState_DebounceRelease;

    if (is_from_isr) {
        // TODO: redefine pdMS_TO_TICKS with guard 0/1, and compilation error if arg time is less than configTICK_RATE_HZ

        g_dynamic_io_lut[device].state = next_state;
        return (pdTRUE == xTimerChangePeriodFromISR(g_dynamic_io_lut[device].debounce_timer, pdMS_TO_TICKS(g_static_io_desc_lut[device].debounce_period_ms), NULL));
    }

    if (pdTRUE != xSemaphoreTakeRecursive(g_dynamic_io_lut[device].mutex, MUTEX_TIMEOUT)) {
        return false;
    }

    g_dynamic_io_lut[device].state = next_state;

    xSemaphoreGiveRecursive(g_dynamic_io_lut[device].mutex);

    // TODO: redefine pdMS_TO_TICKS with guard 0/1, and compilation error if arg time is less than configTICK_RATE_HZ
    return (pdTRUE == xTimerChangePeriod(g_dynamic_io_lut[device].debounce_timer, pdMS_TO_TICKS(g_static_io_desc_lut[device].debounce_period_ms), TIMER_TIMEOUT));
}

static void IO_API_DebounceTimerCallback(TimerHandle_t xTimer) {
    if (eIoState_Started != g_io_state) {
        return;
    }

    sIoDynamic_t *debounce_io = (sIoDynamic_t *) pvTimerGetTimerID(xTimer);

    if (NULL == debounce_io) {
        return;
    }

    bool debounce_status = true;

    if ((eIoDeviceState_DebouncePress != debounce_io->state) && (eIoDeviceState_DebounceRelease != debounce_io->state)) {
        TRACE_WRN("Debounce: exit early, state [%d]\n", debounce_io->state);

        return;
    }

    if (!IO_API_IsGpioStateCorrect(debounce_io->device, debounce_io)) {
        TRACE_WRN("Debounce: GPIO state is incorrect [%d]\n", debounce_io->device);

        debounce_status = false;
    }

    bool is_mutex_taken = false;

    if (pdTRUE == xSemaphoreTakeRecursive(debounce_io->mutex, MUTEX_TIMEOUT)) {
        is_mutex_taken = true;
    } else {
        debounce_status = false;
    }

    if (g_static_io_desc_lut[debounce_io->device].is_exti) {
        if (!Exti_Driver_ClearFlag(debounce_io->device)) {
            debounce_status = false;
        }

        Exti_Driver_EnableIt(debounce_io->device);

        if (debounce_status && (eIoDeviceState_DebouncePress == debounce_io->state)) {
            debounce_io->event_data.pressed_tick = xTaskGetTickCount();
            debounce_io->state = eIoDeviceState_Pressed;

            if (!Exti_Driver_SetTriggerType(debounce_io->device, debounce_io->release_trigger)) {
                debounce_status = false;
            }

            if (is_mutex_taken) {
                xSemaphoreGiveRecursive(debounce_io->mutex);
            }

            return;
        } else {
            if (!Exti_Driver_SetTriggerType(debounce_io->device, debounce_io->press_trigger)) {
                debounce_status = false;
            }
        }
    } else {
        if (debounce_status && (eIoDeviceState_DebouncePress == debounce_io->state)) {
            debounce_io->event_data.pressed_tick = xTaskGetTickCount();

            debounce_io->state = eIoDeviceState_Pressed;

            if (is_mutex_taken) {
                xSemaphoreGiveRecursive(debounce_io->mutex);
            }

            return;
        }
    }

    debounce_io->state = eIoDeviceState_Init;

    if (is_mutex_taken) {
        xSemaphoreGiveRecursive(debounce_io->mutex);
    }

    if (!debounce_status) {
        TRACE_WRN("Debounce: IO [%d] debounce failed\n", debounce_io->device);

        return;
    }

    debounce_io->event_data.detected_press_type = IO_API_GetDetectedPressType(debounce_io->device, debounce_io);

    if (debounce_io->event != NULL) {
        xEventGroupSetBits(debounce_io->event, g_static_io_desc_lut[debounce_io->device].triggered_flag);
    }

    return;
}

static bool IO_API_IsGpioStateCorrect(const eIo_t device, const sIoDynamic_t *device_data) {
    if (!IO_Config_IsCorrectIo(device)) {
        return false;
    }

    if (eIoDeviceState_Default == device_data->state) {
        return false;
    }

    bool io_state = false;
    eExtiTrigger_t trigger_state = eExtiTrigger_First;

    if (!GPIO_Driver_ReadPin(g_static_io_desc_lut[device].gpio_pin, &io_state)) {
        return false;
    }

    switch (device_data->state) {
        case eIoDeviceState_DebouncePress: {
            trigger_state = device_data->press_trigger;
        } break;
        case eIoDeviceState_DebounceRelease: {
            trigger_state = device_data->release_trigger;
        } break;
        case eIoDeviceState_Pressed: {
            trigger_state = device_data->release_trigger;
        } break;
        default: {
            trigger_state = device_data->press_trigger;
        }
    }

    return IO_API_IsGpioTriggered(io_state, trigger_state);
}

static bool IO_API_IsGpioTriggered(const bool io_state, const eExtiTrigger_t trigger_state) {
    bool is_triggered = false;

    switch (trigger_state) {
        case eExtiTrigger_Rising: {
            is_triggered = io_state;
        } break;
        case eExtiTrigger_Falling: {
            is_triggered = !io_state;
        } break;
        case eExtiTrigger_RisingFalling: {
            is_triggered = true;
        } break;
        default: {
            is_triggered = false;
        }
    }

    return is_triggered;
}

static uint32_t IO_API_GetDetectedPressType(const eIo_t device, const sIoDynamic_t *device_data) {
    uint32_t press_type = 0;

    if (!IO_Config_IsCorrectIo(device)) {
        return press_type;
    }

    TickType_t current_tick = xTaskGetTickCount();
    uint32_t press_duration = pdTICKS_TO_MS(current_tick - device_data->event_data.pressed_tick);

    if (press_duration < g_static_io_desc_lut[device].default_press_period_ms) {
        press_type |= ePressType_Default;
    } else if (press_duration < g_static_io_desc_lut[device].long_press_period_ms) {
        press_type |= ePressType_Long;
    }

    return press_type;
}

static void IO_API_NotifyNonDebounce(const eIo_t device, sIoDynamic_t *const device_data, const bool is_from_isr) {
    if (!IO_Config_IsCorrectIo(device)) {
        return;
    }

    if (eIoDeviceState_Pressed == device_data->state) {
        device_data->event_data.pressed_tick = xTaskGetTickCount();
        device_data->event_data.detected_press_type = ePressType_Default;

        if (device_data->event == NULL) {
            return;
        }

        if (is_from_isr) {
            xEventGroupSetBitsFromISR(device_data->event, g_static_io_desc_lut[device].triggered_flag, NULL);
        } else {
            xEventGroupSetBits(device_data->event, g_static_io_desc_lut[device].triggered_flag);
        }
    } else {
        device_data->event_data.detected_press_type = IO_API_GetDetectedPressType(device, device_data);
    }

    return;
}

static bool IO_API_ConfigureTriggers(const eActiveState_t active_state, sIoDynamic_t *const device) {
    if (device == NULL) {
        return false;
    }

    device->press_trigger = g_press_trigger_lut[active_state];

    switch (active_state) {
        case eActiveState_Low: {
            device->release_trigger = eExtiTrigger_Rising;
        } break;
        case eActiveState_High: {
            device->release_trigger = eExtiTrigger_Falling;
        } break;
        case eActiveState_Both: {
            device->release_trigger = g_press_trigger_lut[active_state];
        } break;
        default: {
            return false;
        }
    }

    return true;
}

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

bool IO_API_Init(const eIo_t device, EventGroupHandle_t event_group) {
    if (!IO_Config_IsCorrectIo(device)) {
        return false;
    }

    if (eIoDeviceState_Default != g_dynamic_io_lut[device].state) {
        return true;
    }

    if (NULL == event_group) {
        return false;
    }

    if (eIoState_Started == g_io_state) {
        return false;
    }

    const sIoDesc_t *desc = IO_Config_GetIoDesc(device);

    if (NULL == desc) {
        return false;
    }

    g_static_io_desc_lut[device] = *desc;

    if (!GPIO_Driver_InitAllPins()) {
        return false;
    }

    if (g_static_io_desc_lut[device].is_exti) {
        if (!Exti_Driver_InitDevice(device, &IO_API_ExtiTriggered, &g_dynamic_io_lut[device])) {
            return false;
        }
    } else {
        g_has_polled_io = true;
    }

    if (!IO_API_ConfigureTriggers(g_static_io_desc_lut[device].active_state, &g_dynamic_io_lut[device])) {
        return false;
    }

    if (g_static_io_desc_lut[device].is_debounce_enable) {
        g_dynamic_io_lut[device].debounce_timer = xTimerCreateStatic(g_static_io_desc_lut[device].debounce_timer_name, DEFAULT_TIMER_TICKS, pdFALSE, &g_dynamic_io_lut[device], IO_API_DebounceTimerCallback, &g_dynamic_io_lut[device].debounce_timer_buffer);

        if (NULL == g_dynamic_io_lut[device].debounce_timer) {
            return false;
        }
    }

    if (NULL == g_dynamic_io_lut[device].mutex) {
        g_dynamic_io_lut[device].mutex = xSemaphoreCreateRecursiveMutexStatic(&g_dynamic_io_lut[device].mutex_buffer);

        if (NULL == g_dynamic_io_lut[device].mutex) {
            return false;
        }
    }

    g_dynamic_io_lut[device].device = device;
    g_dynamic_io_lut[device].event_data.device = device;
    g_dynamic_io_lut[device].state = eIoDeviceState_Init;
    g_dynamic_io_lut[device].event = event_group;

    if (eIoState_Default == g_io_state) {
        g_io_state = eIoState_Init;
    }

    return true;
}

bool IO_API_Start(void) {
    if (eIoState_Default == g_io_state) {
        return false;
    }

    if (eIoState_Started == g_io_state) {
        return true;
    }

    if ((NULL == g_io_thread) && g_has_polled_io) {
        g_io_thread = xTaskCreateStatic(IO_API_PollThread, g_io_thread_attributes.name, STACK_SIZE_IN_BYTES(g_io_thread_attributes.stack_size), NULL, g_io_thread_attributes.priority, g_io_thread_stack, &g_io_thread_buffer);

        if (NULL == g_io_thread) {
            return false;
        }
    }

    for (eIo_t device = eIo_First; device < eIo_Last; device++) {
        if (g_static_io_desc_lut[device].is_exti) {
            if (!Exti_Driver_EnableIt(device)) {
                return false;
            }
        }

        g_dynamic_io_lut[device].state = eIoDeviceState_Init;
    }

    g_io_state = eIoState_Started;

    return true;
}

bool IO_API_Stop(void) {
    if (eIoState_Default == g_io_state) {
        return false;
    } else if (eIoState_Init == g_io_state) {
        return true;
    }

    for (eIo_t device = eIo_First; device < eIo_Last; device++) {
        if (g_static_io_desc_lut[device].is_exti) {
            Exti_Driver_DisableIt(device);
        }

        g_dynamic_io_lut[device].state = eIoDeviceState_Init;
    }

    if (g_io_thread != NULL) {
        vTaskDelete(g_io_thread);
        g_io_thread = NULL;
    }

    g_io_state = eIoState_Init;

    return true;
}

bool IO_API_ReadPinState(const eIo_t device, bool *pin_state) {
    if (!IO_Config_IsCorrectIo(device)) {
        return false;
    }

    if (NULL == pin_state) {
        return false;
    }

    if (eIoDeviceState_Default == g_dynamic_io_lut[device].state) {
        return false;
    }

    return (GPIO_Driver_ReadPin(g_static_io_desc_lut[device].gpio_pin, pin_state));
}

sIoEvent_t *IO_API_GetEventData(const eIo_t device) {
    if (!IO_Config_IsCorrectIo(device)) {
        return NULL;
    }

    return &g_dynamic_io_lut[device].event_data;
}

#endif /* ENABLE_IO */
