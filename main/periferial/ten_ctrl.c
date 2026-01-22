//******************************************************************************
//include
//******************************************************************************
#include <stdint.h>

#include "ten_ctrl.h"
#include "esp_log.h"          // ESP_LOGI
#include "driver/gpio.h"        // GPIO управление
#include "soc/gpio_reg.h"
#include "soc/gpio_struct.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//******************************************************************************
// Constants
//******************************************************************************
#define TEN_GPIO 2
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

volatile static uint8_t heater_power = 0;
//******************************************************************************
// Function prototype
//******************************************************************************
void heater_task(void *arg);
//******************************************************************************
// Function
//******************************************************************************
void IRAM_ATTR zcd_isr(void *arg)
{
    static volatile uint8_t burst_counter = 0;
    static uint16_t error = 0;

    static int last_cc = 0;
    int now_cc = xthal_get_ccount();

    if (now_cc - last_cc < 400000 ) { // игнорировать, если прошло < 1 мс (меньше полупериода)
        return;
    }
    last_cc = now_cc;
  
    error += heater_power;
    if (error >= 100)
    {
        if (TEN_ACTIVE) {
            GPIO.out_w1ts = (1ULL << TEN_GPIO); // установить HIGH
        } else {
            GPIO.out_w1tc = (1ULL << TEN_GPIO); // установить LOW
        }
        error -= 100;
    } 
    else 
    {
        if (TEN_ACTIVE) {
            GPIO.out_w1tc = (1ULL << TEN_GPIO); // установить LOW
        } else {
            GPIO.out_w1ts = (1ULL << TEN_GPIO); // установить HIGH
        }
    }

    portYIELD_FROM_ISR(pdFALSE);
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
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };
    gpio_config(&zio);

    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(ZCD_GPIO, zcd_isr, NULL);
}
//------------------------------------------------------------------------------
void heater_set_power(uint8_t p)
{
    if (p > 100) p = 100;
    heater_power = p;
}