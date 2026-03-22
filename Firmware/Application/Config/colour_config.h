#ifndef APPLICATION_CONFIG_COLOUR_CONFIG_H_
#define APPLICATION_CONFIG_COLOUR_CONFIG_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "framework_config.h"

#include <stdbool.h>
#include <stdint.h>

/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

#define MAX_BRIGHTNESS 255

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

typedef enum eColour {
    eColour_First = 0,
    eColour_Off = eColour_First,
    eColour_Red,
    eColour_Green,
    eColour_Blue,
    eColour_Yellow,
    eColour_Cyan,
    eColour_Magenta,
    eColour_White,
    eColour_Last
} eColour_t;

typedef uint32_t ColourRgb_t;

typedef struct sColourHsv {
    uint8_t hue;
    uint8_t saturation;
    uint8_t value;
} sColourHsv_t;

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

bool Colour_Config_IsCorrectColour(const eColour_t colour);
bool Colour_Config_GetRgb(const eColour_t colour, ColourRgb_t *rgb);
bool Colour_Config_GetHsv(const eColour_t colour, sColourHsv_t *hsv);

#endif /* APPLICATION_CONFIG_COLOUR_CONFIG_H_ */
