//******************************************************************************
//include
//******************************************************************************
#include "radiator_fan.h"
#include "esp_log.h"          // ESP_LOGI
#include "driver/gpio.h"        // GPIO управление
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//******************************************************************************
// Constants
//******************************************************************************
#define FAN_GPIO        23          // GPIO, к которому подключён ШИМ-вход вентилятора
#define FAN_LEDC_TIMER  LEDC_TIMER_0
#define FAN_LEDC_CHANNEL LEDC_CHANNEL_0
#define FAN_PWM_FREQ_HZ 25000       // 25 kHz
#define FAN_PWM_RESOLUTION LEDC_TIMER_8_BIT  // 8 бит → 0..255
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

//------------------------------------------------------------------------------
// Local Class
//------------------------------------------------------------------------------

//******************************************************************************
// Function prototype
//******************************************************************************
void fan_set_speed_percent(uint8_t percent);
//******************************************************************************
// Function
//******************************************************************************
void init_fan(void)
{
    // Настройка канала LEDC
    ledc_channel_config_t ledc_channel = 
    {
        .gpio_num = FAN_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,  // или HIGH_SPEED_MODE, но LOW достаточно
        .channel = FAN_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = FAN_LEDC_TIMER,
        .duty = 0,  // начальная скважность = 0%
        .hpoint = 0
    };

    // Настройка таймера
    ledc_timer_config_t ledc_timer = 
    {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = FAN_LEDC_TIMER,
        .duty_resolution = FAN_PWM_RESOLUTION,
        .freq_hz = FAN_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };

    // Установка таймера
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Установка канала
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    fan_set_speed_percent(0); // выключен
}
//------------------------------------------------------------------------------
uint8_t fan_process(float radiator_temperature)
{
    const uint32_t COOLDOWN_TIME_MS = 90ULL * 1000; // 1.5 минуты = 90 секунд → в милисекундах
    uint32_t time_now = pdTICKS_TO_MS(xTaskGetTickCount());
    uint8_t fan;

    static uint32_t time_temperature_50 = 0;
    static uint32_t time_temperature_45 = 0;
    static uint32_t time_temperature_40 = 0;
    static uint32_t time_temperature_35 = 0;
    static uint32_t time_temperature_30 = 0;

    if (radiator_temperature >= 50.0f)
        time_temperature_50 = time_now + COOLDOWN_TIME_MS;
    if (radiator_temperature >= 45.0f)
        time_temperature_45 = time_now + COOLDOWN_TIME_MS;
    if (radiator_temperature >= 40.0f)
        time_temperature_40 = time_now + COOLDOWN_TIME_MS;
    if (radiator_temperature >= 35.0f)
        time_temperature_35 = time_now + COOLDOWN_TIME_MS;
    if (radiator_temperature >= 30.0f)
        time_temperature_30 = time_now + COOLDOWN_TIME_MS;
    
    if ((int32_t)(time_now - time_temperature_50) < 0) 
        fan = 100;
    else if ((int32_t)(time_now - time_temperature_45) < 0) 
        fan = 90;
    else if ((int32_t)(time_now - time_temperature_40) < 0) 
        fan = 80;
    else if ((int32_t)(time_now - time_temperature_35) < 0) 
        fan = 70;
    else if ((int32_t)(time_now - time_temperature_30) < 0) 
        fan = 60;
    else
        fan = 0;

    fan_set_speed_percent(fan);
    return fan;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Внутренние функции
//------------------------------------------------------------------------------
void fan_set_speed_percent(uint8_t percent)
{
    static uint8_t p = 0;
    if(p == percent)
        return;
    p = percent;

    if (percent > 100) percent = 100;

    uint32_t max_duty = (1 << FAN_PWM_RESOLUTION) - 1;
    uint32_t duty = (percent * max_duty) / 100;

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, FAN_LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, FAN_LEDC_CHANNEL));
}