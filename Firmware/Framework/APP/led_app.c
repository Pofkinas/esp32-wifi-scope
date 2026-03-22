/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "led_app.h"

#if defined(ENABLE_LED) || defined(ENABLE_PWM_LED)

#include <stddef.h>
#include "cli_app.h"
#include "debug_api.h"
#include "heap_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos_typedefs.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

#if defined(DEBUG_LED_APP)
CREATE_MODULE_NAME (LED_APP)
#else
CREATE_MODULE_NAME_EMPTY
#endif /* DEBUG_LED_APP */

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

static sLedCommandDesc_t g_received_task = {.task = eLedTask_Last, .data = NULL};
static bool g_is_initialized = false;

static TaskHandle_t g_led_thread = NULL;
static StackType_t g_led_thread_stack[STACK_SIZE_IN_BYTES(LED_THREAD_STACK_SIZE)] = {0};
static StaticTask_t g_led_thread_buffer = {0};

static QueueHandle_t g_led_message_queue = NULL;
static sLedCommandDesc_t g_led_message_queue_storage[LED_MESSAGE_QUEUE_CAPACITY] = {0};
static StaticQueue_t g_led_message_queue_buffer = {0};

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/
 
static void LED_APP_Thread (void *pvParameters);

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

static void LED_APP_Thread (void *pvParameters) {
    while (1) {
        // TODO: redefine pdMS_TO_TICKS with guard 0/1, and compilation error if arg time is less than configTICK_RATE_HZ
        // if (pdTRUE != xQueueReceive(g_led_message_queue, &g_received_task, pdMS_TO_TICKS(LED_MESSAGE_QUEUE_TIMEOUT))) {
        if (pdTRUE != xQueueReceive(g_led_message_queue, &g_received_task, LED_MESSAGE_QUEUE_TIMEOUT)) {
            continue;
        }

        if (NULL == g_received_task.data) {
            TRACE_ERR("No arguments\n");
        }
        
        switch (g_received_task.task) {
            #if defined(ENABLE_LED)
            case eLedTask_Set: {
                sLedCommon_t *arguments = (sLedCommon_t*) g_received_task.data;

                if (NULL == arguments) {
                    TRACE_ERR("No arguments\n");

                    Heap_API_Free(arguments);

                    break;
                }

                if (!LED_Config_IsCorrectLed(arguments->led)) {
                    TRACE_ERR("Invalid Led\n");

                    Heap_API_Free(arguments);

                    break;
                }
                
                if (!LED_API_TurnOn(arguments->led)) {
                    TRACE_ERR("LED Turn On Failed\n");

                    Heap_API_Free(arguments);

                    break;
                }

                TRACE_INFO("Led [%d] Set\n", arguments->led);

                Heap_API_Free(arguments);    
            } break;
            case eLedTask_Reset: {
                sLedCommon_t *arguments = (sLedCommon_t*) g_received_task.data;

                if (NULL == arguments) {
                    TRACE_ERR("No arguments\n");

                    Heap_API_Free(arguments);

                    break;
                }

                if (!LED_Config_IsCorrectLed(arguments->led)) {
                    TRACE_ERR("Invalid Led\n");

                    Heap_API_Free(arguments);

                    break;
                }

                if (!LED_API_TurnOff(arguments->led)) {
                    TRACE_ERR("LED Turn Off Failed\n");

                    Heap_API_Free(arguments);

                    break;
                }

                TRACE_INFO("Led [%d] Reset\n", arguments->led);

                Heap_API_Free(arguments);
            } break;
            case eLedTask_Toggle: {
                sLedCommon_t *arguments = (sLedCommon_t*) g_received_task.data;

                if (NULL == arguments) {
                    TRACE_ERR("No arguments\n");

                    Heap_API_Free(arguments);

                    break;
                }

                if (!LED_Config_IsCorrectLed(arguments->led)) {
                    TRACE_ERR("Invalid Led\n");

                    Heap_API_Free(arguments);

                    break;
                }

                if (!LED_API_Toggle(arguments->led)) {
                    TRACE_ERR("LED Toggle Failed\n");

                    Heap_API_Free(arguments);

                    break;
                }

                TRACE_INFO("Led [%d] Toggle\n", arguments->led);

                Heap_API_Free(arguments);
            } break;
            case eLedTask_Blink: {
                sLedBlink_t *arguments = (sLedBlink_t*) g_received_task.data;

                if (NULL == arguments) {
                    TRACE_ERR("No arguments\n");

                    Heap_API_Free(arguments);

                    break;
                }

                if (!LED_Config_IsCorrectLed(arguments->led)) {
                    TRACE_ERR("Invalid Led\n");

                    Heap_API_Free(arguments);

                    break;
                }

                if (!LED_API_IsCorrectBlinkTime(arguments->blink_time)) {
                    TRACE_ERR("Invalid blink time\n");

                    Heap_API_Free(arguments);

                    break;
                }

                if (!LED_API_IsCorrectBlinkFrequency(arguments->blink_frequency)) {
                    TRACE_ERR("Invalid blink frequency\n");

                    Heap_API_Free(arguments);

                    break;
                }

                if (!LED_API_Blink(arguments->led, arguments->blink_time, arguments->blink_frequency)) {
                    TRACE_ERR("LED Blink Failed\n");

                    Heap_API_Free(arguments);

                    break;
                }

                TRACE_INFO("Led [%d] Blink %u s, @ %u Hz\n", arguments->led, arguments->blink_time, arguments->blink_frequency);

                Heap_API_Free(arguments);
            } break;
            #endif /* ENABLE_LED */
            #if defined(ENABLE_PWM_LED)
            case eLedTask_Set_Brightness: {
                sLedSetBrightness_t *arguments = (sLedSetBrightness_t*) g_received_task.data;

                if (NULL == arguments) {
                    TRACE_ERR("No arguments\n");

                    Heap_API_Free(arguments);

                    break;
                }

                if (!LED_Config_IsCorrectPwmLed(arguments->led)) {
                    TRACE_ERR("Invalid Led\n");

                    Heap_API_Free(arguments);

                    break;
                }

                if (!LED_API_IsCorrectDutyCycle(arguments->led, arguments->duty_cycle)) {
                    TRACE_ERR("Invalid duty cycle\n");

                    Heap_API_Free(arguments);

                    break;
                }

                if (!LED_API_Set_Brightness(arguments->led, arguments->duty_cycle)) {
                    TRACE_ERR("LED Set Brightness Failed\n");

                    Heap_API_Free(arguments);

                    break;
                }

                TRACE_INFO("Pwm Led [%d] Brightness %u\n", arguments->led, arguments->duty_cycle);

                Heap_API_Free(arguments);
            } break;
            case eLedTask_Pulse: {
                sLedPulse_t *arguments = (sLedPulse_t*) g_received_task.data;

                if (NULL == arguments) {
                    TRACE_ERR("No arguments\n");

                    Heap_API_Free(arguments);

                    break;
                }

                if (!LED_Config_IsCorrectPwmLed(arguments->led)) {
                    TRACE_ERR("Invalid Led\n");

                    Heap_API_Free(arguments);

                    break;
                }

                if (!LED_API_IsCorrectPulseTime(arguments->pulse_time)) {
                    TRACE_ERR("Invalid pulse time\n");

                    Heap_API_Free(arguments);

                    break;
                }

                if (!LED_API_IsCorrectPulseFrequency(arguments->pulse_frequency)) {
                    TRACE_ERR("Invalid pulse frequency\n");

                    Heap_API_Free(arguments);

                    break;
                }

                if (!LED_API_Pulse(arguments->led, arguments->pulse_time, arguments->pulse_frequency)) {
                    TRACE_ERR("LED Pulse Failed\n");

                    Heap_API_Free(arguments);

                    break;
                }

                TRACE_INFO("Pwm Led [%d] Pulse %u s, @ %u Hz\n", arguments->led, arguments->pulse_time, arguments->pulse_frequency);

                Heap_API_Free(arguments);
            } break;
            #endif /* ENABLE_PWM_LED */
            default: {
                TRACE_ERR("Task not found\n");
            } break;
        }
    }
}

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

bool LED_APP_Init (void) {
    if (g_is_initialized) {
        return true;
    }

    if (!LED_API_Init()) {
        return false;
    }

    g_led_message_queue = xQueueCreateStatic(LED_MESSAGE_QUEUE_CAPACITY, sizeof(sLedCommandDesc_t), (uint8_t*) &g_led_message_queue_storage[0], &g_led_message_queue_buffer);
    
    if (NULL == g_led_message_queue) {
        return false;
    }

    g_led_thread = xTaskCreateStatic(LED_APP_Thread, g_led_thread_attributes.name, STACK_SIZE_IN_BYTES(g_led_thread_attributes.stack_size), NULL, g_led_thread_attributes.priority, g_led_thread_stack, &g_led_thread_buffer);

    if (NULL == g_led_thread) {
        return false;
    }

    g_is_initialized = true;

    return g_is_initialized;
}

bool LED_APP_AddTask (sLedCommandDesc_t *task_to_message_queue) {
    if (!g_is_initialized) {
        return false;
    }   
    
    if (NULL == task_to_message_queue) {
        return false;
    }

    if (NULL == g_led_message_queue) {
        return false;
    }

    // TODO: redefine pdMS_TO_TICKS with guard 0/1, and compilation error if arg time is less than configTICK_RATE_HZ
    //if (pdTRUE != xQueueSend(g_led_message_queue, task_to_message_queue, pdMS_TO_TICKS(LED_MESSAGE_QUEUE_PUT_TIMEOUT))) {
    if (pdTRUE != xQueueSend(g_led_message_queue, task_to_message_queue, LED_MESSAGE_QUEUE_PUT_TIMEOUT)) {
        return false;
    }

    return true;
}

#endif /* defined(ENABLE_LED) || defined(ENABLE_PWM_LED) */
