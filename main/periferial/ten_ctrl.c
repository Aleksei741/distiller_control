//******************************************************************************
//include
//******************************************************************************
#include "ten_ctrl.h"
#include "esp_log.h"          // ESP_LOGI
#include "driver/gpio.h"        // GPIO управление
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//******************************************************************************
// Constants
//******************************************************************************
#define TEN_GPIO 25
#define TEN_ACTIVE 1

#define ZCD_GPIO  26
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
static const char *TAG = "[ten_control]";

static TaskHandle_t hTask = NULL;
static SemaphoreHandle_t g_mutex = NULL;

volatile uint8_t heater_power = 0;
volatile bool fTenON = false;
//******************************************************************************
// Function prototype
//******************************************************************************
void heater_task(void *arg);
//******************************************************************************
// Function
//******************************************************************************
void IRAM_ATTR zcd_isr(void *arg)
{
    BaseType_t mustYield = pdFALSE;
    if (fTenON) 
    {
        gpio_set_level(TEN_GPIO, TEN_ACTIVE);              
    }

    vTaskNotifyGiveFromISR((TaskHandle_t)arg, &mustYield);  
    return (mustYield == pdTRUE);
}
//------------------------------------------------------------------------------
void heater_init()
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << TEN_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
    gpio_set_level(TEN_GPIO, TEN_ACTIVE ? 0 : 1);

    gpio_config_t zio = {
        .pin_bit_mask = 1ULL << ZCD_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&zio);

    g_mutex = xSemaphoreCreateMutex();
    if (g_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }

    xTaskCreate(heater_task, "ten_task", 2048, NULL, 5, &hTask);  

    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(ZCD_GPIO, zcd_isr, hTask);
}
//------------------------------------------------------------------------------
void heater_task(void *arg)
{
    uint8_t power;
    volatile uint16_t burst_counter = 0;    

    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (xSemaphoreTake(g_mutex, portMAX_DELAY)) 
        {
            power = heater_power;
            xSemaphoreGive(g_mutex); 
        }

        burst_counter++;
        if (burst_counter >= 100)
            burst_counter = 0;

        if(burst_counter < power)
            fTenON = true;
        else
            fTenON = false;

        if(gpio_get_level(TEN_GPIO) == TEN_ACTIVE && !fTenON)
        {
            vTaskDelay(pdMS_TO_TICKS(3));
            gpio_set_level(TEN_GPIO, TEN_ACTIVE ? 0 : 1);
        }
    }
}
//------------------------------------------------------------------------------
void heater_set_power(uint8_t p)
{
    if (p < 0) p = 0;
    if (p > 100) p = 100;

    if (xSemaphoreTake(g_mutex, portMAX_DELAY)) 
    {
        heater_power = p;
        xSemaphoreGive(g_mutex); 
    }    
}