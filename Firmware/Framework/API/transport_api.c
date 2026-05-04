/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "transport_api.h"

#if defined(ENABLE_TRANSPORT)
#include "debug_api.h"
#include "uart_api.h"

#if defined(ENABLE_UDP)
#include "udp_api.h"
#endif /* ENABLE_UDP */

#include "wifi_api.h"
#include "uart_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

#define TRANSPORT_API_MUTEX_TIMEOUT 0U

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

typedef enum eTransportState {
    eTransportState_First = 0,
    eTransportState_Default = eTransportState_First,
    eTransportState_Initialized,
    eTransportState_Last
} eTransportState_t;

typedef struct sTransportApiDynamic {
    eTransportState_t state;
    eTransportMode_t current_mode;
    SemaphoreHandle_t mutex;
    StaticSemaphore_t mutex_buffer;
} sTransportApiDynamic_t;

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

#if defined(DEBUG_TRANSPORT_API)
CREATE_MODULE_NAME(TRANSPORT_API)
#else
CREATE_MODULE_NAME_EMPTY
#endif /* DEBUG_TRANSPORT_API */

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

sTransportApiDynamic_t g_transport_dynamic = {0};

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

bool Transport_API_Init(void) {
    if (eTransportState_Initialized == g_transport_dynamic.state) {
        return true;
    }

    g_transport_dynamic.mutex = xSemaphoreCreateRecursiveMutexStatic(&g_transport_dynamic.mutex_buffer);

    if (NULL == g_transport_dynamic.mutex) {
        return false;
    }

    g_transport_dynamic.current_mode = eTransport_UART;
    g_transport_dynamic.state = eTransportState_Initialized;

    return true;
}

bool Transport_API_SetMode(const eTransportMode_t mode) {
    if (eTransportState_Initialized != g_transport_dynamic.state) {
        return false;
    }

    if ((eTransport_First > mode) || (eTransport_Last <= mode)) {
        return false;
    }

    if (pdTRUE != xSemaphoreTakeRecursive(g_transport_dynamic.mutex, TRANSPORT_API_MUTEX_TIMEOUT)) {
        return false;
    }

    if ((eTransport_WiFi_UDP == mode) && (eWifiState_Ready != Wifi_API_GetStatus())) {
        xSemaphoreGiveRecursive(g_transport_dynamic.mutex);

        TRACE_WRN("SetMode: Refused switch to WiFi_UDP; Wi-Fi not connected\n");

        return false;
    }

    g_transport_dynamic.current_mode = mode;

    xSemaphoreGiveRecursive(g_transport_dynamic.mutex);

    return true;
}

eTransportMode_t Transport_API_GetMode(void) {
    return g_transport_dynamic.current_mode;
}

bool Transport_API_Send(const sMessage_t message, const uint32_t timeout) {
    if (eTransportState_Initialized != g_transport_dynamic.state) {
        return false;
    }

    if (pdTRUE != xSemaphoreTakeRecursive(g_transport_dynamic.mutex, TRANSPORT_API_MUTEX_TIMEOUT)) {
        return false;
    }

    eTransportMode_t mode = g_transport_dynamic.current_mode;

    xSemaphoreGiveRecursive(g_transport_dynamic.mutex);

    switch (mode) {
        case eTransport_UART: {
            return UART_API_Send(TRANSPORT_API_TARGET_UART, message, timeout);
        }
        case eTransport_WiFi_UDP: {
            return UDP_API_Send(TRANSPORT_API_TARGET_UDP, message, timeout);
        }
        default: {
            return false;
        }
    }
}

bool Transport_API_Receive(sMessage_t *message, const uint32_t timeout) {
    if (eTransportState_Initialized != g_transport_dynamic.state) {
        return false;
    }

    if (NULL == message) {
        return false;
    }

    if (pdTRUE != xSemaphoreTakeRecursive(g_transport_dynamic.mutex, TRANSPORT_API_MUTEX_TIMEOUT)) {
        return false;
    }

    eTransportMode_t mode = g_transport_dynamic.current_mode;

    xSemaphoreGiveRecursive(g_transport_dynamic.mutex);

    switch (mode) {
        case eTransport_UART: {
            return UART_API_Receive(TRANSPORT_API_TARGET_UART, message, timeout);
        }
        case eTransport_WiFi_UDP: {
            return UDP_API_Receive(TRANSPORT_API_TARGET_UDP, message, timeout);
        }
        default: {
            return false;
        }
    }
}

#endif /* ENABLE_TRANSPORT */
