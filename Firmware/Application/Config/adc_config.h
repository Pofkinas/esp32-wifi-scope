#ifndef APPLICATION_CONFIG_ADC_CONFIG_H
#define APPLICATION_CONFIG_ADC_CONFIG_H
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "framework_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "esp_adc/adc_continuous.h"

/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

#define ADC1_FRAME_COUNT 256
#define ADC1_CHANNEL0_BUFFER 4096

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

typedef enum eAdc {
    eAdc_First = 0,
    eAdc_1 = eAdc_First,
    eAdc_Last
} eAdc_t;

typedef enum eAdcChannel {
    eAdcChannel_First = 0,
    eAdcChannel_0 = eAdcChannel_First,
    eAdcChannel_Last
} eAdcChannel_t;

typedef struct sAdcChannelDesc {
    uint8_t attenuation;
    uint8_t channel;
    uint8_t periph;
    uint8_t bit_width;
#ifdef ENABLE_VOLTAGE
    uint32_t v_ref;
#endif /* ENABLE_VOLTAGE */
} sAdcChannelDesc_t;

typedef struct sAdcDesc {
    uint32_t used_channel_num;
    sAdcChannelDesc_t const *channel_desc;
    uint32_t frames_count;
    uint32_t buffer_size;
    uint32_t sample_frequency_hz;
    uint8_t flush_pool;
    adc_digi_convert_mode_t conversion_mode;
    adc_digi_output_format_t output_format;
} sAdcDesc_t;

typedef struct sAdcConfig {
    eAdc_t adc;
    eAdcChannel_t channel;
} sAdcConfig_t;

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

extern adc_continuous_data_t *g_adc_data_buffers[eAdc_Last];
extern uint32_t *g_adc_channel_buffers[eAdc_Last][eAdcChannel_Last];

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

bool ADC_Config_IsCorrectAdc(const eAdc_t adc);
bool ADC_Config_IsCorrectAdcChannel(const eAdcChannel_t channel);
const sAdcDesc_t *ADC_Config_GetAdcDesc(const eAdc_t adc);

#endif /* APPLICATION_CONFIG_ADC_CONFIG_H */
