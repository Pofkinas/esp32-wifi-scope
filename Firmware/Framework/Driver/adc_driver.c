/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "adc_driver.h"

#if defined(ENABLE_ADC)
#include <stdio.h>
#include "esp_adc/adc_continuous.h"
#ifdef ENABLE_VOLTAGE
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#endif /* ENABLE_VOLTAGE */

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

#define ADC_READ_PARSE_DELAY 0U

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

typedef enum eAdcState {
    eAdcState_First = 0,
    eAdcState_Default = eAdcState_First,
    eAdcState_Initialized,
    eAdcState_Running,
    eAdcState_Last
} eAdcState_t;

typedef struct sAdcDynamic {
    eAdc_t adc;
    eAdcState_t state;
    adc_callback_t callback;
    adc_continuous_handle_t handle;
    adc_continuous_data_t *adc_data;
} sAdcDynamic_t;

typedef struct sAdcChannelDynamic {
    uint32_t *raw_data;
    size_t size;
#ifdef ENABLE_VOLTAGE
    adc_cali_handle_t calib_handle;
#endif /* ENABLE_VOLTAGE */
} sAdcChannelDynamic_t;

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

static sAdcDesc_t g_adc_lut[eAdc_Last] = {0};
static sAdcDynamic_t g_adc_dynamic[eAdc_Last] = {0};
static sAdcChannelDynamic_t g_adc_channel_dynamic[eAdc_Last][eAdcChannel_Last] = {0};

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

static bool ADC_Driver_IRQHandler(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, eAdcEvent_t event, void *user_data);
static bool ADC_Driver_ConvDone_IRQHandler(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data);
static bool ADC_Driver_PoolOvf_IRQHandler(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data);
static bool ADC_Driver_BuildChannelData(const sAdcDesc_t adc, const adc_continuous_data_t *parsed_data, const size_t samples, sAdcChannelDynamic_t *channels);

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

static bool IRAM_ATTR ADC_Driver_IRQHandler(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, eAdcEvent_t event, void *user_data) {
    if (NULL == user_data) {
        return false;
    }

    bool yield_from_isr = false;
    eAdc_t adc = *(eAdc_t *) user_data;

    if (!ADC_Config_IsCorrectAdc(adc)) {
        return false;
    }

    if (NULL != g_adc_dynamic[adc].callback) {
        g_adc_dynamic[adc].callback(adc, event, &yield_from_isr);
    }

    return yield_from_isr;
}

static bool IRAM_ATTR ADC_Driver_ConvDone_IRQHandler(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data) {
    return ADC_Driver_IRQHandler(handle, edata, eAdcEvent_CaptureDone, user_data);
}

static bool IRAM_ATTR ADC_Driver_PoolOvf_IRQHandler(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data) {
    return ADC_Driver_IRQHandler(handle, edata, eAdcEvent_Overflow, user_data);
}

static bool ADC_Driver_BuildChannelData(const sAdcDesc_t adc, const adc_continuous_data_t *parsed_data, const size_t samples, sAdcChannelDynamic_t *channels) {
    if ((NULL == parsed_data) || (NULL == channels)) {
        return false;
    }

    if (0 == samples) {
        return false;
    }

    for (eAdcChannel_t channel = eAdcChannel_First; channel < eAdcChannel_Last; channel++) {
        channels[channel].size = 0;
    }

    for (size_t sample = 0; sample < samples; sample++) {
        if (parsed_data[sample].valid) {
            for (eAdcChannel_t channel = eAdcChannel_First; channel < eAdcChannel_Last; channel++) {
                if (parsed_data[sample].channel == adc.channel_desc[channel].channel) {
                    channels[channel].raw_data[channels[channel].size] = parsed_data[sample].raw_data;
                    channels[channel].size++;

                    if (channels[channel].size >= adc.frames_count) {
                        break;
                    }
                }
            }
        }
    }

    return true;
}

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

bool ADC_Driver_InitDevice(const eAdc_t adc, adc_callback_t adc_callback) {
    if (!ADC_Config_IsCorrectAdc(adc)) {
        return false;
    }

    if (NULL == adc_callback) {
        return false;
    }

    if (eAdcState_Default != g_adc_dynamic[adc].state) {
        return true;
    }

    const sAdcDesc_t *adc_lut = ADC_Config_GetAdcDesc(adc);

    if (NULL == adc_lut) {
        return false;
    }

    if (0 != (adc_lut->frames_count % SOC_ADC_DIGI_RESULT_BYTES)) {
        return false;
    }

    g_adc_lut[adc] = *adc_lut;

    adc_continuous_handle_cfg_t adc_handle_config = {0};

    adc_handle_config.max_store_buf_size = g_adc_lut[adc].buffer_size;
    adc_handle_config.conv_frame_size = g_adc_lut[adc].frames_count;
    adc_handle_config.flags.flush_pool = g_adc_lut[adc].flush_pool;

    if (ESP_OK != adc_continuous_new_handle(&adc_handle_config, &g_adc_dynamic[adc].handle)) {
        return false;
    }

    adc_continuous_config_t adc_config = {0};

    adc_config.pattern_num = g_adc_lut[adc].used_channel_num;
    adc_config.adc_pattern = (adc_digi_pattern_config_t *) g_adc_lut[adc].channel_desc;
    adc_config.sample_freq_hz = g_adc_lut[adc].sample_frequency_hz;
    adc_config.conv_mode = g_adc_lut[adc].conversion_mode;
    adc_config.format = g_adc_lut[adc].output_format;

    if (ESP_OK != adc_continuous_config(g_adc_dynamic[adc].handle, &adc_config)) {
        return false;
    }

    /* clang-format off */
    adc_continuous_evt_cbs_t adc_callbacks = {
        .on_conv_done = ADC_Driver_ConvDone_IRQHandler,
        .on_pool_ovf = ADC_Driver_PoolOvf_IRQHandler
    };
    /* clang-format on */

    if (ESP_OK != adc_continuous_register_event_callbacks(g_adc_dynamic[adc].handle, &adc_callbacks, &g_adc_dynamic[adc].adc)) {
        return false;
    }

    for (eAdcChannel_t channel = eAdcChannel_First; channel < eAdcChannel_Last; channel++) {
        g_adc_channel_dynamic[adc][channel].raw_data = g_adc_channel_buffers[adc][channel];
    }

    g_adc_dynamic[adc].adc = adc;
    g_adc_dynamic[adc].callback = adc_callback;
    g_adc_dynamic[adc].adc_data = g_adc_data_buffers[adc];
    g_adc_dynamic[adc].state = eAdcState_Initialized;

    return true;
}

bool ADC_Driver_Start(const eAdc_t adc) {
    if (!ADC_Config_IsCorrectAdc(adc)) {
        return false;
    }

    if (eAdcState_Initialized != g_adc_dynamic[adc].state) {
        return false;
    }

    if (ESP_OK != adc_continuous_start(g_adc_dynamic[adc].handle)) {
        return false;
    }

    g_adc_dynamic[adc].state = eAdcState_Running;

    return true;
}

bool ADC_Driver_Stop(const eAdc_t adc) {
    if (!ADC_Config_IsCorrectAdc(adc)) {
        return false;
    }

    if (eAdcState_Running != g_adc_dynamic[adc].state) {
        return false;
    }

    if (ESP_OK != adc_continuous_stop(g_adc_dynamic[adc].handle)) {
        return false;
    }

    g_adc_dynamic[adc].state = eAdcState_Initialized;

    return true;
}

bool ADC_Driver_Read(const eAdc_t adc) {
    if (!ADC_Config_IsCorrectAdc(adc)) {
        return false;
    }

    if (eAdcState_Running != g_adc_dynamic[adc].state) {
        return false;
    }

    uint32_t sample_num = 0;

    if (ESP_OK != adc_continuous_read_parse(g_adc_dynamic[adc].handle, g_adc_dynamic[adc].adc_data, g_adc_lut[adc].frames_count, &sample_num, ADC_READ_PARSE_DELAY)) {
        return false;
    }

    if (!ADC_Driver_BuildChannelData(g_adc_lut[adc], g_adc_dynamic[adc].adc_data, sample_num, g_adc_channel_dynamic[adc])) {
        return false;
    }

    return true;
}

bool ADC_Driver_GetChannelData(const eAdc_t adc, const eAdcChannel_t channel, uint32_t **data, size_t *size) {
    if (!ADC_Config_IsCorrectAdc(adc) || !ADC_Config_IsCorrectAdcChannel(channel)) {
        return false;
    }

    if ((NULL == data) || (NULL == size)) {
        return false;
    }

    if (eAdcState_Running != g_adc_dynamic[adc].state) {
        return false;
    }

    *data = g_adc_channel_dynamic[adc][channel].raw_data;
    *size = g_adc_channel_dynamic[adc][channel].size;

    return true;
}

bool ADC_Driver_ClearBuffer(const eAdc_t adc) {
    if (!ADC_Config_IsCorrectAdc(adc)) {
        return false;
    }

    if (eAdcState_Default == g_adc_dynamic[adc].state) {
        return false;
    }

    if (ESP_OK != adc_continuous_flush_pool(g_adc_dynamic[adc].handle)) {
        return false;
    }

    return true;
}

bool ADC_Driver_Calibrate(const eAdc_t adc) {
    if (!ADC_Config_IsCorrectAdc(adc)) {
        return false;
    }

    if (eAdcState_Initialized != g_adc_dynamic[adc].state) {
        return false;
    }

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t calib_config = {0};
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t calib_config = {0};
#endif /* ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED */

    for (eAdcChannel_t channel = eAdcChannel_First; channel < eAdcChannel_Last; channel++) {
        calib_config.unit_id = g_adc_lut[adc].channel_desc[channel].periph;
        calib_config.atten = g_adc_lut[adc].channel_desc[channel].attenuation;
        calib_config.bitwidth = g_adc_lut[adc].channel_desc[channel].bit_width;
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        calib_config.chan = g_adc_lut[adc].channel_desc[channel].channel;

        if (ESP_OK != adc_cali_create_scheme_curve_fitting(&calib_config, &g_adc_channel_dynamic[adc][channel].calib_handle)) {
            return false;
        }
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        calib_config.default_vref = g_adc_lut[adc].channel_desc[channel].v_ref;

        if (ESP_OK != adc_cali_create_scheme_line_fitting(&calib_config, &g_adc_channel_dynamic[adc][channel].calib_handle)) {
            return false;
        }
#endif /* ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED */
    }

    return true;
}

bool ADC_Driver_GetCalibrationVoltage(const eAdc_t adc, const eAdcChannel_t channel, const uint32_t raw_data, uint32_t *voltage) {
    if (!ADC_Config_IsCorrectAdc(adc) || !ADC_Config_IsCorrectAdcChannel(channel)) {
        return false;
    }

    if (NULL == voltage) {
        return false;
    }

    return (ESP_OK == adc_cali_raw_to_voltage(g_adc_channel_dynamic[adc][channel].calib_handle, raw_data, (int *) voltage));
}

#endif /* ENABLE_ADC */
