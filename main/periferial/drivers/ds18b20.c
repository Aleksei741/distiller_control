//******************************************************************************
//include
//******************************************************************************
#include "ds18b20.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/dport_reg.h"
#include "esp_timer.h"
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
static const char *TAG = "[ds18b20]";
//------------------------------------------------------------------------------
// Local Class
//------------------------------------------------------------------------------

//******************************************************************************
// Function prototype
//******************************************************************************
uint8_t ds18b20_crc8(const uint8_t *data, uint8_t len);
int ds18b20_reset(const ds18b20_t *ds18b20);
void ds18b20_write_bit(const ds18b20_t *ds18b20, int bit);
int ds18b20_read_bit(const ds18b20_t *ds18b20);
void ds18b20_write_byte(const ds18b20_t *ds18b20, uint8_t byte);
uint8_t ds18b20_read_byte(const ds18b20_t *ds18b20);

static inline void ds18b20_delay_us(uint32_t us);
//******************************************************************************
// Function
//******************************************************************************
void init_ds18b20_gpio(ds18b20_t *ds18b20)
{
    // Настройка пина в режиме Open Drain с подтяжкой
    gpio_config_t gpio_conf = {
        .pin_bit_mask = (1ULL << ds18b20->pin),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,  // Open Drain с возможностью чтения
        .pull_up_en = GPIO_PULLUP_DISABLE,   // Включить внутреннюю подтяжку
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&gpio_conf);
    gpio_set_level(ds18b20->pin, 1);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Высокоуровневые функции DS18B20
//------------------------------------------------------------------------------
// Поиск ROM-кода первого найденного датчика
bool ds18b20_read_rom(ds18b20_t *ds18b20) 
{
    uint8_t data[9];

    ESP_LOGI(TAG, "ds18b20_reset(ds18b20) pin[%d]", ds18b20->pin);
    if(!ds18b20_reset(ds18b20))
    {
        ESP_LOGW(TAG, "ds18b20_read_rom pin[%d] No presence pulse detected", ds18b20->pin);
        return false;
    }

    ESP_LOGI(TAG, "ds18b20_write_byte(ds18b20, 0x33) pin[%d]", ds18b20->pin);
    ds18b20_write_byte(ds18b20, 0x33); // Read ROM command

    for (int i = 0; i < 8; i++)
        data[i] = ds18b20_read_byte(ds18b20);

    ESP_LOGI(TAG, "ds18b20_read_byte(ds18b20): %02X %02X %02X %02X %02X %02X %02X %02X",
                    data[0], data[1], data[2], data[3],
                    data[4], data[5], data[6], data[7]);

    if(data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x00 &&
       data[4] == 0x00 && data[5] == 0x00 && data[6] == 0x00 && data[7] == 0x00)
    {
        ESP_LOGW(TAG, "ds18b20_read_rom pin[%d] not pull up the data line", ds18b20->pin);
        return false;
    }
     
    uint8_t crc = ds18b20_crc8(data, 7);
    if (crc != data[7]) 
    {
        ESP_LOGW(TAG, "ds18b20_read_rom CRC error pin[%d]: calc=%02X recv=%02X",
                 ds18b20->pin, crc, data[7]);
        return false;
    }

    for (int i = 0; i < 8; i++)
        ds18b20->rom[i] = data[i];

    return true;
}
//------------------------------------------------------------------------------
// Выбор конкретного датчика по ROM-коду
void ds18b20_match_rom(const ds18b20_t *ds18b20) 
{
    ds18b20_write_byte(ds18b20, 0x55); // Match ROM command
    for(int i = 0; i < 8; i++)
        ds18b20_write_byte(ds18b20, ds18b20->rom[i]);
}
//------------------------------------------------------------------------------
// Чтение температуры конкретного датчика
bool ds18b20_read_temp_rom(const ds18b20_t *ds18b20, float *temperature) 
{
    uint8_t data[9] = {0};
    
    // Запускаем преобразование
    if (!ds18b20_reset(ds18b20)) return false;
    ds18b20_match_rom(ds18b20);
    ds18b20_write_byte(ds18b20, 0x44);
    vTaskDelay(pdMS_TO_TICKS(750));
    
    // Читаем скрэтчпад
    if (!ds18b20_reset(ds18b20)) return false;
    ds18b20_match_rom(ds18b20);
    ds18b20_write_byte(ds18b20, 0xBE);
    
    for (int i = 0; i < 9; i++)
        data[i] = ds18b20_read_byte(ds18b20);

    if(data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x00 &&
       data[4] == 0x00 && data[5] == 0x00 && data[6] == 0x00 && data[7] == 0x00)
    {
        ESP_LOGW(TAG, "ds18b20_read_temp_rom pin[%d] not pull up the data line", ds18b20->pin);
        return false;
    }
    
    // Проверка CRC
    if (ds18b20_crc8(data, 8) != data[8]) 
    {
        ESP_LOGW(TAG, "ds18b20_read_temp_rom pin[%d] CRC error", ds18b20->pin);
        return false;
    }

    int16_t temp_raw = (data[1] << 8) | data[0];    
    *temperature = (float)temp_raw / 16.0f;       
    return true;
}
//------------------------------------------------------------------------------
// Чтение температуры при одиночном подключении датчика
bool ds18b20_read_temp_single(const ds18b20_t *ds18b20, float *temperature)
{
    uint8_t data[9];

    // Запуск конверсии
    if (!ds18b20_reset(ds18b20)) 
    {
        ESP_LOGW(TAG, "ds18b20_read_temp_single pin[%d] No presence pulse 1", ds18b20->pin);
        return false;
    }

    ds18b20_write_byte(ds18b20, 0xCC); // SKIP ROM
    ds18b20_write_byte(ds18b20, 0x44); // CONVERT T

    // Время преобразования (750 мс для 12-bit)
    vTaskDelay(pdMS_TO_TICKS(750));

    // Чтение скрэтчпада
    if (!ds18b20_reset(ds18b20)) 
    {
        ESP_LOGW(TAG, "ds18b20_read_temp_single pin[%d] No presence pulse 2", ds18b20->pin);
        return false;
    }

    ds18b20_write_byte(ds18b20, 0xCC); // SKIP ROM
    ds18b20_write_byte(ds18b20, 0xBE); // READ SCRATCHPAD

    for (int i = 0; i < 9; i++)
        data[i] = ds18b20_read_byte(ds18b20);

    if(data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x00 &&
       data[4] == 0x00 && data[5] == 0x00 && data[6] == 0x00 && data[7] == 0x00)
    {
        ESP_LOGW(TAG, "ds18b20_read_temp_single pin[%d] not pull up the data line", ds18b20->pin);
        return false;
    }

    // CRC
    if (ds18b20_crc8(data, 8) != data[8]) 
    {
        ESP_LOGW(TAG, "ds18b20_read_temp_single pin[%d] CRC error", ds18b20->pin);
        return false;
    }

    int16_t raw = (data[1] << 8) | data[0];
    *temperature = raw / 16.0f;
    return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// crc8 вычисление
//------------------------------------------------------------------------------
uint8_t ds18b20_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    while (len--) {
        uint8_t inbyte = *data++;
        for (uint8_t i = 0; i < 8; i++) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;  // Dallas polynomial
            inbyte >>= 1;
        }
    }
    return crc;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Низкоуровневые функции 1-Wire
//------------------------------------------------------------------------------
// Функция сброса с детектированием Presence Pulse
int ds18b20_reset(const ds18b20_t *ds18b20)
{
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);

    gpio_set_level(ds18b20->pin, 0);
    //esp_rom_delay_us(480);
    ds18b20_delay_us(480);
    gpio_set_level(ds18b20->pin, 1);
    //esp_rom_delay_us(70);   // Ждем 15-60 µs для Presence Pulse
    ds18b20_delay_us(70);
    
    // Читаем линию - датчик должен опустить ее в низкий уровень
    int presence = !gpio_get_level(ds18b20->pin);
    
    // Ждем окончания временного слота
    //esp_rom_delay_us(410);
    ds18b20_delay_us(410);
    
    portEXIT_CRITICAL(&mux);
    return presence;
}
//------------------------------------------------------------------------------
void ds18b20_write_bit(const ds18b20_t *ds18b20, int bit)
{
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);

    // Записываем "1" или "0"
    if (bit) {
        // Запись "1": короткий низкий уровень
        gpio_set_level(ds18b20->pin, 0);
        //esp_rom_delay_us(6);     // Держим 1-15 µs
        ds18b20_delay_us(6);
        gpio_set_level(ds18b20->pin, 1);
        //esp_rom_delay_us(64);    // Ждем до конца временного слота
        ds18b20_delay_us(64);
    } else {
        // Запись "0": длинный низкий уровень
        gpio_set_level(ds18b20->pin, 0);
        //esp_rom_delay_us(60);    // Держим 60 µs
        ds18b20_delay_us(60);
        gpio_set_level(ds18b20->pin, 1);
        //esp_rom_delay_us(10);    // Восстановление
        ds18b20_delay_us(10);
    }
    portEXIT_CRITICAL(&mux);
}
//------------------------------------------------------------------------------
int ds18b20_read_bit(const ds18b20_t *ds18b20)
{
    int bit;

    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);
    
    // Генерируем слот чтения
    gpio_set_level(ds18b20->pin, 0);
    //esp_rom_delay_us(3);     // Держим 1-15 µs
    ds18b20_delay_us(3);
    
    // Отпускаем линию
    gpio_set_level(ds18b20->pin, 1);
    //esp_rom_delay_us(10);    // Ждем 15 µs
    ds18b20_delay_us(10);
    
    // Читаем уровень линии
    bit = gpio_get_level(ds18b20->pin);
    
    // Ждем до конца временного слота
    //esp_rom_delay_us(53);    // Всего должно быть 60 µs
    ds18b20_delay_us(53);
    portEXIT_CRITICAL(&mux);
    return bit;
}
//------------------------------------------------------------------------------
void ds18b20_write_byte(const ds18b20_t *ds18b20, uint8_t byte) 
{
    for (int i = 0; i < 8; i++) 
    {
        ds18b20_write_bit(ds18b20, byte & 0x01);
        byte >>= 1;
    }
}
//------------------------------------------------------------------------------
uint8_t ds18b20_read_byte(const ds18b20_t *ds18b20) 
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) 
    {
        byte >>= 1;
        if (ds18b20_read_bit(ds18b20)) 
        {
            byte |= 0x80;
        }
    }
    return byte;
}
//------------------------------------------------------------------------------
// Точная задержка в микросекундах (без отключения прерываний)
inline void ds18b20_delay_us(uint32_t us)
{
    uint64_t start = esp_timer_get_time();
    while ((esp_timer_get_time() - start) < us) {
        // активная задержка
    }
}
//------------------------------------------------------------------------------
