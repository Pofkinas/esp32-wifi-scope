/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "wifi_api.h"

#if defined(ENABLE_WIFI)
#include <string.h>

#include "debug_api.h"
#include "transport_api.h"
#include "wifi_driver.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

#define WIFI_API_MUTEX_TIMEOUT 0U

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

typedef struct sWifiApiDynamic {
    eWifiState_t current_state;
    SemaphoreHandle_t mutex;
    StaticSemaphore_t mutex_buffer;
    uint8_t max_retries;
    uint8_t retry_count;
    uIPv4Address_t ip_address;
} sWifiApiDynamic_t;

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

#if defined(DEBUG_WIFI_API)
CREATE_MODULE_NAME(WIFI_API)
#else
CREATE_MODULE_NAME_EMPTY
#endif /* DEBUG_WIFI_API */

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

static sWifiApiDynamic_t g_wifi_dynamic = {0};

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

static void Wifi_API_WifiEventCallback(eWifiEvent_t event_id);

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

static void Wifi_API_WifiEventCallback(eWifiEvent_t event_id) {
    if (pdTRUE != xSemaphoreTakeRecursive(g_wifi_dynamic.mutex, WIFI_API_MUTEX_TIMEOUT)) {
        return;
    }

    bool revert_transport = false;

    switch (event_id) {
        case eWifiEvent_Connected: {
            g_wifi_dynamic.current_state = eWifiState_Connected;
        } break;
        case eWifiEvent_Disconnected: {
            if (g_wifi_dynamic.retry_count < g_wifi_dynamic.max_retries) {
                g_wifi_dynamic.retry_count++;
                g_wifi_dynamic.current_state = eWifiState_AttemptingConnection;

                Wifi_Driver_Reconnect();
            } else {
                Wifi_API_Disconnect();
                revert_transport = true;
            }
        } break;
        case eWifiEvent_GotIp: {
            g_wifi_dynamic.retry_count = 0;
            g_wifi_dynamic.current_state = eWifiState_Ready;

            Wifi_Driver_GetIpv4Address(&g_wifi_dynamic.ip_address);
        } break;

        default: {
        } break;
    }

    xSemaphoreGiveRecursive(g_wifi_dynamic.mutex);

    if (eWifiEvent_GotIp == event_id) {
        TRACE_INFO("WifiEventCallback: Connected, IP %u.%u.%u.%u\n", g_wifi_dynamic.ip_address.octets[0], g_wifi_dynamic.ip_address.octets[1], g_wifi_dynamic.ip_address.octets[2], g_wifi_dynamic.ip_address.octets[3]);
    }

    if (revert_transport) {
        Transport_API_SetMode(eTransport_UART);

        TRACE_WRN("WifiEventCallback: Disconnected, reverting transport to UART\n");
    }

    return;
}

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

bool Wifi_API_Init(void) {
    if (eWifiState_Default < g_wifi_dynamic.current_state) {
        return true;
    }

    const sWifiStaDesc_t *desc = Wifi_Config_GetWifiStaDesc();

    if (NULL == desc) {
        return false;
    }

    if (!Wifi_Driver_Init(Wifi_API_WifiEventCallback)) {
        return false;
    }

    g_wifi_dynamic.mutex = xSemaphoreCreateRecursiveMutexStatic(&g_wifi_dynamic.mutex_buffer);

    if (NULL == g_wifi_dynamic.mutex) {
        return false;
    }

    if (!Wifi_Driver_Start()) {
        return false;
    }

    g_wifi_dynamic.current_state = eWifiState_Disconnected;
    g_wifi_dynamic.retry_count = 0;
    g_wifi_dynamic.max_retries = desc->max_retry;

    return true;
}

bool Wifi_API_Connect(const char *ssid, const char *password) {
    if (eWifiState_Disconnected != g_wifi_dynamic.current_state) {
        TRACE_WRN("Connect: Refused connect attempt; Wi-Fi state: %d\n", g_wifi_dynamic.current_state);

        return false;
    }

    if (pdTRUE != xSemaphoreTakeRecursive(g_wifi_dynamic.mutex, WIFI_API_MUTEX_TIMEOUT)) {
        return false;
    }

    g_wifi_dynamic.retry_count = 0;
    g_wifi_dynamic.current_state = eWifiState_AttemptingConnection;

    xSemaphoreGiveRecursive(g_wifi_dynamic.mutex);

    if (!Wifi_Driver_Connect(ssid, password)) {
        TRACE_ERR("Connect: Wifi_Driver_Connect failed\n");

        if (pdTRUE == xSemaphoreTakeRecursive(g_wifi_dynamic.mutex, WIFI_API_MUTEX_TIMEOUT)) {
            g_wifi_dynamic.current_state = eWifiState_Disconnected;

            xSemaphoreGiveRecursive(g_wifi_dynamic.mutex);
        }

        return false;
    }

    return true;
}

bool Wifi_API_Disconnect(void) {
    if (eWifiState_Disconnected > g_wifi_dynamic.current_state) {
        return false;
    }

    if (eWifiState_Disconnected == g_wifi_dynamic.current_state) {
        return true;
    }

    if (!Wifi_Driver_Disconnect()) {
        return false;
    }

    if (pdTRUE != xSemaphoreTakeRecursive(g_wifi_dynamic.mutex, WIFI_API_MUTEX_TIMEOUT)) {
        return false;
    }

    g_wifi_dynamic.current_state = eWifiState_Disconnected;
    memset(&g_wifi_dynamic.ip_address, 0, sizeof(g_wifi_dynamic.ip_address));

    xSemaphoreGiveRecursive(g_wifi_dynamic.mutex);

    return true;
}

eWifiState_t Wifi_API_GetStatus(void) {
    return g_wifi_dynamic.current_state;
}

bool Wifi_API_GetIpv4Address(uIPv4Address_t *address) {
    if (NULL == address) {
        return false;
    }

    if (eWifiState_Default == g_wifi_dynamic.current_state) {
        return false;
    }

    if (pdTRUE != xSemaphoreTakeRecursive(g_wifi_dynamic.mutex, WIFI_API_MUTEX_TIMEOUT)) {
        return false;
    }

    *address = g_wifi_dynamic.ip_address;

    xSemaphoreGiveRecursive(g_wifi_dynamic.mutex);

    return true;
}

#endif /* ENABLE_WIFI */
