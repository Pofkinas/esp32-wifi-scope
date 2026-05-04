/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "wifi_driver.h"

#if defined(ENABLE_WIFI)
#include <string.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

typedef enum eWifiDriverState {
    eWifiDriverState_First = 0,
    eWifiDriverState_Default = eWifiDriverState_First,
    eWifiDriverState_Initialized,
    eWifiDriverState_Disconnected,
    eWifiDriverState_AttemptingConnection,
    eWifiDriverState_Connected,
    eWifiDriverState_Last
} eWifiDriverState_t;

typedef struct sWifiDriver {
    eWifiDriverState_t state;
    char *default_ssid;
    char *default_password;
    esp_netif_t *netif;
    esp_event_handler_instance_t wifi_event_instance;
    esp_event_handler_instance_t ip_event_instance;
    wifi_event_callback_t event_callback;
} sWifiDriver_t;

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

static sWifiDriver_t g_wifi_driver = {0};

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

static void Wifi_Driver_DispatchEvent(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static eWifiEvent_t Wifi_Driver_HandleWifiEvent(const int32_t event_id);
static eWifiEvent_t Wifi_Driver_HandleIpEvent(const int32_t event_id);

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

static void Wifi_Driver_DispatchEvent(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (NULL == g_wifi_driver.event_callback) {
        return;
    }

    eWifiEvent_t driver_event = eWifiEvent_Last;

    if (WIFI_EVENT == event_base) {
        driver_event = Wifi_Driver_HandleWifiEvent(event_id);
    } else if (IP_EVENT == event_base) {
        driver_event = Wifi_Driver_HandleIpEvent(event_id);
    }

    g_wifi_driver.event_callback(driver_event);

    return;
}

static eWifiEvent_t Wifi_Driver_HandleWifiEvent(const int32_t event_id) {
    switch (event_id) {
        case WIFI_EVENT_STA_CONNECTED: {
            g_wifi_driver.state = eWifiDriverState_Connected;
            return eWifiEvent_Connected;
        } break;
        case WIFI_EVENT_STA_DISCONNECTED: {
            if (eWifiDriverState_Disconnected == g_wifi_driver.state) {
                break;
            }

            g_wifi_driver.state = eWifiDriverState_Disconnected;
            return eWifiEvent_Disconnected;
        } break;
        default: {
        } break;
    }

    return eWifiEvent_Last;
}

static eWifiEvent_t Wifi_Driver_HandleIpEvent(const int32_t event_id) {
    switch (event_id) {
        case IP_EVENT_STA_GOT_IP: {
            g_wifi_driver.state = eWifiDriverState_Connected;
            return eWifiEvent_GotIp;
        } break;
        default: {
        } break;
    }

    return eWifiEvent_Last;
}

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

bool Wifi_Driver_Init(const wifi_event_callback_t callback) {
    if (NULL == callback) {
        return false;
    }

    if (eWifiDriverState_Default != g_wifi_driver.state) {
        return true;
    }

    const sWifiStaDesc_t *desc = Wifi_Config_GetWifiStaDesc();

    if (NULL == desc) {
        return false;
    }

    g_wifi_driver.default_ssid = desc->default_ssid;
    g_wifi_driver.default_password = desc->default_password;

    esp_err_t error = nvs_flash_init();

    if (error == ESP_ERR_NVS_NO_FREE_PAGES || error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    if (ESP_OK != esp_event_loop_create_default()) {
        return false;
    }

    if (ESP_OK != esp_netif_init()) {
        return false;
    }

    g_wifi_driver.netif = esp_netif_create_default_wifi_sta();

    if (NULL == g_wifi_driver.netif) {
        return false;
    }

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();

    if (ESP_OK != esp_wifi_init(&init_cfg)) {
        return false;
    }

    if (ESP_OK != esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &Wifi_Driver_DispatchEvent, NULL, &g_wifi_driver.wifi_event_instance)) {
        return false;
    }

    if (ESP_OK != esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &Wifi_Driver_DispatchEvent, NULL, &g_wifi_driver.ip_event_instance)) {
        return false;
    }

    if (ESP_OK != esp_wifi_set_mode(WIFI_MODE_STA)) {
        return false;
    }

    g_wifi_driver.event_callback = callback;
    g_wifi_driver.state = eWifiDriverState_Initialized;

    return true;
}

bool Wifi_Driver_Start(void) {
    if (eWifiDriverState_Initialized != g_wifi_driver.state) {
        return false;
    }

    if (ESP_OK != esp_wifi_start()) {
        return false;
    }

    g_wifi_driver.state = eWifiDriverState_Disconnected;

    return true;
}

bool Wifi_Driver_Stop(void) {
    if (eWifiDriverState_Disconnected != g_wifi_driver.state) {
        return false;
    }

    if (ESP_OK != esp_wifi_stop()) {
        return false;
    }

    g_wifi_driver.state = eWifiDriverState_Initialized;

    return true;
}

bool Wifi_Driver_Connect(const char *ssid, const char *password) {
    if (eWifiDriverState_Disconnected != g_wifi_driver.state) {
        return false;
    }

    wifi_config_t wifi_cfg = {0};

    const char *use_ssid = (NULL == ssid) ? g_wifi_driver.default_ssid : ssid;
    const char *use_password = (NULL == password) ? g_wifi_driver.default_password : password;

    strncpy((char *) wifi_cfg.sta.ssid, use_ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *) wifi_cfg.sta.password, use_password, sizeof(wifi_cfg.sta.password) - 1);

    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_cfg.sta.pmf_cfg.capable = true;
    wifi_cfg.sta.pmf_cfg.required = false;

    if (ESP_OK != esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg)) {
        return false;
    }

    if (ESP_OK != esp_wifi_connect()) {
        return false;
    }

    g_wifi_driver.state = eWifiDriverState_AttemptingConnection;

    return true;
}

bool Wifi_Driver_Reconnect(void) {
    if ((eWifiDriverState_Disconnected != g_wifi_driver.state) && (eWifiDriverState_AttemptingConnection != g_wifi_driver.state)) {
        return false;
    }

    if (ESP_OK != esp_wifi_connect()) {
        return false;
    }

    g_wifi_driver.state = eWifiDriverState_AttemptingConnection;

    return true;
}

bool Wifi_Driver_Disconnect(void) {
    if ((eWifiDriverState_Connected != g_wifi_driver.state) && (eWifiDriverState_AttemptingConnection != g_wifi_driver.state)) {
        return false;
    }

    if (ESP_OK != esp_wifi_disconnect()) {
        return false;
    }

    g_wifi_driver.state = eWifiDriverState_Disconnected;

    return true;
}

bool Wifi_Driver_GetIpv4Address(uIPv4Address_t *address) {
    if (NULL == address) {
        return false;
    }

    if (eWifiDriverState_Connected != g_wifi_driver.state) {
        return false;
    }

    esp_netif_ip_info_t ip_info = {0};

    if (ESP_OK != esp_netif_get_ip_info(g_wifi_driver.netif, &ip_info)) {
        return false;
    }

    address->raw = ip_info.ip.addr;

    return true;
}

#endif /* ENABLE_WIFI */
