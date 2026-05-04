#ifndef SOURCE_DRIVER_WIFI_DRIVER_H_
#define SOURCE_DRIVER_WIFI_DRIVER_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "framework_config.h"

#if defined(ENABLE_WIFI)
#include <stdbool.h>
#include <stdint.h>
#include "wifi_config.h"
#include "transport_config.h"

/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

typedef enum eWifiEvent {
    eWifiEvent_First = 0,
    eWifiEvent_Connected = eWifiEvent_First,
    eWifiEvent_Disconnected,
    eWifiEvent_GotIp,
    eWifiEvent_Last
} eWifiEvent_t;

typedef void (*wifi_event_callback_t)(eWifiEvent_t event_id);

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

bool Wifi_Driver_Init(const wifi_event_callback_t callback);
bool Wifi_Driver_Start(void);
bool Wifi_Driver_Stop(void);
bool Wifi_Driver_Connect(const char *ssid, const char *password);
bool Wifi_Driver_Reconnect(void);
bool Wifi_Driver_Disconnect(void);
bool Wifi_Driver_GetIpv4Address(uIPv4Address_t *address);

#endif /* ENABLE_WIFI */
#endif /* SOURCE_DRIVER_WIFI_DRIVER_H_ */
