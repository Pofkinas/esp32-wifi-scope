#ifndef SOURCE_DRIVER_EXTI_DRIVER_H_
#define SOURCE_DRIVER_EXTI_DRIVER_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "framework_config.h"

#if defined(ENABLE_EXTI)
#include <stdbool.h>
#include "io_config.h"

/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

typedef enum eExtiTrigger {
    eExtiTrigger_First = 0,
    eExtiTrigger_Rising = eExtiTrigger_First,
    eExtiTrigger_Falling,
    eExtiTrigger_RisingFalling,
    eExtiTrigger_Last
} eExtiTrigger_t;

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

typedef void (*exti_callback_t) (void *context);

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

bool Exti_Driver_InitDevice (const eIo_t exti_device, exti_callback_t exti_callback, void *callback_context);
bool Exti_Driver_DisableIt (const eIo_t exti_device);
bool Exti_Driver_EnableIt (const eIo_t exti_device);
bool Exti_Driver_SetTriggerType (const eIo_t exti_device, const eExtiTrigger_t trigger_type);
bool Exti_Driver_ClearFlag (const eIo_t exti_device);

#endif /* ENABLE_EXTI */
#endif /* SOURCE_DRIVER_EXTI_DRIVER_H_ */
