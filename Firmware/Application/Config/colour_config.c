/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "colour_config.h"

#include <stddef.h>

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

/* clang-format off */
static const uint32_t g_static_rgb_value_lut[eColour_Last] = {
    [eColour_Off] = 0x000000,
    [eColour_Red] = 0xFF0000,
    [eColour_Green] = 0x00FF00,
    [eColour_Blue] = 0x0000FF,
    [eColour_Yellow] = 0xFFFF00,
    [eColour_Cyan] = 0x00FFFF,
    [eColour_Magenta] = 0xFF00FF,
    [eColour_White] = 0xFFFFFF
};

static const sColourHsv_t g_static_hsv_value_lut[eColour_Last] = {
    [eColour_Off] = {
        .hue = 0,
        .saturation = 0,
        .value = 0
    },
    [eColour_Red] = {
        .hue = 0,
        .saturation = 255,
        .value = 255
    },
    [eColour_Green] = {
        .hue = 85,
        .saturation = 255,
        .value = 255
    },
    [eColour_Blue] = {
        .hue = 171,
        .saturation = 255,
        .value = 255
    },
    [eColour_Yellow] = {
        .hue = 43,
        .saturation = 255,
        .value = 255
    },
    [eColour_Cyan] = {
        .hue = 128,
        .saturation = 255,
        .value = 255
    },
    [eColour_Magenta] = {
        .hue = 213,
        .saturation = 255,
        .value = 255
    },
    [eColour_White] = {
        .hue = 0,
        .saturation = 0,
        .value = 255
    }
};
/* clang-format on */

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

bool Colour_Config_IsCorrectColour (const eColour_t colour) {
    return (colour >= eColour_First) && (colour < eColour_Last);
}

bool Colour_Config_GetRgb (const eColour_t colour, ColourRgb_t *rgb) {
    if (!Colour_Config_IsCorrectColour(colour) || (NULL == rgb)) {
        return false;
    }

    *rgb = g_static_rgb_value_lut[colour];
    
    return true;
}

bool Colour_Config_GetHsv (const eColour_t colour, sColourHsv_t *hsv) {
    if (!Colour_Config_IsCorrectColour(colour) || (NULL == hsv)) {
        return false;
    }

    *hsv = g_static_hsv_value_lut[colour];
    
    return true;
}
