#ifndef APPLICATION_CONFIG_TRANSPORT_CONFIG_H
#define APPLICATION_CONFIG_TRANSPORT_CONFIG_H
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "framework_config.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "freertos_types.h"

/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

#define IPV4_STRING_LEN 16

#define UDP_TARGET_IP 192, 168, 0, 103 // 192.168.0.103
#define UDP_TARGET_PORT 4210
#define UDP_LOCAL_PORT 4211

#define UDP_API_RX_BUFFER_SIZE 1500
#define UDP_API_MESSAGE_QUEUE_CAPACITY 8
#define UDP_API_MESSAGE_QUEUE_PUT_TIMEOUT 0U
#define UDP_API_RX_TIMEOUT_MS 1000U
#define UDP_API_MUTEX_TIMEOUT 0U

#define IP_SEPARATOR "."
#define IP_SEPARATOR_LENGTH (sizeof(IP_SEPARATOR) - 1)

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

typedef union uIPv4Address {
    uint32_t raw;
    uint8_t octets[4];
} uIPv4Address_t;

typedef enum eUdp {
    eUdp_First = 0,
    eUdp_Socket0 = eUdp_First,
    eUdp_Last
} eUdp_t;

typedef struct sUdpDesc {
    uint16_t local_port;
    uint16_t default_target_port;
    uIPv4Address_t *default_target_ip;
} sUdpDesc_t;

typedef struct sUdpApiConst {
    size_t rx_buffer_capacity;
} sUdpApiConst_t;

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

extern const sTaskDesc_t g_udp_fsm_thread_attributes;

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

bool UDP_Config_IsCorrectUdp(const eUdp_t udp);
const sUdpDesc_t *UDP_Config_GetUdpDesc(const eUdp_t udp);
const sUdpApiConst_t *UDP_Config_GetUdpApiConst(const eUdp_t udp);

#endif /* APPLICATION_CONFIG_TRANSPORT_CONFIG_H */
