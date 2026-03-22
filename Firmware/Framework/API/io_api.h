#ifndef SOURCE_API_IO_API_H_
#define SOURCE_API_IO_API_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "framework_config.h"

#if defined(ENABLE_IO)
#include <stdbool.h>
#include <stdint.h>
#include "io_config.h"

#include "freertos/event_groups.h"

/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

typedef enum ePressType {
    ePressType_First = 0,
    ePressType_None = ePressType_First,
    ePressType_Default = 0x1,
    ePressType_Long = 0x2,
    ePressType_Last
} ePressType_t;
 
typedef struct sIoEvent {
    eIo_t device;
    uint32_t pressed_tick;
    uint32_t detected_press_type;
} sIoEvent_t;

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

bool IO_API_Init (const eIo_t device, EventGroupHandle_t event_group);
bool IO_API_Start (void);
bool IO_API_Stop (void);
bool IO_API_ReadPinState (const eIo_t device, bool *pin_state);
sIoEvent_t *IO_API_GetEventData(const eIo_t device);

#endif /* ENABLE_IO */
#endif /* SOURCE_API_IO_API_H_ */
