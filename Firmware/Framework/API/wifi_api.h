#ifndef SOURCE_API_WIFI_API_H_
#define SOURCE_API_WIFI_API_H_
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

typedef enum eWifiState {
    eWifiState_First = 0,
    eWifiState_Default = eWifiState_First,
    eWifiState_Disconnected,
    eWifiState_AttemptingConnection,
    eWifiState_Connected,
    eWifiState_Ready,
    eWifiState_Last
} eWifiState_t;

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

bool Wifi_API_Init(void);
bool Wifi_API_Connect(const char *ssid, const char *password);
bool Wifi_API_Disconnect(void);
eWifiState_t Wifi_API_GetStatus(void);
bool Wifi_API_GetIpv4Address(uIPv4Address_t *address);

#endif /* ENABLE_WIFI */
#endif /* SOURCE_API_WIFI_API_H_ */
