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
#include "freertos/semphr.h"

//******************************************************************************
// Constants
//******************************************************************************
#define ADC_BUFFER_LEN 1024
#define ADC_CHANNEL_220V ADC_CHANNEL_6
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

static uint16_t zero_voltage_220V = 0;
static bool fSetZero220V = false;
static bool f220VActive = false;
static bool f220VDeactive = false;
static bool f220VMeasure = false;
double ESR_summ = 0;
uint32_t ESR_cnt = 0;
double ESR_voltage_220v = 0;
//------------------------------------------------------------------------------
// Local Class
//------------------------------------------------------------------------------

//******************************************************************************
// Function prototype
//******************************************************************************
static bool conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data);
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
void adc_init(void)
{
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 4096,
        .conv_frame_size = ADC_BUFFER_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &hADC));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = 2000,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = 1;

    //Настройка 220В
    adc_pattern[0].atten = ADC_ATTEN_DB_12;
    adc_pattern[0].channel = ADC_CHANNEL_220V;
    adc_pattern[0].unit = ADC_UNIT_1;
    adc_pattern[0].bit_width = ADC_BITWIDTH_12;

       
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(hADC, &dig_cfg));

    adc_continuous_evt_cbs_t cbs = 
    {
        .on_conv_done = conv_done_cb,
    };

    xTaskCreate(ADC_calculate_task, "ADC_calculate_task", 8192, NULL, 5, &hTask);   

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
    
    if(hADC != NULL)
    {
        ESP_ERROR_CHECK(adc_continuous_deinit(hADC));
        hADC = NULL;
    }
}
//------------------------------------------------------------------------------
void ADC_calculate_task(void *arg)
{
    int cnt;
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint32_t ret_num = 0;
        esp_err_t ret = adc_continuous_read(hADC, adc_result, ADC_BUFFER_LEN, &ret_num, 0);
        if (ret == ESP_OK) 
        {
            adc_continuous_data_t data[ADC_BUFFER_LEN / SOC_ADC_DIGI_RESULT_BYTES];
            uint32_t num_parsed_samples = 0;

            esp_err_t parse_ret = adc_continuous_parse_data(hADC, adc_result, ret_num, data, &num_parsed_samples);
            if (parse_ret == ESP_OK) 
            {   
                //Калибровка нуля.
                if(fSetZero220V)
                {
                    uint32_t sun_zero = 0;
                    uint16_t sum_cnt = 0;
                    for(cnt = 0; cnt < num_parsed_samples; cnt++)
                    {
                        //ADC_CHANNEL_220V
                        if(data[cnt].channel == ADC_CHANNEL_220V && data[cnt].valid)
                        {
                            sun_zero += data[cnt].raw_data;
                            sum_cnt++;
                        }
                    }
                    zero_voltage_220V = sun_zero / sum_cnt;
                    fSetZero220V = false;
                    continue;
                }

                for(cnt = 0; cnt < num_parsed_samples; cnt++)
                {
                    //ADC_CHANNEL_220V
                    if(data[cnt].channel == ADC_CHANNEL_220V && data[cnt].valid)
                    {
                        if(f220VMeasure)
                        {
                            double val = (double)data[cnt].raw_data;
                            ESR_summ += val * val;
                            ESR_cnt++;
                            
                            if(f220VDeactive)
                            {
                                if(data[cnt].raw_data < zero_voltage_220V * 1.02 &&
                                   data[cnt].raw_data > zero_voltage_220V * 0.98)
                                {
                                    f220VMeasure = false;
                                    ESR_voltage_220v = sqrt(ESR_summ / ESR_cnt);
                                }
                                f220VDeactive = false;
                            }
                        } 
                        else if(f220VActive)
                        {
                            if(data[cnt].raw_data > zero_voltage_220V * 1.02 ||
                               data[cnt].raw_data < zero_voltage_220V * 0.98)
                            {                                
                                double val = (double)data[cnt].raw_data;
                                ESR_summ += val * val;
                                ESR_cnt = 1;
                                f220VActive = false;
                                f220VMeasure = true;
                            }
                        }
                    }
                }
            }
        }
    }
}
//------------------------------------------------------------------------------
void SetZero220V(void)
{
    fSetZero220V = true;
}
//------------------------------------------------------------------------------
void adc_220V_active(bool moc_active)
{
    if(moc_active)
        f220VActive = true;
    else
        f220VDeactive = true;
}