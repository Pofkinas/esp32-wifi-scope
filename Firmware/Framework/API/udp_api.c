/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "udp_api.h"

#if defined(ENABLE_UDP)
#include <stdio.h>
#include <string.h>

#if defined(ENABLE_WIFI)
#include "wifi_api.h"
#endif /* ENABLE_WIFI */

#include "debug_api.h"
#include "heap_api.h"
#include "udp_driver.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos_types.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

typedef enum eUdpState {
    eState_First = 0,
    eState_Default = eState_First,
    eState_Active,
    eState_Last
} eUdpState_t;

typedef struct sUdpApiDynamic {
    eUdpState_t current_state;
    sUdpDesc_t *socket_desc;
    char target_ip[IPV4_STRING_LEN];
    uint16_t target_port;
    SemaphoreHandle_t mutex;
    StaticSemaphore_t mutex_buffer;
    QueueHandle_t message_queue;
    sMessage_t message_queue_storage[UDP_API_MESSAGE_QUEUE_CAPACITY];
    StaticQueue_t message_queue_buffer;
    sUdpApiConst_t const *api_const;
} sUdpApiDynamic_t;

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

#if defined(DEBUG_UDP_API)
CREATE_MODULE_NAME(UDP_API)
#else
CREATE_MODULE_NAME_EMPTY
#endif /* DEBUG_UDP_API */

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

static TaskHandle_t g_fsm_thread = NULL;
static StackType_t g_fsm_thread_stack[STACK_SIZE_IN_BYTES(UDP_API_THREAD_STACK_SIZE)] = {0};
static StaticTask_t g_fsm_thread_buffer = {0};

static sUdpApiDynamic_t g_dynamic_udp_lut[eUdp_Last] = {0};

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

static void UDP_API_FsmThread(void *arg);

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

static void UDP_API_FsmThread(void *arg) {
    while (1) {
        uint32_t ready_mask = UDP_Driver_WaitForData(portMAX_DELAY);

        if (0 == ready_mask) {
            continue;
        }

        for (eUdp_t udp = eUdp_First; udp < eUdp_Last; udp++) {
            if (eState_Active != g_dynamic_udp_lut[udp].current_state) {
                continue;
            }

            if (0 == (ready_mask & (1U << (uint32_t) udp))) {
                continue;
            }

            if (NULL == g_dynamic_udp_lut[udp].api_const) {
                continue;
            }

            size_t rx_size = g_dynamic_udp_lut[udp].api_const->rx_buffer_capacity;
            uint8_t *rx_buffer = (uint8_t *) Heap_API_Calloc(rx_size, sizeof(uint8_t));

            if (NULL == rx_buffer) {
                TRACE_WRN("FsmThread: Failed to allocate RX buffer for UDP [%d]\n", udp);

                continue;
            }

            if (!UDP_Driver_Receive(udp, rx_buffer, &rx_size)) {
                Heap_API_Free(rx_buffer);

                continue;
            }

            sMessage_t message = {.data = (char *) rx_buffer, .size = rx_size};

            if (pdTRUE != xQueueSend(g_dynamic_udp_lut[udp].message_queue, &message, UDP_API_MESSAGE_QUEUE_PUT_TIMEOUT)) {
                TRACE_WRN("FsmThread: Queue full for UDP [%d]\n", udp);
                Heap_API_Free(rx_buffer);
            }
        }
    }
}

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

bool UDP_API_Init(const eUdp_t udp) {
    if (!UDP_Config_IsCorrectUdp(udp)) {
        return false;
    }

#if defined(ENABLE_WIFI)
    if (!Wifi_API_Init()) {
        return false;
    }
#endif /* ENABLE_WIFI */

    if (eState_Default != g_dynamic_udp_lut[udp].current_state) {
        return true;
    }

    const sUdpDesc_t *socket_desc = UDP_Config_GetUdpDesc(udp);
    const sUdpApiConst_t *api_const = UDP_Config_GetUdpApiConst(udp);

    if ((NULL == socket_desc) || (NULL == api_const)) {
        return false;
    }

    if (!UDP_Driver_Init(udp, socket_desc->local_port)) {
        return false;
    }

    g_dynamic_udp_lut[udp].mutex = xSemaphoreCreateRecursiveMutexStatic(&g_dynamic_udp_lut[udp].mutex_buffer);

    if (NULL == g_dynamic_udp_lut[udp].mutex) {
        return false;
    }

    g_dynamic_udp_lut[udp].message_queue = xQueueCreateStatic(UDP_API_MESSAGE_QUEUE_CAPACITY, sizeof(sMessage_t), (uint8_t *) &g_dynamic_udp_lut[udp].message_queue_storage[0], &g_dynamic_udp_lut[udp].message_queue_buffer);

    if (NULL == g_dynamic_udp_lut[udp].message_queue) {
        return false;
    }

    if (NULL == g_fsm_thread) {
        g_fsm_thread = xTaskCreateStatic(UDP_API_FsmThread, g_udp_fsm_thread_attributes.name, STACK_SIZE_IN_BYTES(g_udp_fsm_thread_attributes.stack_size), NULL, g_udp_fsm_thread_attributes.priority, g_fsm_thread_stack, &g_fsm_thread_buffer);
    }

    if (NULL == g_fsm_thread) {
        return false;
    }

    snprintf(g_dynamic_udp_lut[udp].target_ip, IPV4_STRING_LEN, "%u.%u.%u.%u", socket_desc->default_target_ip->octets[0], socket_desc->default_target_ip->octets[1], socket_desc->default_target_ip->octets[2], socket_desc->default_target_ip->octets[3]);
    g_dynamic_udp_lut[udp].target_port = socket_desc->default_target_port;
    g_dynamic_udp_lut[udp].api_const = api_const;
    g_dynamic_udp_lut[udp].current_state = eState_Active;

    return true;
}

bool UDP_API_SetTarget(const eUdp_t udp, const uIPv4Address_t address, const uint16_t port) {
    if (!UDP_Config_IsCorrectUdp(udp)) {
        return false;
    }

    if (eState_Default == g_dynamic_udp_lut[udp].current_state) {
        return false;
    }

    if (pdTRUE != xSemaphoreTakeRecursive(g_dynamic_udp_lut[udp].mutex, UDP_API_MUTEX_TIMEOUT)) {
        return false;
    }

    snprintf(g_dynamic_udp_lut[udp].target_ip, IPV4_STRING_LEN, "%u.%u.%u.%u", address.octets[0], address.octets[1], address.octets[2], address.octets[3]);
    g_dynamic_udp_lut[udp].target_port = port;

    xSemaphoreGiveRecursive(g_dynamic_udp_lut[udp].mutex);

    return true;
}

bool UDP_API_Send(const eUdp_t udp, const sMessage_t message, const uint32_t timeout) {
    if (!UDP_Config_IsCorrectUdp(udp)) {
        return false;
    }

    if (eState_Default == g_dynamic_udp_lut[udp].current_state) {
        return false;
    }

    if (pdTRUE != xSemaphoreTakeRecursive(g_dynamic_udp_lut[udp].mutex, timeout)) {
        return false;
    }

    if (!UDP_Driver_Send(udp, g_dynamic_udp_lut[udp].target_ip, g_dynamic_udp_lut[udp].target_port, (const uint8_t *) message.data, message.size)) {
        xSemaphoreGiveRecursive(g_dynamic_udp_lut[udp].mutex);

        return false;
    }

    xSemaphoreGiveRecursive(g_dynamic_udp_lut[udp].mutex);

    return true;
}

bool UDP_API_Receive(const eUdp_t udp, sMessage_t *message, const uint32_t timeout) {
    if (!UDP_Config_IsCorrectUdp(udp)) {
        return false;
    }

    if (NULL == message) {
        return false;
    }

    if (eState_Default == g_dynamic_udp_lut[udp].current_state) {
        return false;
    }

    if (pdTRUE != xQueueReceive(g_dynamic_udp_lut[udp].message_queue, message, timeout)) {
        return false;
    }

    return true;
}

#endif /* ENABLE_UDP */
