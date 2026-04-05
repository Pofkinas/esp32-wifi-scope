/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "capture_api.h"

#if defined(ENABLE_CAPTURE)
#include <stdio.h>
#include "heap_api.h"
#include "debug_api.h"
#include "adc_driver.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos_types.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

#define CAPTURE_MUTEX_TIMEOUT 0

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

typedef enum eCaptureState {
    eCaptureState_First = 0,
    eCaptureState_Default = eCaptureState_First,
    eCaptureState_Initialized,
    eCaptureState_Started,
    eCaptureState_Stopped,
    eCaptureState_Last
} eCaptureState_t;

typedef struct sCaptureMessage {
    eCaptureType_t type;
    void *data;
    void *event;
} sCaptureMessage_t;

typedef struct sCaptureDynamic {
    eCaptureState_t state;
    sCaptureDesc_t desc;
    sRingBuffer_Handle_t buffer;
    size_t data_captured;
    size_t data_to_capture;
    SemaphoreHandle_t mutex;
    StaticSemaphore_t mutex_buffer;
    void *processed_data;
} sCaptureDynamic_t;

typedef struct sCaptureAdc {
    eCaptureDevice_t device_list[eCaptureDevice_Last];
    size_t list_size;
} sCaptureAdc_t;

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

#if defined(DEBUG_CAPTURE_API)
CREATE_MODULE_NAME(CAPTURE_API)
#else
CREATE_MODULE_NAME_EMPTY
#endif /* DEBUG_CAPTURE_API */

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

static TaskHandle_t g_capture_thread = NULL;
static StackType_t g_capture_thread_stack[STACK_SIZE_IN_BYTES(CAPTURE_API_THREAD_STACK_SIZE)] = {0};
static StaticTask_t g_capture_thread_buffer = {0};

static QueueHandle_t g_capture_queue = NULL;
static sCaptureMessage_t g_capture_queue_storage[CAPTURE_MESSAGE_QUEUE_LENGTH];
static StaticQueue_t g_capture_queue_buffer;

static sCaptureDynamic_t g_capture_dynamic[eCaptureDevice_Last] = {0};
static sCaptureAdc_t g_capture_adc[eAdc_Last] = {0};

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

static bool Capture_API_IsCorrectDevice(const eCaptureDevice_t device);
static bool Capture_API_IsCorrectType(const eCaptureType_t type);
static void Capture_API_Thread(void *pvParameters);

static void Capture_API_AdcCallback(const eAdc_t adc, const eAdcEvent_t event, bool *yield_from_isr);
static bool Capture_API_HandleAdcEvent(const eAdc_t adc, const eAdcEvent_t event);

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

static bool Capture_API_IsCorrectDevice(const eCaptureDevice_t device) {
    return (device >= eCaptureDevice_First) && (device < eCaptureDevice_Last);
}

static bool Capture_API_IsCorrectType(const eCaptureType_t type) {
    return (type >= eCaptureType_First) && (type < eCaptureType_Last);
}

static void Capture_API_Thread(void *pvParameters) {
    sCaptureMessage_t message = {0};

    while (1) {
        if (pdTRUE != xQueueReceive(g_capture_queue, &message, portMAX_DELAY)) {
            TRACE_ERR("Thread: Failed to get message from queue\n");

            continue;
        }

        switch (message.type) {
            case eCaptureType_Adc: {
                Capture_API_HandleAdcEvent((eAdc_t) message.data, (eAdcEvent_t) message.event);
            } break;
            default: {
                TRACE_WRN("Thread: Received message with unsupported type [%d]\n", message.type);

                continue;
            } break;
        }
    }
}

static void Capture_API_AdcCallback(const eAdc_t adc, const eAdcEvent_t event, bool *yield_from_isr) {
    if (!ADC_Config_IsCorrectAdc(adc)) {
        return;
    }

    sCaptureMessage_t message = {0};

    message.type = eCaptureType_Adc;
    message.data = (void *) adc;
    message.event = (void *) event;

    if (NULL != g_capture_thread) {
        xQueueSendFromISR(g_capture_queue, &message, (BaseType_t *) yield_from_isr);
    }

    return;
}

static bool Capture_API_HandleAdcEvent(const eAdc_t adc, const eAdcEvent_t event) {
    if (!ADC_Config_IsCorrectAdc(adc)) {
        return false;
    }

    switch (event) {
        case eAdcEvent_CaptureDone: {
            if (!ADC_Driver_Read(adc)) {
                TRACE_WRN("Thread: Failed to read data from ADC [%d]\n", adc);

                return false;
            }

            if (g_capture_adc[adc].list_size == 0) {
                TRACE_WRN("Thread: No capture ADC [%d] devices\n", adc);

                return false;
            }

            for (size_t adc_item = 0; adc_item < g_capture_adc[adc].list_size; adc_item++) {
                eCaptureDevice_t device = g_capture_adc[adc].device_list[adc_item];

                if (eCaptureState_Started != g_capture_dynamic[device].state) {
                    continue;
                }

                uint32_t *data = NULL;
                size_t data_size = 0;
                sAdcConfig_t *adc_config = (sAdcConfig_t *) g_capture_dynamic[device].desc.device_data;

                if (!ADC_Driver_GetChannelData(adc_config->adc, adc_config->channel, &data, &data_size)) {
                    TRACE_WRN("Thread: Failed to get channel data for device [%d]\n", device);

                    continue;
                }

                if ((NULL == data) || (data_size == 0)) {
                    TRACE_WRN("Thread: No data captured for device [%d]\n", device);

                    continue;
                }

                bool is_data_processed = (NULL != g_capture_dynamic[device].desc.process_callback);

                if (is_data_processed) {
                    if (!g_capture_dynamic[device].desc.process_callback(data, &g_capture_dynamic[device].processed_data, data_size, g_capture_dynamic[device].desc.process_context)) {
                        TRACE_WRN("Thread: Failed to process data for device [%d]\n", device);

                        continue;
                    }
                }

                void *push_data = is_data_processed ? g_capture_dynamic[device].processed_data : data;

                if (!Ring_Buffer_PushBulk(g_capture_dynamic[device].buffer, push_data, data_size)) {
                    TRACE_WRN("Thread: Failed to push data to buffer for device [%d]\n", device);
                }

                g_capture_dynamic[device].data_captured += data_size;

                if (g_capture_dynamic[device].data_to_capture == 0) {
                    continue;
                }

                if (g_capture_dynamic[device].data_captured < g_capture_dynamic[device].data_to_capture) {
                    continue;
                }

                if (NULL != g_capture_dynamic[device].desc.notify_callback) {
                    g_capture_dynamic[device].desc.notify_callback(device, eCaptureEvent_CaptureDone);
                    g_capture_dynamic[device].data_captured = 0;
                }
            }
        } break;
        case eAdcEvent_Overflow: {
            ADC_Driver_ClearBuffer(adc);

            for (size_t adc_item = 0; adc_item < g_capture_adc[adc].list_size; adc_item++) {
                eCaptureDevice_t device = g_capture_adc[adc].device_list[adc_item];

                if (eCaptureState_Started != g_capture_dynamic[device].state) {
                    continue;
                }

                if (NULL != g_capture_dynamic[device].desc.notify_callback) {
                    g_capture_dynamic[device].desc.notify_callback(device, eCaptureEvent_Overflow);
                }
            }

            TRACE_WRN("Thread: Overflow event for ADC [%d]\n", adc);
        } break;
        default: {
            TRACE_WRN("Thread: Unknown event [%d] for ADC [%d]\n", event, adc);
        }
    }

    return true;
}

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

bool Capture_API_Init(const eCaptureDevice_t device, const sCaptureDesc_t *desc, const sRingBuffer_Handle_t buffer) {
    if (!Capture_API_IsCorrectDevice(device)) {
        return false;
    }

    if (eCaptureState_Default != g_capture_dynamic[device].state) {
        return true;
    }

    if ((NULL == desc) || (NULL == buffer)) {
        return false;
    }

    if ((!Capture_API_IsCorrectType(desc->type)) || (NULL == desc->device_data) || (0 == desc->data_size) || (NULL == desc->process_callback)) {
        return false;
    }

    memcpy(&g_capture_dynamic[device].desc, desc, sizeof(sCaptureDesc_t));

    switch (desc->type) {
        case eCaptureType_Adc: {
            sAdcConfig_t *adc_config = (sAdcConfig_t *) desc->device_data;

            if (!ADC_Driver_InitDevice(adc_config->adc, Capture_API_AdcCallback)) {
                return false;
            }

            g_capture_adc[adc_config->adc].device_list[g_capture_adc[adc_config->adc].list_size] = device;
            g_capture_adc[adc_config->adc].list_size++;
        } break;
        default: {
            TRACE_ERR("Init: Unsupported capture type [%d] for device [%d]\n", desc->type, device);

            return false;
        } break;
    }

    if (!Heap_API_Init()) {
        TRACE_ERR("Init: Heap_API_Init failed for device [%d]\n", device);

        return false;
    }

    g_capture_dynamic[device].processed_data = Heap_API_Malloc(desc->data_size);

    if (NULL == g_capture_dynamic[device].processed_data) {
        TRACE_ERR("Init: Failed to alloc for device [%d]\n", device);

        return false;
    }

    g_capture_dynamic[device].mutex = xSemaphoreCreateRecursiveMutexStatic(&g_capture_dynamic[device].mutex_buffer);

    if (NULL == g_capture_dynamic[device].mutex) {
        return false;
    }

    if (NULL == g_capture_queue) {
        g_capture_queue = xQueueCreateStatic(CAPTURE_MESSAGE_QUEUE_LENGTH, sizeof(sCaptureMessage_t), (uint8_t *) g_capture_queue_storage, &g_capture_queue_buffer);

        if (NULL == g_capture_queue) {
            return false;
        }
    }

    if (NULL == g_capture_thread) {
        g_capture_thread = xTaskCreateStatic(Capture_API_Thread, g_capture_thread_attributes.name, STACK_SIZE_IN_BYTES(g_capture_thread_attributes.stack_size), NULL, g_capture_thread_attributes.priority, g_capture_thread_stack, &g_capture_thread_buffer);

        if (NULL == g_capture_thread) {
            return false;
        }
    }

    g_capture_dynamic[device].buffer = buffer;
    g_capture_dynamic[device].state = eCaptureState_Initialized;

    return true;
}

bool Capture_API_Start(const eCaptureDevice_t device, const size_t data_to_capture) {
    if (!Capture_API_IsCorrectDevice(device)) {
        return false;
    }

    if (eCaptureState_Initialized != g_capture_dynamic[device].state) {
        return false;
    }

    switch (g_capture_dynamic[device].desc.type) {
        case eCaptureType_Adc: {
            sAdcConfig_t *adc_config = (sAdcConfig_t *) g_capture_dynamic[device].desc.device_data;

            if (!ADC_Driver_Start(adc_config->adc)) {
                return false;
            }
        } break;
        default: {
            TRACE_ERR("Start: Unsupported capture type [%d] for device [%d]\n", g_capture_dynamic[device].desc.type, device);

            return false;
        } break;
    }

    if (pdTRUE != xSemaphoreTakeRecursive(g_capture_dynamic[device].mutex, CAPTURE_MUTEX_TIMEOUT)) {
        TRACE_ERR("Start: Failed to acquire capture mutex for device [%d]\n", device);

        return false;
    }

    g_capture_dynamic[device].data_captured = 0;
    g_capture_dynamic[device].data_to_capture = data_to_capture;
    g_capture_dynamic[device].state = eCaptureState_Started;

    xSemaphoreGiveRecursive(g_capture_dynamic[device].mutex);

    return true;
}

bool Capture_API_Stop(const eCaptureDevice_t device) {
    if (!Capture_API_IsCorrectDevice(device)) {
        return false;
    }

    if (eCaptureState_Started != g_capture_dynamic[device].state) {
        return false;
    }

    switch (g_capture_dynamic[device].desc.type) {
        case eCaptureType_Adc: {
            sAdcConfig_t *adc_config = (sAdcConfig_t *) g_capture_dynamic[device].desc.device_data;

            if (!ADC_Driver_Stop(adc_config->adc)) {
                return false;
            }
        } break;
        default: {
            TRACE_ERR("Stop: Unsupported capture type [%d] for device [%d]\n", g_capture_dynamic[device].desc.type, device);

            return false;
        } break;
    }

    if (pdTRUE != xSemaphoreTakeRecursive(g_capture_dynamic[device].mutex, CAPTURE_MUTEX_TIMEOUT)) {
        TRACE_ERR("Stop: Failed to acquire capture mutex for device [%d]\n", device);

        return false;
    }

    g_capture_dynamic[device].state = eCaptureState_Initialized;

    xSemaphoreGiveRecursive(g_capture_dynamic[device].mutex);

    return true;
}

bool Capture_API_UpdateTarget(const eCaptureDevice_t device, const size_t data_to_capture) {
    if (!Capture_API_IsCorrectDevice(device)) {
        return false;
    }

    if (eCaptureState_Started != g_capture_dynamic[device].state) {
        return false;
    }

    if (pdTRUE != xSemaphoreTakeRecursive(g_capture_dynamic[device].mutex, CAPTURE_MUTEX_TIMEOUT)) {
        TRACE_ERR("UpdateTarget: Failed to acquire capture mutex for ADC [%d]\n", device);

        return false;
    }

    g_capture_dynamic[device].data_to_capture = data_to_capture;

    xSemaphoreGiveRecursive(g_capture_dynamic[device].mutex);

    return true;
}

bool Capture_API_ClearBuffer(const eCaptureDevice_t device) {
    if (!Capture_API_IsCorrectDevice(device)) {
        return false;
    }

    if (eCaptureState_Default == g_capture_dynamic[device].state) {
        return false;
    }

    switch (g_capture_dynamic[device].desc.type) {
        case eCaptureType_Adc: {
            sAdcConfig_t *adc_config = (sAdcConfig_t *) g_capture_dynamic[device].desc.device_data;

            if (!ADC_Driver_ClearBuffer(adc_config->adc)) {
                return false;
            }
        } break;
        default: {
            TRACE_ERR("ClearBuffer: Unsupported capture type [%d] for device [%d]\n", g_capture_dynamic[device].desc.type, device);

            return false;
        } break;
    }

    if (!Ring_Buffer_Clear(g_capture_dynamic[device].buffer)) {
        return false;
    }

    return true;
}

#endif /* ENABLE_CAPTURE */
