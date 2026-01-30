//******************************************************************************
//include
//******************************************************************************
#include "autoclave_mode.h"
#include <ten_ctrl.h>
#include <parameters.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//******************************************************************************
// Constants
//******************************************************************************
#define DEFAULT_VALUE 0xff
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
static char *TAG = "[autoclave_mode]";
static float need_temperature = 0;
static PID_t pid_controller;
static SemaphoreHandle_t g_mutex = NULL;
//------------------------------------------------------------------------------
// Local Class
//------------------------------------------------------------------------------

//******************************************************************************
// Function prototype
//******************************************************************************
bool autoclave_mode_init();
//******************************************************************************
// Function
//******************************************************************************
void autoclave_mode_set_need_temperature(float p)
{
    if(g_mutex == NULL)
        return;

    if(p > 130.0) p = 130.0;
    need_temperature = p;
    ESP_LOGI(TAG, "Set autoclave mode temperature to: %.2f C", p);
}
//------------------------------------------------------------------------------
void autoclave_mode_set_parameters_pid(const PID_t *pid)
{
    if(pid == NULL || g_mutex == NULL)
        return;
    
    if(!autoclave_mode_init()) return;

    if(xSemaphoreTake(g_mutex, portMAX_DELAY))
    {
        memcpy(&pid_controller.param, &pid->param, sizeof(pid_controller.param));
        xSemaphoreGive(g_mutex);

        save_autoclave_mode_pid_parameters(&pid_controller);
        ESP_LOGI(TAG, "Set autoclave mode PID parameters");
    }
}
//------------------------------------------------------------------------------
void autoclave_mode_get_pid(PID_t *pid)
{
    if(pid == NULL || g_mutex == NULL)
        return;

    if(!autoclave_mode_init()) return;

    if(xSemaphoreTake(g_mutex, portMAX_DELAY))
    {
        memcpy(pid, &pid_controller, sizeof(PID_t));
        xSemaphoreGive(g_mutex);
    }
}
//------------------------------------------------------------------------------
void autoclave_mode_process(dc_status_t* status)
{
    static uint32_t pid_last_ticks = 0;

    if(status == NULL)
        return;

    if(status->mode != DC_MODE_AUTOCLAVE)
    {
        need_temperature = 0;
        return;
    }

    if(!autoclave_mode_init()) return;

    if(xSemaphoreTake(g_mutex, portMAX_DELAY))
    {
        uint32_t ticks = pdTICKS_TO_MS(xTaskGetTickCount());

        uint32_t timePeriod = (uint32_t)(PID_GetDt(&pid_controller) * 1000.0f); // в миллисекундах
        if(ticks - pid_last_ticks >= timePeriod)
        {
            float dt = (ticks - pid_last_ticks) / 1000.0f; // в секундах
            pid_last_ticks = ticks; 

            float current_temperature = status->temperature_kube;
            float output = PID_calc(&pid_controller, need_temperature, current_temperature, dt);

            ESP_LOGI(TAG, "Autoclave mode: need_temp=%dC current_temp=%.2fC heater_power=%.2f%%",
                need_temperature, current_temperature, output);

            heater_set_power((uint8_t)output);
        }
        xSemaphoreGive(g_mutex);
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Local Function
//------------------------------------------------------------------------------
bool autoclave_mode_init()
{
    static bool flagInit = false;
    if(!flagInit)
    {        
        g_mutex = xSemaphoreCreateMutex();
        if (g_mutex == NULL) 
        {
            ESP_LOGE(TAG, "Failed to create mutex");
            return false;
        }

        if(xSemaphoreTake(g_mutex, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "Autoclave mode started");
            flagInit = true;
            need_temperature = 0;
            if(load_autoclave_mode_pid_parameters(&pid_controller) != ESP_OK)
            {
                ESP_LOGW(TAG, "Using default PID parameters for autoclave mode");
                // Default parameters
                PID_Init(&pid_controller, 100.0f, 40.0f, 0.0f);
                PID_SetOutputLimits(&pid_controller, 0.0f, 100.0f);
                PID_SetIntegralLimits(&pid_controller, -50.0f, 50.0f);
                PID_SetDFilter(&pid_controller, 0.1f);
                PID_SetDt(&pid_controller, 10.0f);
            }
            xSemaphoreGive(g_mutex);
        }
        return true;
    }

    return false;
}
//------------------------------------------------------------------------------
