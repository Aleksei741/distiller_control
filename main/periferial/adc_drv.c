//******************************************************************************
//include
//******************************************************************************
#include "adc_drv.h"
#include "esp_log.h"
#include <math.h>
#include <stdlib.h>
#include "esp_adc/adc_continuous.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//******************************************************************************
// Constants
//******************************************************************************
#define ADC_BUFFER_LEN 256
#define MAX_ADC_CHANNELS 3

//******************************************************************************
// Type
//******************************************************************************

//******************************************************************************
// Variables
//******************************************************************************
//------------------------------------------------------------------------------
// Global
//------------------------------------------------------------------------------
static const char* TAG = "[ADC_driver]";
//------------------------------------------------------------------------------
// Local Variable
//------------------------------------------------------------------------------
static adc_continuous_handle_t hADC = NULL;
static TaskHandle_t hTask = NULL;
static uint8_t adc_result[ADC_BUFFER_LEN] = {0};

adc_channel_config_t adc_channel_configs[SOC_ADC_PATT_LEN_MAX] = {0};
uint8_t adc_channel_count = 0;
//------------------------------------------------------------------------------
// Local Class
//------------------------------------------------------------------------------

//******************************************************************************
// Function prototype
//******************************************************************************
void adc_driver_init(void);
void adc_deinit(void);

void ADC_calculate_task(void *arg);
//******************************************************************************
// Function
//******************************************************************************
static bool IRAM_ATTR conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    vTaskNotifyGiveFromISR((TaskHandle_t)user_data, &mustYield);
    return (mustYield == pdTRUE);
}
//------------------------------------------------------------------------------
void adc_init(adc_channel_config_t *config)
{
    if (adc_channel_count == 0) 
    {
        adc_deinit();
    }

    if (adc_channel_count >= SOC_ADC_PATT_LEN_MAX) return; // защита

    adc_channel_configs[adc_channel_count].on_conv_done = config->on_conv_done;
    adc_channel_configs[adc_channel_count].channel = config->channel;
    adc_channel_count++;

    ESP_LOGI(TAG, "adc_init, ADC channel %d initialized", config->channel);

    adc_driver_init();
}
//------------------------------------------------------------------------------
void adc_driver_init(void)
{
    uint8_t cnt;
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 4096,
        .conv_frame_size = ADC_BUFFER_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &hADC));

    adc_continuous_config_t dig_cfg = {
            .sample_freq_hz = 20000,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = adc_channel_count;
    ESP_LOGI(TAG, "adc_driver_init, dig_cfg.pattern_num %d", dig_cfg.pattern_num);

    for(cnt = 0; cnt < adc_channel_count; cnt++)
    {
        adc_pattern[cnt].atten = ADC_ATTEN_DB_12;
        adc_pattern[cnt].channel = adc_channel_configs[cnt].channel;
        adc_pattern[cnt].unit = ADC_UNIT_1;
        adc_pattern[cnt].bit_width = ADC_BITWIDTH_12;

        ESP_LOGI(TAG, "atten %d, channel %d, unit %d, bit_width %d", adc_pattern[cnt].atten,  
            adc_pattern[cnt].channel, adc_pattern[cnt].unit, adc_pattern[cnt].bit_width);
    }
       
    dig_cfg.adc_pattern = adc_pattern;
    ESP_LOGI(TAG, "Частота дискретизации: %d Гц", dig_cfg.sample_freq_hz);
    ESP_LOGI(TAG, "Количество каналов: %d", dig_cfg.pattern_num);
    ESP_ERROR_CHECK(adc_continuous_config(hADC, &dig_cfg));

    xTaskCreate(ADC_calculate_task, "ADC_calculate_task", 8192, NULL, 5, &hTask);   

    adc_continuous_evt_cbs_t cbs = 
    {
        .on_conv_done = conv_done_cb,
    };

    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(hADC, &cbs, hTask));
    ESP_ERROR_CHECK(adc_continuous_start(hADC));

    ESP_LOGI(TAG, "AВС initialized");
}
//------------------------------------------------------------------------------
void adc_deinit(void)
{
    if(hTask != NULL)
    {
        vTaskDelete(hTask);  // удаляем задачу
        hTask = NULL;
    }
    
    if (hADC != NULL) 
    {
        adc_continuous_stop(hADC);
        adc_continuous_deinit(hADC);
        hADC = NULL;
    }

    ESP_LOGI(TAG, "adc_deinit, ADC deinitialized");
}
//------------------------------------------------------------------------------
void ADC_calculate_task(void *arg)
{
    uint8_t cnt, ch;
    
    // Динамически выделяем память для массива данных
    adc_continuous_data_t *data = malloc(ADC_BUFFER_LEN / SOC_ADC_DIGI_RESULT_BYTES * sizeof(adc_continuous_data_t));
    if (data == NULL) {
        ESP_LOGE(TAG, "Не удалось выделить память для данных АЦП");
        vTaskDelete(NULL);
    }
    
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint32_t ret_num = 0;
        esp_err_t ret = adc_continuous_read(hADC, adc_result, ADC_BUFFER_LEN, &ret_num, 0);
        if (ret == ESP_OK) 
        {
            uint32_t num_parsed_samples = 0;
            esp_err_t parse_ret = adc_continuous_parse_data(hADC, adc_result, ret_num, data, &num_parsed_samples);
            if (parse_ret == ESP_OK) 
            {   
                for(cnt = 0; cnt < num_parsed_samples; cnt++)
                {
                    for(ch = 0; ch < adc_channel_count; ch++)
                    {
                        if(data[cnt].valid && data[cnt].unit == ADC_UNIT_1 && data[cnt].channel == adc_channel_configs[ch].channel)
                        {
                            if (adc_channel_configs[ch].on_conv_done)
                                adc_channel_configs[ch].on_conv_done((uint16_t)data[cnt].raw_data);
                        }
                    }
                }
            }
        }
    }
    
    // Освобождаем память при завершении задачи
    free(data);
}
//------------------------------------------------------------------------------
