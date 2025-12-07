//******************************************************************************
//include
//******************************************************************************
#include <stdio.h>

#include "temperature_sensor.h"
#include "ds18b20.h"
#include "parameters.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
//******************************************************************************
// Constants
//******************************************************************************
#define DS18B20_COLUMN_PIN 16
#define DS18B20_RADIATOR_PIN 4
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
static const char *TAG = "[temperature_sensor]";
static ds18b20_t driver_column = { .pin = DS18B20_COLUMN_PIN };
static ds18b20_t driver_kube = { .pin = DS18B20_COLUMN_PIN };
static ds18b20_t driver_radiator = { .pin = DS18B20_RADIATOR_PIN };

static SemaphoreHandle_t g_temp_mutex = NULL;
static float column_temperature = 0.0f;
static float kube_temperature = 0.0f;
static float radiator_temperature = 0.0f;

bool fReadrColumnROM = false;
bool fReadrKubeROM = false;
//------------------------------------------------------------------------------
// Local Class
//------------------------------------------------------------------------------

//******************************************************************************
// Function prototype
//******************************************************************************
void temperature_task(void *arg);
//******************************************************************************
// Function
//******************************************************************************
void init_temperature_sensor()
{
    ESP_LOGI(TAG, "Temperature sensors initialized running...");
    init_ds18b20_gpio(&driver_column);
    init_ds18b20_gpio(&driver_radiator);

    if(load_column_rom_temperature_sensor(driver_column.rom) == ESP_OK)
        driver_column.fInitialized = true;
    
    if(load_kube_rom_temperature_sensor(driver_kube.rom) == ESP_OK)
        driver_kube.fInitialized = true;
       
    g_temp_mutex = xSemaphoreCreateMutex();
    if (g_temp_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }

    xTaskCreate(temperature_task, "temperature_task", 4096, NULL, 5, NULL);    
    ESP_LOGI(TAG, "Temperature sensors initialized");
}
//------------------------------------------------------------------------------
void temperature_task(void *arg)
{
    vTaskDelay(pdMS_TO_TICKS(100)); // Даем время на инициализацию
    
    while (1)
    {
        float t;
        bool fReadrColumnROM_ = 0;
        bool fReadrKubeROM_ = 0;

        if (xSemaphoreTake(g_temp_mutex, portMAX_DELAY)) 
        {
            fReadrColumnROM_ = fReadrColumnROM;
            fReadrKubeROM_ = fReadrKubeROM;
            fReadrColumnROM = false;
            fReadrKubeROM = false;
            xSemaphoreGive(g_temp_mutex);
        }

        if(fReadrKubeROM_)
        {
            if(ds18b20_read_rom(&driver_kube) == false) 
                ESP_LOGW(TAG, "Failed to read kube temperature");
            else
            {
                ESP_LOGI(TAG, "Kube ROM read successfully: %02X %02X %02X %02X %02X %02X %02X %02X",
                    driver_kube.rom[0], driver_kube.rom[1], driver_kube.rom[2], driver_kube.rom[3],
                    driver_kube.rom[4], driver_kube.rom[5], driver_kube.rom[6], driver_kube.rom[7]);
                save_kube_rom_temperature_sensor(driver_kube.rom);
                driver_kube.fInitialized = true;
                fReadrKubeROM_ = false;
            }        
        }

        if(fReadrColumnROM_)
        {
            if(ds18b20_read_rom(&driver_column) == false) 
                ESP_LOGW(TAG, "Failed to read column temperature");
            else
            {
                ESP_LOGI(TAG, "Column ROM read successfully: %02X %02X %02X %02X %02X %02X %02X %02X",
                    driver_column.rom[0], driver_column.rom[1], driver_column.rom[2], driver_column.rom[3],
                    driver_column.rom[4], driver_column.rom[5], driver_column.rom[6], driver_column.rom[7]);
                save_column_rom_temperature_sensor(driver_column.rom);
                driver_column.fInitialized = true;
                fReadrColumnROM_ = false;
            }        
        }
         
        if(driver_column.fInitialized)
        {
            if(ds18b20_read_temp_rom(&driver_column, &t) == false)
                ESP_LOGW(TAG, "Failed to read column temperature");
            else
                if (xSemaphoreTake(g_temp_mutex, portMAX_DELAY)) 
                {
                    column_temperature = t;
                    xSemaphoreGive(g_temp_mutex);
                }
        }

        if(driver_kube.fInitialized)
        {
            if(ds18b20_read_temp_rom(&driver_kube, &t) == false)
                ESP_LOGW(TAG, "Failed to read kube temperature");
            else
                if (xSemaphoreTake(g_temp_mutex, portMAX_DELAY)) 
                {
                    kube_temperature = t;
                    xSemaphoreGive(g_temp_mutex);
                }
        }

        if(ds18b20_read_temp_single(&driver_radiator, &t) == false)
        {
            ESP_LOGW(TAG, "Failed to read radiator temperature");
        }
        else
        {
            if (xSemaphoreTake(g_temp_mutex, portMAX_DELAY)) 
            {
                radiator_temperature = t;
                xSemaphoreGive(g_temp_mutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // раз в секунду
    }
}
//------------------------------------------------------------------------------
float get_column_temperature()
{
    float temp = 0.0f;
    if (g_temp_mutex != NULL) 
    {
        if (xSemaphoreTake(g_temp_mutex, portMAX_DELAY)) 
        {
            temp = column_temperature;
            xSemaphoreGive(g_temp_mutex);
        }
    }

    return temp;
}
//------------------------------------------------------------------------------
float get_kube_temperature()
{
    float temp = 0.0f;
    if (g_temp_mutex != NULL) 
    {
        if (xSemaphoreTake(g_temp_mutex, portMAX_DELAY)) 
        {
            temp = kube_temperature;
            xSemaphoreGive(g_temp_mutex);
        }
    }
        
    return temp;
}
//------------------------------------------------------------------------------
float get_radiator_temperature()
{
    float temp = 0.0f;
    if (g_temp_mutex != NULL) 
    {
        if (xSemaphoreTake(g_temp_mutex, portMAX_DELAY)) 
        {
            temp = radiator_temperature;
            xSemaphoreGive(g_temp_mutex);
        }
    }
        
    return temp;
}
//------------------------------------------------------------------------------
void request_init_column_rom(void)
{
    if (g_temp_mutex != NULL) 
    {
        if (xSemaphoreTake(g_temp_mutex, 10)) 
        {
            fReadrColumnROM = true;
            xSemaphoreGive(g_temp_mutex);
        }
    }
}
//------------------------------------------------------------------------------
void request_init_kube_rom(void)
{
    if (g_temp_mutex != NULL) 
    {
        if (xSemaphoreTake(g_temp_mutex, 10)) 
        {
            fReadrKubeROM = true;
            xSemaphoreGive(g_temp_mutex);
        }
    }
}
//------------------------------------------------------------------------------
bool get_column_rom(uint8_t *out_rom)
{
    if(driver_column.fInitialized)
    {
        for(int i=0; i<8; i++)
            out_rom[i] = driver_column.rom[i];
        return true;
    }
    return false;
}
//------------------------------------------------------------------------------
bool get_kube_rom(uint8_t *out_rom)
{
    if(driver_kube.fInitialized)
    {
        for(int i=0; i<8; i++)
            out_rom[i] = driver_kube.rom[i];
        return true;
    }
    return false;
}
//------------------------------------------------------------------------------