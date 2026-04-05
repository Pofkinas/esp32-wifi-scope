/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "adc_config.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

static adc_continuous_data_t g_adc1_data[ADC1_FRAME_COUNT] = {0};
static uint32_t g_adc1_channel0_raw_data[ADC1_CHANNEL0_BUFFER] = {0};

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

/* clang-format off */
static const sAdcChannelDesc_t g_static_adc1_channel_desc[eAdcChannel_Last] = {
    [eAdcChannel_0] = {
        .attenuation = ADC_ATTEN_DB_12,
        .periph = ADC_UNIT_1,
        .channel = ADC_CHANNEL_0,
        .bit_width = ADC_BITWIDTH_12,
    }
};

static const sAdcDesc_t g_static_adc_desc[eAdc_Last] = {
    [eAdc_1] = {
        .used_channel_num = eAdcChannel_Last,
        .channel_desc = g_static_adc1_channel_desc,
        .frames_count = ADC1_FRAME_COUNT,
        .buffer_size = 8192,
        .sample_frequency_hz = 60000,
        .flush_pool = 1,
        .conversion_mode = ADC_CONV_SINGLE_UNIT_1,
        .output_format = ADC_DIGI_OUTPUT_FORMAT_TYPE2
    }
};
/* clang-format on */

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/* clang-format off */
adc_continuous_data_t *g_adc_data_buffers[eAdc_Last] = {
    [eAdc_1] = g_adc1_data
};

uint32_t *g_adc_channel_buffers[eAdc_Last][eAdcChannel_Last] = {
    [eAdc_1] = {
        [eAdcChannel_0] = g_adc1_channel0_raw_data
    }
};
/* clang-format off */

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

bool ADC_Config_IsCorrectAdc(const eAdc_t adc) {
    return (adc >= eAdc_First && adc < eAdc_Last);
}

bool ADC_Config_IsCorrectAdcChannel(const eAdcChannel_t channel) {
    return (channel >= eAdcChannel_First && channel < eAdcChannel_Last);
}

const sAdcDesc_t *ADC_Config_GetAdcDesc(const eAdc_t adc) {
    if (!ADC_Config_IsCorrectAdc(adc)) {
        return NULL;
    }

    return &g_static_adc_desc[adc];
}
