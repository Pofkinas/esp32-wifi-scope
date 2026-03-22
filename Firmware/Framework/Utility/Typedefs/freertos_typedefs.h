#ifndef SOURCE_UTILITY_TYPEDEFS_FREERTOS_TYPEDEFS_H_
#define SOURCE_UTILITY_TYPEDEFS_FREERTOS_TYPEDEFS_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "framework_config.h"

#include <stdint.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"

/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

#if defined(MCU_ESP32_S3) 
    #define STACK_SIZE_IN_BYTES(x)  (x)
#endif /* MCU_ESP32_S3 */
#if defined(MCU_STM32FXX)
    #define STACK_SIZE_IN_BYTES(x)  ((x) / sizeof(StackType_t))
#endif /* MCU_STM32FXX */

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

typedef enum eTaskPriority {
    eTaskPriority_First = 0,
    eTaskPriority_Idle = eTaskPriority_First,
    eTaskPriority_Low,
    eTaskPriority_BelowNormal,
    eTaskPriority_Normal,
    eTaskPriority_AboveNormal,
    eTaskPriority_High,
    eTaskPriority_Realtime,
    eTaskPriority_Last
} eTaskPriority_t;

typedef struct sTaskDesc {
    char *name;
    uint32_t stack_size;
    UBaseType_t priority;
} sTaskDesc_t;

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

#endif /* SOURCE_UTILITY_TYPEDEFS_FREERTOS_TYPEDEFS_H_ */
