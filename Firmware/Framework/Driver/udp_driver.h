#ifndef SOURCE_DRIVER_UDP_DRIVER_H_
#define SOURCE_DRIVER_UDP_DRIVER_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "framework_config.h"

#if defined(ENABLE_UDP)
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "transport_config.h"

/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

bool UDP_Driver_Init(const eUdp_t udp, const uint16_t local_port);
bool UDP_Driver_Send(const eUdp_t udp, const char *ip_str, const uint16_t port, const uint8_t *data, const size_t size);
uint32_t UDP_Driver_WaitForData(const uint32_t timeout_ms);
bool UDP_Driver_Receive(const eUdp_t udp, uint8_t *buf, size_t *size);
bool UDP_Driver_CloseSocket(const eUdp_t udp);

#endif /* ENABLE_UDP */
#endif /* SOURCE_DRIVER_UDP_DRIVER_H_ */
