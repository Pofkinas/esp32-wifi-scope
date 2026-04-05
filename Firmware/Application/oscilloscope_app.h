#ifndef APPLICATION_OSCILLOSCOPE_APP_H
#define APPLICATION_OSCILLOSCOPE_APP_H
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "framework_config.h"

#include <stdbool.h>

/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

// Each voltage sample is represented as uint16_t (2 bytes), so a 1 KB chunk holds 512 samples.
// ADC running @10 kSps generates 10000 * 2 = 20000 B/s split across 30 frames,
// each sending (20000 / 30) ~= 667 B — fits in 1 chunk.
#define FRAME_SIZE 1024
#define MAX_FRAMES 128

#define FRAMES_PER_SECOND 50

// Data header format: "<timestamp>{separator}<frame_number>{separator}<total_bytes>{delimiter}"
// Total bytes is the number of bytes captured in frame
#define FRAME_HEADER_SIZE 32
#define FRAME_HEADER_DELIMITER "\n"
#define FRAME_HEADER_SEPARATOR " "

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

bool Oscilloscope_APP_Init(void);
bool Oscilloscope_APP_Start(void);
bool Oscilloscope_APP_Pause(void);
bool Oscilloscope_APP_Stop(void);

#endif /* APPLICATION_OSCILLOSCOPE_APP_H */
