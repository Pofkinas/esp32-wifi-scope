#ifndef SOURCE_API_CAPTURE_API_H
#define SOURCE_API_CAPTURE_API_H
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "framework_config.h"

#if defined(ENABLE_CAPTURE)
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "capture_config.h"
#include "ring_buffer.h"

/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

typedef enum eCaptureType {
    eCaptureType_First = 0,
    eCaptureType_Adc = eCaptureType_First,
    eCaptureType_Last
} eCaptureType_t;

typedef enum eCaptureEvent {
    eCaptureEvent_First = 0,
    eCaptureEvent_CaptureDone = eCaptureEvent_First,
    eCaptureEvent_Overflow,
    eCaptureEvent_Last
} eCaptureEvent_t;

typedef bool (*capture_process_t)(uint32_t *raw_data, void *out_data, size_t elements, const void *context);
typedef void (*capture_notify_t)(const eCaptureDevice_t device, const eCaptureEvent_t event);

typedef struct sCaptureDesc {
    eCaptureType_t type;
    void const *device_data;
    size_t data_size;
    capture_process_t process_callback;
    void const *process_context;
    capture_notify_t notify_callback;
} sCaptureDesc_t;

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

bool Capture_API_Init(const eCaptureDevice_t device, const sCaptureDesc_t *desc, const sRingBuffer_Handle_t buffer);
bool Capture_API_Start(const eCaptureDevice_t device, const size_t data_to_capture);
bool Capture_API_Stop(const eCaptureDevice_t device);
bool Capture_API_UpdateTarget(const eCaptureDevice_t device, const size_t data_to_capture);
bool Capture_API_ClearBuffer(const eCaptureDevice_t device);

#endif /* ENABLE_CAPTURE */
#endif /* SOURCE_API_CAPTURE_API_H */
