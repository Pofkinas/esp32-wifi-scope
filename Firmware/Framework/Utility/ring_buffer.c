/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "ring_buffer.h"

#include <string.h>

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

struct sRingBufferDesc {
    size_t buffer_capacity;
    size_t data_size;
    size_t head;
    size_t tail;
    size_t count;
    uint8_t *buffer;
};

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

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

sRingBuffer_Handle_t Ring_Buffer_Init(size_t buffer_capacity, size_t data_size) {
    sRingBuffer_Handle_t ring_buffer = malloc(sizeof(struct sRingBufferDesc));

    if (NULL == ring_buffer) {
        return NULL;
    }

    ring_buffer->buffer_capacity = buffer_capacity;
    ring_buffer->data_size = data_size;

    ring_buffer->buffer = malloc(buffer_capacity * data_size);

    if (NULL == ring_buffer->buffer) {
        free(ring_buffer);
        return NULL;
    }

    ring_buffer->head = 0;
    ring_buffer->tail = 0;
    ring_buffer->count = 0;

    return ring_buffer;
}

bool Ring_Buffer_DeInit(sRingBuffer_Handle_t ring_buffer) {
    if (NULL == ring_buffer) {
        return false;
    }

    if (NULL == ring_buffer->buffer) {
        free(ring_buffer);
        return false;
    }

    free(ring_buffer->buffer);
    free(ring_buffer);
    return true;
}

bool Ring_Buffer_IsFull(sRingBuffer_Handle_t ring_buffer) {
    if (NULL != ring_buffer) {
        return (ring_buffer->count == ring_buffer->buffer_capacity);
    }

    return false;
}

bool Ring_Buffer_IsEmpty(sRingBuffer_Handle_t ring_buffer) {
    if (NULL != ring_buffer) {
        return (0 == ring_buffer->count);
    }

    return false;
}

bool Ring_Buffer_Push(sRingBuffer_Handle_t ring_buffer, const void *data) {
    if ((NULL == ring_buffer) || (NULL == data)) {
        return false;
    }

    memcpy(&ring_buffer->buffer[ring_buffer->head], data, ring_buffer->data_size);
    ring_buffer->head += ring_buffer->data_size;

    if (ring_buffer->count < ring_buffer->buffer_capacity) {
        ring_buffer->count++;
    }

    if ((ring_buffer->buffer_capacity * ring_buffer->data_size) == ring_buffer->head) {
        ring_buffer->head = 0;
    }

    if (Ring_Buffer_IsFull(ring_buffer)) {
        ring_buffer->tail = ring_buffer->head;
    }

    return true;
}

bool Ring_Buffer_PushBulk(sRingBuffer_Handle_t ring_buffer, const void *data, const size_t elements) {
    if ((NULL == ring_buffer) || (NULL == data) || (0 == elements)) {
        return false;
    }

    size_t data_bytes = elements * ring_buffer->data_size;
    size_t buf_bytes = ring_buffer->buffer_capacity * ring_buffer->data_size;
    size_t space_to_end = buf_bytes - ring_buffer->head;

    if (data_bytes <= space_to_end) {
        memcpy(&ring_buffer->buffer[ring_buffer->head], data, data_bytes);
    } else {
        memcpy(&ring_buffer->buffer[ring_buffer->head], data, space_to_end);
        memcpy(&ring_buffer->buffer[0], (const uint8_t *) data + space_to_end, data_bytes - space_to_end);
    }

    ring_buffer->head = (ring_buffer->head + data_bytes) % buf_bytes;

    if ((ring_buffer->count + elements) > ring_buffer->buffer_capacity) {
        ring_buffer->tail = ring_buffer->head;
        ring_buffer->count = ring_buffer->buffer_capacity;
    } else {
        ring_buffer->count += elements;
    }

    return true;
}

bool Ring_Buffer_Pop(sRingBuffer_Handle_t ring_buffer, void *data) {
    if ((NULL == ring_buffer) || (NULL == data)) {
        return false;
    }

    if (Ring_Buffer_IsEmpty(ring_buffer)) {
        return false;
    }

    memcpy(data, &ring_buffer->buffer[ring_buffer->tail], ring_buffer->data_size);
    ring_buffer->tail += ring_buffer->data_size;

    if (ring_buffer->count > 0) {
        ring_buffer->count--;
    }

    if ((ring_buffer->buffer_capacity * ring_buffer->data_size) == ring_buffer->tail) {
        ring_buffer->tail = 0;
    }

    return true;
}

bool Ring_Buffer_PopBulk(sRingBuffer_Handle_t ring_buffer, void *data, const size_t elements) {
    if ((NULL == ring_buffer) || (NULL == data) || (0 == elements)) {
        return false;
    }

    if (ring_buffer->count < elements) {
        return false;
    }

    size_t data_bytes = elements * ring_buffer->data_size;
    size_t buf_bytes = ring_buffer->buffer_capacity * ring_buffer->data_size;
    size_t data_to_end = buf_bytes - ring_buffer->tail;

    if (data_bytes <= data_to_end) {
        memcpy(data, &ring_buffer->buffer[ring_buffer->tail], data_bytes);
    } else {
        memcpy(data, &ring_buffer->buffer[ring_buffer->tail], data_to_end);
        memcpy((uint8_t *) data + data_to_end, &ring_buffer->buffer[0], data_bytes - data_to_end);
    }

    ring_buffer->tail = (ring_buffer->tail + data_bytes) % buf_bytes;
    ring_buffer->count -= elements;

    return true;
}

bool Ring_Buffer_Clear(sRingBuffer_Handle_t ring_buffer) {
    if (NULL == ring_buffer) {
        return false;
    }

    memset(ring_buffer->buffer, 0, (ring_buffer->buffer_capacity * ring_buffer->data_size));

    ring_buffer->head = 0;
    ring_buffer->tail = 0;
    ring_buffer->count = 0;

    return true;
}
