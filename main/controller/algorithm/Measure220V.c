//******************************************************************************
//include
//******************************************************************************
#include "adc_drv.h"
#include "parameters.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

#include "esp_adc/adc_continuous.h"

//******************************************************************************
// Constants
//******************************************************************************
#define HYST 0.02 // Гистерезис ±2%
#define PHASE_K 0.3

//******************************************************************************
// Type
//******************************************************************************
typedef enum {
    STATUS_PHASE_UNKNOW,
    STATUS_PHASE_IN_RANGE,
    STATUS_PHASE_IN_ABOVE,
    STATUS_PHASE_IN_BELOW,
} status_phase_t;

typedef enum {
    PHASE_UNKNOW,
    PHASE_IN_RANGE,
    PHASE_IN_ABOVE,
    PHASE_IN_RANGE_AFTER_IN_ABOVE,
    PHASE_IN_BELOW,
    PHASE_IN_RANGE_AFTER_IN_BELOW,
} phase_t;

typedef struct {
    uint32_t cnt;
    double summ;
} ESR_t;
//******************************************************************************
// Variables
//******************************************************************************
//------------------------------------------------------------------------------
// Global
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local Variable
//------------------------------------------------------------------------------
static char *TAG = "[voltage_220V]";

static TaskHandle_t hTask = NULL;
static SemaphoreHandle_t g_mutex = NULL;

ESR_t esr = {0};

static double zero_voltage_220V = 0;
static bool fSetZero220V = false;

static bool fCalibrate = false;
static double calibrate_voltage = 0;
static double calibrate_esr = 0;
static double voltage = 0;
//------------------------------------------------------------------------------
// Local Class
//------------------------------------------------------------------------------

//******************************************************************************
// Function prototype
//******************************************************************************
void voltage_220V_calculate_task(void *arg);

static inline bool in_range(double val, double ref, double hysteresis);
static inline bool in_above(double val, double ref, double min_lvl);
static inline bool in_below(double val, double ref, double min_lvl);
//******************************************************************************
// Function
//******************************************************************************
static void voltage_done_cb(uint16_t data) 
{ 
    static status_phase_t status_phase = STATUS_PHASE_UNKNOW; 
    static status_phase_t last_status_phase = STATUS_PHASE_UNKNOW; 
    static phase_t phase = PHASE_UNKNOW;
    static phase_t last_phase = PHASE_UNKNOW;
    
    if (xSemaphoreTake(g_mutex, portMAX_DELAY)) 
    { 
        double val = (double)data;
        esr.cnt++;

        if(fSetZero220V) 
            esr.summ += val;
        else
        {
            esr.summ += (val - zero_voltage_220V) * (val - zero_voltage_220V);
        }

        if(in_range(val, zero_voltage_220V, HYST)) 
            status_phase = STATUS_PHASE_IN_RANGE; 
        else if(in_above(val, zero_voltage_220V, PHASE_K)) 
            status_phase = STATUS_PHASE_IN_ABOVE; 
        else if(in_below(val, zero_voltage_220V, PHASE_K)) 
            status_phase = STATUS_PHASE_IN_BELOW; 
        else 
            status_phase = STATUS_PHASE_UNKNOW;
            
        if(status_phase != STATUS_PHASE_UNKNOW && 
            last_status_phase != status_phase) 
        { 
            if(status_phase == STATUS_PHASE_IN_RANGE && 
                (phase == PHASE_UNKNOW || phase == PHASE_IN_RANGE) && 
                (last_phase == PHASE_UNKNOW || last_phase == PHASE_IN_RANGE)) 
            { 
                phase = PHASE_IN_RANGE; 
            }
            //Прямая последовательность
            else if(status_phase == STATUS_PHASE_IN_ABOVE 
                && last_phase == PHASE_IN_RANGE) 
            {
                phase = PHASE_IN_ABOVE; 
            } 
            else if(status_phase == STATUS_PHASE_IN_RANGE && 
                last_phase == PHASE_IN_ABOVE) 
            { 
                phase = PHASE_IN_RANGE_AFTER_IN_ABOVE;
            } 
            else if(status_phase == STATUS_PHASE_IN_BELOW && 
                last_phase == PHASE_IN_RANGE_AFTER_IN_ABOVE) 
            { 
                phase = PHASE_IN_BELOW; 
            } 
            else if(status_phase == STATUS_PHASE_IN_RANGE && 
                last_phase == PHASE_IN_BELOW) 
            { 
                phase = PHASE_IN_RANGE_AFTER_IN_BELOW; 
                xTaskNotifyGive(hTask); 
            } 
            //Обратная последовательность
            else if(status_phase == STATUS_PHASE_IN_BELOW 
                && last_phase == PHASE_IN_RANGE) 
            {
                phase = PHASE_IN_BELOW; 
            } 
            else if(status_phase == STATUS_PHASE_IN_RANGE && 
                last_phase == PHASE_IN_BELOW) 
            { 
                phase = PHASE_IN_RANGE_AFTER_IN_BELOW;
            } 
            else if(status_phase == STATUS_PHASE_IN_ABOVE && 
                last_phase == PHASE_IN_RANGE_AFTER_IN_BELOW) 
            { 
                phase = PHASE_IN_ABOVE; 
            } 
            else if(status_phase == STATUS_PHASE_IN_RANGE && 
                last_phase == PHASE_IN_ABOVE) 
            { 
                phase = PHASE_IN_RANGE_AFTER_IN_ABOVE; 
                xTaskNotifyGive(hTask); 
            } 
            else // последовательность нарушена
            {  
                phase = PHASE_UNKNOW; 
            }

            last_status_phase = status_phase; 
            last_phase = phase; 
        } 
        xSemaphoreGive(g_mutex); 
    } 
}
//------------------------------------------------------------------------------
void Measure220V_init(void)
{
    meas_220V_settings_t calib;
    if(load_calibration_220V(&calib) == ESP_OK)
    {
        zero_voltage_220V = (double)calib.zero_lvl;
        calibrate_voltage = calib.ref_voltage;
        calibrate_esr = calib.ref_ESR;
    }
    else
        ESP_LOGE(TAG, "Failed load calibration");

    g_mutex = xSemaphoreCreateMutex();
    if (g_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }

    adc_channel_config_t config_220V = {
        .on_conv_done = voltage_done_cb,
        .channel = ADC_CHANNEL_6 
    };
    adc_init(&config_220V);

    ESP_LOGI(TAG, "Measure220V initialized");
}
//------------------------------------------------------------------------------
void voltage_220V_calculate_task(void *arg)
{
    bool flag_zero = false;
    bool flag_calib = false;
    uint32_t ticks = pdTICKS_TO_MS(xTaskGetTickCount());
    uint32_t last_ticks = pdTICKS_TO_MS(xTaskGetTickCount());

    printf("Uptime: %lu ms\n", ticks);
    while(1)
    {
        uint32_t notification = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(20));
        
        //---------------------------------------------------------------------
        //calibration zero
        if (xSemaphoreTake(g_mutex, portMAX_DELAY)) 
        {
            flag_zero = fSetZero220V;
            flag_calib = fCalibrate;
            xSemaphoreGive(g_mutex); 
        } 

        if(flag_zero) 
        {
            if (xSemaphoreTake(g_mutex, portMAX_DELAY)) 
            {
                esr.cnt = 0; 
                esr.summ = 0; 
                xSemaphoreGive(g_mutex); 
            }

            xTaskNotifyStateClear(NULL);
            vTaskDelay(pdMS_TO_TICKS(1000));

            if (xSemaphoreTake(g_mutex, portMAX_DELAY)) 
            {
                zero_voltage_220V = esr.summ / esr.cnt;
                esr.cnt = 0; 
                esr.summ = 0;
                fSetZero220V = false; 
                xSemaphoreGive(g_mutex); 
            }            
        }
        //---------------------------------------------------------------------
        //calibration
        else if(flag_calib) 
        {
            meas_220V_settings_t calib;
            if (xSemaphoreTake(g_mutex, portMAX_DELAY)) 
            {
                esr.cnt = 0; 
                esr.summ = 0; 
                xSemaphoreGive(g_mutex); 
            }

            xTaskNotifyStateClear(NULL);
            vTaskDelay(pdMS_TO_TICKS(2000));

            if (xSemaphoreTake(g_mutex, portMAX_DELAY)) 
            {
                calibrate_esr = esr.summ / esr.cnt;
                esr.cnt = 0; 
                esr.summ = 0;
                fCalibrate = false; 

                calib.ref_ESR = calibrate_esr;
                calib.ref_voltage = calibrate_voltage;
                calib.zero_lvl = (uint16_t)zero_voltage_220V;
                xSemaphoreGive(g_mutex); 
            }
            save_calibration_220V(&calib);
        }
        
        //---------------------------------------------------------------------
        else
        {
            ticks = pdTICKS_TO_MS(xTaskGetTickCount());
            if (ticks - last_ticks > 15)
            {
                if (xSemaphoreTake(g_mutex, portMAX_DELAY)) 
                {
                    voltage = (esr.summ / esr.cnt) * calibrate_voltage / calibrate_esr;
                    esr.cnt = 0; 
                    esr.summ = 0;
                    xSemaphoreGive(g_mutex);                    
                }
                ESP_LOGI(TAG, "voltage 220V: %f", voltage);
            }
            else
            {
                if (xSemaphoreTake(g_mutex, portMAX_DELAY)) 
                {
                    esr.cnt = 0; 
                    esr.summ = 0;
                    xSemaphoreGive(g_mutex); 
                }
                ESP_LOGI(TAG, "voltage 220V: %f", voltage);
            }
        }
        last_ticks = ticks;
    }
}
//------------------------------------------------------------------------------
bool zero_calibrate_220V(bool start)
{
    bool ret;

    xSemaphoreTake(g_mutex, portMAX_DELAY);
    if(start) fSetZero220V = true;
    ret = fSetZero220V == false;
    xSemaphoreGive(g_mutex); 

    return ret;
}
//------------------------------------------------------------------------------
bool calibrate_220V(bool start)
{
    bool ret;

    xSemaphoreTake(g_mutex, portMAX_DELAY);
    if(start) fCalibrate = true;
    ret = fCalibrate == false;
    xSemaphoreGive(g_mutex);

    return ret;
}
//------------------------------------------------------------------------------
float get_voltage_220V(bool start)
{
    float result;

    xSemaphoreTake(g_mutex, portMAX_DELAY);
    result = (float)voltage;
    xSemaphoreGive(g_mutex);

    return result;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Auxiliary functions
//------------------------------------------------------------------------------
static inline bool in_range(double val, double ref, double hysteresis)
{
    return (val >= ref * (1.0 - hysteresis) && val <= ref * (1.0 + hysteresis));
}
//------------------------------------------------------------------------------
static inline bool in_above(double val, double ref, double min_lvl)
{
    return (val >= ref * (1.0 + min_lvl));
}
//------------------------------------------------------------------------------
static inline bool in_below(double val, double ref, double min_lvl)
{
    return (val <= ref * (1.0 - min_lvl));
}
//------------------------------------------------------------------------------