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
#include <Measure220V.h>
#include <statistic.h>
#include <statistic_sampler.h>
#include <ten_ctrl.h>
#include <buttons.h>
#include <radiator_fan.h>

#include <manual_control.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
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
static char *TAG = "[distiller_control]";

static SemaphoreHandle_t g_mutex = NULL;
static dc_status_t dc_status;

QueueHandle_t qCommand;
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
    init_temperature_sensor();
    wifi_init();
    init_webserver();
    start_webserver();
    psram_statistic_init();
    heater_init();
    init_button_reset_wifi_ap(); // Обязательно после heater_init
    init_fan();
    
    g_mutex = xSemaphoreCreateMutex();
    if (g_mutex == NULL) 
    {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }

    qCommand = xQueueCreate(40, sizeof(dc_input_command_t));
    if (qCommand == NULL)
        ESP_LOGE(TAG, "Failed to create qCommand");

    xTaskCreate(distiller_control_task, "distiller_control_task", 4096, NULL, 5, NULL);
}
//------------------------------------------------------------------------------
void distiller_control_task(void *arg)
{
    dc_input_command_t cmd;
    dc_status_t status;    
    heater_set_power(0);
    uint8_t flag_temperature = 0;

    memset(&status, 0, sizeof(status));
    status.mode = DC_MANUAL_CONTROL;

    while (1)
    {
        get_radiator_temperature(&status.temperature_radiator);

        flag_temperature |= get_column_temperature(&status.temperature_column) ? 1 : 0;
        flag_temperature |= get_kube_temperature(&status.temperature_kube) ? 2 : 0;                
        if((flag_temperature & 3) == 3)
            statistic_sampler(status.temperature_column, status.temperature_kube, status.ten_power);

        if (xSemaphoreTake(g_mutex, portMAX_DELAY)) 
        {
            memcpy(&dc_status, &status, sizeof(dc_status_t));            
            xSemaphoreGive(g_mutex);
        }

        if(get_button_prees_reset_wifi_ap())
            set_default_wifi_ap_settings();

        status.fan = fan_process(status.temperature_radiator);

        while(xQueueReceive(qCommand, &cmd, 0) == pdTRUE)
        {
            switch(cmd.command) 
            {
                case DC_REQUEST_INIT_COLUMN_ROM:
                    request_init_column_rom();
                    break;
                case DC_REQUEST_INIT_KUBE_ROM:
                    request_init_kube_rom();
                    break;
                case DC_SET_TEN_POWER:
                    if(status.mode == DC_MANUAL_CONTROL)
                        manual_control_set_heater_power(cmd.valuei);
                    break;
                case DC_SET_MODE:
                    status.mode = (dc_mode_e)cmd.valuei;
                    break;
                default:
                    ESP_LOGW(TAG, "Unknown command: %d", cmd.command);
                    break;
            }
        }

        if(status.mode == DC_MANUAL_CONTROL)
            manual_control_process(&status);
        else
            manual_control_set_heater_power(0);
        
        

        vTaskDelay(pdMS_TO_TICKS(100));
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
        memcpy(out_status, &dc_status, sizeof(dc_status_t));
        xSemaphoreGive(g_mutex);
    }
    return true;
}
//------------------------------------------------------------------------------
void send_dc_command(dc_command_e command, void* value)
{
    dc_input_command_t cmd = {0}; // Инициализируем нулями (особенно union)
    cmd.command = command;

    if(value != NULL)
    {
        if (command == DC_SET_TEN_POWER)
            cmd.valuei = *(uint32_t*)value;
        else if(command == DC_SET_MODE)
            cmd.valuei = *(uint32_t*)value;
    }

    if (xQueueSend(qCommand, &cmd, 0) != pdTRUE) {
        ESP_LOGE(TAG, "Command %d dropped (queue full)", (int)command);
    }
}
//------------------------------------------------------------------------------
