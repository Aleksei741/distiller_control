//******************************************************************************
//include
//******************************************************************************
#include "temperature_sensor.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
//******************************************************************************
// Constants
//******************************************************************************
#define DS18B20_COLUMN_PIN 0
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
static ds18b20_t tempereture_column = { .pin = DS18B20_COLUMN_PIN };

static SemaphoreHandle_t g_temp_mutex = NULL;
static float g_temperature = 0.0f; // Текущая температура
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
    init_ds18b20_gpio(&tempereture_column);
    
    // Создаем мьютекс
    g_temp_mutex = xSemaphoreCreateMutex();
    if (g_temp_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }

    while(1)
    {
        if(ds18b20_read_rom(&tempereture_column))
        {
            ESP_LOGI(TAG, "DS18B20 ROM code: %02X %02X %02X %02X %02X %02X %02X %02X",
                     tempereture_column.rom[0], tempereture_column.rom[1],
                     tempereture_column.rom[2], tempereture_column.rom[3],
                     tempereture_column.rom[4], tempereture_column.rom[5],
                     tempereture_column.rom[6], tempereture_column.rom[7]);
            break;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read DS18B20 ROM code");
            vTaskDelay(pdMS_TO_TICKS(1000)); // ждем и пробуем снова
        }
    }
    
    // Запускаем задачу
    xTaskCreate(temperature_task, "temperature_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Temperature sensor initialized");
}
//------------------------------------------------------------------------------
void temperature_task(void *arg)
{
    vTaskDelay(pdMS_TO_TICKS(100)); // Даем время на инициализацию
    
    while (1)
    {        
        // Читаем температуру
        float t;
         
        if(ds18b20_read_temp_rom(&tempereture_column, &t) == false) 
        {
            ESP_LOGW(TAG, "Failed to read temperature");
            vTaskDelay(pdMS_TO_TICKS(1000)); // ждем и пробуем снова
            continue;
        }
        
        // Сохраняем температуру
        if (xSemaphoreTake(g_temp_mutex, portMAX_DELAY)) {
            g_temperature = t;
            xSemaphoreGive(g_temp_mutex);
        }

        ESP_LOGI(TAG, "Temperature: %.2f °C", t);
        vTaskDelay(pdMS_TO_TICKS(1000)); // раз в секунду
    }
}
//------------------------------------------------------------------------------