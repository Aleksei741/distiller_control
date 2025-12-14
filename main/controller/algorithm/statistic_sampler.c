//******************************************************************************
//include
//******************************************************************************
#include <math.h>

#include "statistic_sampler.h"
#include <statistic.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//******************************************************************************
// Constants
//******************************************************************************
#define MIN_DELAY 1000
#define MIN_PERIOD_SAVE_MS 60000
#define HEATER_TOLERANCE 10 //%
#define COLUMN_TEMP_TOLERANCE 1 //град
#define KUBE_TEMP_TOLERANCE 3 //град
//******************************************************************************
// Type
//******************************************************************************

//******************************************************************************
// Variables
//******************************************************************************
//------------------------------------------------------------------------------
// Global
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local Variable
//------------------------------------------------------------------------------
static const char *TAG = "STAT_SAMPLER";

static statistic_t statistic;
static bool first_sample = true;
//******************************************************************************
// Function prototype
//******************************************************************************
bool heater_level_exceeds_tolerance(uint8_t current_level, uint8_t saved_level, uint8_t tolerance);
bool temperature_exceeds_tolerance(float current_temp, float saved_temp, float tolerance);
//******************************************************************************
// Function
//******************************************************************************
void statistic_sampler(float temperature_column, float temperature_kube, uint8_t heater)
{
    uint32_t time = pdTICKS_TO_MS(xTaskGetTickCount());

    if (first_sample)
    {
        statistic.temperature_column = temperature_column;
        statistic.temperature_kube  = temperature_kube;
        statistic.heater            = heater;
        statistic.time              = time;
        psram_statistic_push(&statistic);
        first_sample = false;
        return;
    }

    if(time > statistic.time + MIN_DELAY)
    {
        // Проверяем, прошло ли достаточно времени ИЛИ изменились параметры
        bool time_expired = (time - statistic.time) >= MIN_PERIOD_SAVE_MS;
        bool heater_changed = heater_level_exceeds_tolerance(heater, statistic.heater, HEATER_TOLERANCE);
        bool column_temp_changed = temperature_exceeds_tolerance(temperature_column, statistic.temperature_column, COLUMN_TEMP_TOLERANCE);
        bool kube_temp_changed = temperature_exceeds_tolerance(temperature_kube, statistic.temperature_kube, KUBE_TEMP_TOLERANCE);
    
        if (time_expired || heater_changed || column_temp_changed || kube_temp_changed)
        {
            statistic.heater = heater;
            statistic.temperature_column = temperature_column;
            statistic.temperature_kube = temperature_kube;
            statistic.time = time;
            psram_statistic_push(&statistic);
            ESP_LOGD(TAG, "Sample saved: heater=%u, col=%.1f, kube=%.1f", heater, temperature_column, temperature_kube);
        }
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Вспомогательные функции
//------------------------------------------------------------------------------
bool heater_level_exceeds_tolerance(uint8_t current_level, uint8_t saved_level, uint8_t tolerance)
{
    // Вычисляем абсолютную разницу между уровнями
    // Так как uint8_t беззнаковый, приводим к int, чтобы избежать проблем при вычитании
    int diff = (int)current_level - (int)saved_level;
    int abs_diff = abs(diff);

    // Сравниваем с допуском
    return abs_diff > (int)tolerance;
}
//------------------------------------------------------------------------------
bool temperature_exceeds_tolerance(float current_temp, float saved_temp, float tolerance)
{
    float diff = current_temp - saved_temp;
    float abs_diff = fabsf(diff);  // fabsf — для float (быстрее и точнее, чем fabs)
    return abs_diff > tolerance;
}