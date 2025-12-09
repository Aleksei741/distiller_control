//******************************************************************************
//include
//******************************************************************************
#include <string.h>

#include "distiller_control.h"
#include "temperature_sensor.h"
#include <parameters.h>
#include <wifi_control.h>
#include <webServer.h>
#include <led.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//******************************************************************************
// Constants
//******************************************************************************

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
static char *TAG = "[distiller_control]";

static SemaphoreHandle_t g_mutex = NULL;
static dc_status_t dc_status;
//------------------------------------------------------------------------------
// Local Class
//------------------------------------------------------------------------------

//******************************************************************************
// Function prototype
//******************************************************************************
void distiller_control_task(void *arg);
//******************************************************************************
// Function
//******************************************************************************
void init_distiller_control()
{
    ESP_LOGI(TAG, "Initializing distiller control...");
    
    parameters_init();    
    led_init();    
    init_temperature_sensor();
    wifi_init();
    init_webserver();
    start_webserver();

    g_mutex = xSemaphoreCreateMutex();
    if (g_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }

    xTaskCreate(distiller_control_task, "distiller_control_task", 4096, NULL, 5, NULL);
}
//------------------------------------------------------------------------------
void distiller_control_task(void *arg)
{
    dc_status_t status;
    dc_mode_e mode = DC_MANUAL_CONTROL;

    

    while (1)
    {
        get_column_temperature(&status.temperature_column);
        get_kube_temperature(&status.temperature_kube);
        get_radiator_temperature(&status.temperature_radiator);

        if (xSemaphoreTake(g_mutex, portMAX_DELAY)) 
        {
            memcpy(&dc_status, &status, sizeof(dc_status_t));
            xSemaphoreGive(g_mutex);
        }

        // Получаем время в тиках RTOS
    TickType_t ticks = xTaskGetTickCount();
    // Переводим в миллисекунды
    uint32_t ms = ticks * portTICK_PERIOD_MS;

        ESP_LOGI(TAG, "Time %u ms - Column Temp: %.2f C, Kube Temp: %.2f C, Radiator Temp: %.2f C", 
            ms,
            status.temperature_column, 
            status.temperature_kube, 
            status.temperature_radiator);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool get_dc_parameters(dc_parametes_e param, void *out_params)
{
    if(out_params == NULL)
        return false;

    if(param == DC_COLUMN_ROM)
    {
        get_column_rom((uint8_t *)out_params);
        return true;
    }
    else if(param == DC_KUBE_ROM)
    {
        get_kube_rom((uint8_t *)out_params);
        return true;
    }
    return false;
}
//------------------------------------------------------------------------------
bool set_dc_parameters(dc_parametes_e param, void *in_params)
{
    if(in_params == NULL)
        return false;

    // Заглушка - параметры только для чтения
    return false;
}
//------------------------------------------------------------------------------
bool get_dc_status(dc_status_t *out_status)
{
    if (out_status == NULL)
        return false;

    if (xSemaphoreTake(g_mutex, portMAX_DELAY)) 
    {
        out_status->temperature_column = dc_status.temperature_column;
        out_status->temperature_kube = dc_status.temperature_kube;
        out_status->temperature_radiator = dc_status.temperature_radiator;
        xSemaphoreGive(g_mutex);
    }
    return true;
}
//------------------------------------------------------------------------------
void send_dc_command(dc_command_e command)
{
    if(command == DC_REQUEST_INIT_COLUMN_ROM)
    {
        request_init_column_rom();
    }
    else if(command == DC_REQUEST_INIT_KUBE_ROM)
    {
        request_init_kube_rom();
    }
}
//------------------------------------------------------------------------------
