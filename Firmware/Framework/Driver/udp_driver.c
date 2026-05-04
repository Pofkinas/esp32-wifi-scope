/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "udp_driver.h"

#if defined(ENABLE_UDP)
#include <string.h>
#include <errno.h>

#include "lwip/sockets.h"
#include "lwip/netdb.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

#define UDP_DRIVER_INVALID_SOCKET (-1)

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

typedef enum eUdpDriverState {
    eUdpDriverState_First = 0,
    eUdpDriverState_Default = eUdpDriverState_First,
    eUdpDriverState_Initialized,
    eUdpDriverState_Last
} eUdpDriverState_t;

typedef struct sUdpDriver {
    eUdpDriverState_t state;
    int socket;
} sUdpDriver_t;

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

static sUdpDriver_t g_udp_dynamic_lut[eUdp_Last] = {0};

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

bool UDP_Driver_Init(const eUdp_t udp, const uint16_t local_port) {
    if (!UDP_Config_IsCorrectUdp(udp)) {
        return false;
    }

    if (eUdpDriverState_Initialized == g_udp_dynamic_lut[udp].state) {
        return true;
    }

    int socket_id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (socket_id < 0) {
        return false;
    }

    struct sockaddr_in local_addr = {0};

    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(local_port);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (0 != bind(socket_id, (struct sockaddr *) &local_addr, sizeof(local_addr))) {
        close(socket_id);

        return false;
    }

    g_udp_dynamic_lut[udp].socket = socket_id;
    g_udp_dynamic_lut[udp].state = eUdpDriverState_Initialized;

    return true;
}

bool UDP_Driver_Send(const eUdp_t udp, const char *ip_str, const uint16_t port, const uint8_t *data, const size_t size) {
    if (!UDP_Config_IsCorrectUdp(udp)) {
        return false;
    }

    if (!UDP_Config_IsCorrectUdp(udp) || (NULL == ip_str) || (NULL == data) || (0 == size)) {
        return false;
    }

    if (eUdpDriverState_Initialized != g_udp_dynamic_lut[udp].state) {
        return false;
    }

    struct sockaddr_in destination_address = {0};

    destination_address.sin_family = AF_INET;
    destination_address.sin_port = htons(port);

    if (1 != inet_pton(AF_INET, ip_str, &destination_address.sin_addr)) {
        return false;
    }

    int sent = sendto(g_udp_dynamic_lut[udp].socket, data, size, 0, (struct sockaddr *) &destination_address, sizeof(destination_address));

    return (sent == (int) size);
}

uint32_t UDP_Driver_WaitForData(const uint32_t timeout_ms) {
    fd_set read_fds;
    FD_ZERO(&read_fds);

    int max_fd = -1;

    for (eUdp_t udp = eUdp_First; udp < eUdp_Last; udp++) {
        if (eUdpDriverState_Initialized == g_udp_dynamic_lut[udp].state) {
            FD_SET(g_udp_dynamic_lut[udp].socket, &read_fds);

            if (g_udp_dynamic_lut[udp].socket > max_fd) {
                max_fd = g_udp_dynamic_lut[udp].socket;
            }
        }
    }

    if (max_fd < 0) {
        return 0;
    }

    struct timeval timeval_timeout = {0};
    struct timeval *timeout = NULL; // NULL timeout means wait indefinitely

    if (portMAX_DELAY != timeout_ms) {
        timeval_timeout.tv_sec = timeout_ms / 1000U;
        timeval_timeout.tv_usec = (timeout_ms % 1000U) * 1000U;
        timeout = &timeval_timeout;
    }

    if (0 >= select((max_fd + 1), &read_fds, NULL, NULL, timeout)) {
        return 0;
    }

    uint32_t mask = 0;

    for (eUdp_t udp = eUdp_First; udp < eUdp_Last; udp++) {
        if ((eUdpDriverState_Initialized == g_udp_dynamic_lut[udp].state)) {
            if (FD_ISSET(g_udp_dynamic_lut[udp].socket, &read_fds)) {
                mask |= (1U << (uint32_t) udp);
            }
        }
    }

    return mask;
}

bool UDP_Driver_Receive(const eUdp_t udp, uint8_t *data, size_t *size) {
    if (!UDP_Config_IsCorrectUdp(udp)) {
        return false;
    }

    if ((NULL == data) || (NULL == size)) {
        return false;
    }

    if (0 == *size) {
        return false;
    }

    if (eUdpDriverState_Initialized != g_udp_dynamic_lut[udp].state) {
        return false;
    }

    struct sockaddr_in source_addr = {0};
    socklen_t source_len = sizeof(source_addr);

    int received = recvfrom(g_udp_dynamic_lut[udp].socket, data, *size, 0, (struct sockaddr *) &source_addr, &source_len);

    if (received <= 0) {
        return false;
    }

    if ((size_t) received > *size) {
        return false;
    }

    *size = (size_t) received;

    return true;
}

bool UDP_Driver_CloseSocket(const eUdp_t udp) {
    if (!UDP_Config_IsCorrectUdp(udp)) {
        return false;
    }

    if (eUdpDriverState_Initialized != g_udp_dynamic_lut[udp].state) {
        return true;
    }

    if (0 != close(g_udp_dynamic_lut[udp].socket)) {
        return false;
    }

    g_udp_dynamic_lut[udp].state = eUdpDriverState_Default;

    return true;
}

#endif /* ENABLE_WIFI */
