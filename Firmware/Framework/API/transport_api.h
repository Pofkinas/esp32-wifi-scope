#ifndef SOURCE_API_TRANSPORT_API_H_
#define SOURCE_API_TRANSPORT_API_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "framework_config.h"

#if defined(ENABLE_TRANSPORT)
#include <stdbool.h>
#include <stdint.h>
#include "message.h"

/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

typedef enum eTransportMode {
    eTransport_First = 0,
    eTransport_UART = eTransport_First,
    eTransport_WiFi_UDP,
    eTransport_Last
} eTransportMode_t;

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

bool Transport_API_Init(void);
bool Transport_API_SetMode(const eTransportMode_t mode);
eTransportMode_t Transport_API_GetMode(void);
bool Transport_API_Send(const sMessage_t message, const uint32_t timeout);
bool Transport_API_Receive(sMessage_t *message, const uint32_t timeout);

#endif /* ENABLE_TRANSPORT */
#endif /* SOURCE_API_TRANSPORT_API_H_ */
