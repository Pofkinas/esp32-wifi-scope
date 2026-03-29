#ifndef SOURCE_UTILITY_RING_BUFFER_H_
#define SOURCE_UTILITY_RING_BUFFER_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

typedef struct sRingBufferDesc *sRingBuffer_Handle_t;

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

sRingBuffer_Handle_t Ring_Buffer_Init(size_t buffer_capacity, size_t data_size);
bool Ring_Buffer_DeInit(sRingBuffer_Handle_t ring_buffer);
bool Ring_Buffer_IsFull(sRingBuffer_Handle_t ring_buffer);
bool Ring_Buffer_IsEmpty(sRingBuffer_Handle_t ring_buffer);
bool Ring_Buffer_Push(sRingBuffer_Handle_t ring_buffer, const void *data);
bool Ring_Buffer_PushBulk(sRingBuffer_Handle_t ring_buffer, const void *data, const size_t elements);
bool Ring_Buffer_Pop(sRingBuffer_Handle_t ring_buffer, void *data);
bool Ring_Buffer_PopBulk(sRingBuffer_Handle_t ring_buffer, void *data, const size_t elements);
bool Ring_Buffer_Clear(sRingBuffer_Handle_t ring_buffer);

#endif /* SOURCE_UTILITY_RING_BUFFER_H_ */
