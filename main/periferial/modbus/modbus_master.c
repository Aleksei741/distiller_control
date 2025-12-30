//******************************************************************************
//include
//******************************************************************************
#include "modbus_master.h"
#include "modbus_rtu_holding_registers.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
//******************************************************************************
// Constants
//******************************************************************************
#define MODBUS_RTU_BAUD         9600
#define MODBUS_RTU_UART         UART_NUM_1
#define MODBUS_RTU_TX_PIN       GPIO_NUM_17
#define MODBUS_RTU_RX_PIN       GPIO_NUM_16
#define MODBUS_RTU_RTS_PIN      GPIO_NUM_18  // для RS485 DE/RE

#define MODBUS_SLAVE_ADR        0x01

#define MODBUS_ANGLE_REG        0x01
//******************************************************************************
// Type
//******************************************************************************
typedef struct 
{
    uint16_t angle;
} modbus_write_params_t;
//******************************************************************************
// Variables
//******************************************************************************
//------------------------------------------------------------------------------
// Global
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local Variable
//------------------------------------------------------------------------------
static char *TAG = "[modbus_RTU_master]";

static TaskHandle_t modbus_task_handle = NULL;
static SemaphoreHandle_t modbus_params_mutex = NULL;
static modbus_write_params_t current_params = {0};
//******************************************************************************
// Function prototype
//******************************************************************************
esp_err_t uart_init(void);
static void modbus_rtu_task(void *arg);
//******************************************************************************
// Function
//******************************************************************************
esp_err_t modbus_rtu_master_init(void)
{
    // Инициализация UART
    esp_err_t err = uart_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "UART init failed");
        return err;
    }

    // Создаём mutex
    modbus_params_mutex = xSemaphoreCreateMutex();
    if (modbus_params_mutex == NULL) 
    {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_FAIL;
    }

    // Запускаем задачу
    BaseType_t task_created = xTaskCreate(modbus_rtu_task, "modbus_rtu_task", 2048, NULL, 5, &modbus_task_handle);

    if (task_created != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to create task");
        vSemaphoreDelete(modbus_params_mutex);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Modbus RTU Master initialized");
    return ESP_OK;
}
//------------------------------------------------------------------------------
void modbus_master_set_write_params(uint16_t value, modbus_param_t param)
{
    if (modbus_params_mutex == NULL) return;

    if (xSemaphoreTake(modbus_params_mutex, pdMS_TO_TICKS(10)) == pdTRUE) 
    {
        if(param == MODBUS_HOSE_ANGLE)
        {
            current_params.angle = value;
        }        
        xSemaphoreGive(modbus_params_mutex);
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Задача Modbus Master
static void modbus_rtu_task(void *arg)
{
    uint8_t frame[256];
    uint8_t response[256];
    uint16_t value = 0;
    const uint32_t task_period_ms = 1000; // опрашивать раз в секунду

    while (1) 
    {
        modbus_write_params_t params;

        // Копируем параметры потокобезопасно
        if (xSemaphoreTake(modbus_params_mutex, pdMS_TO_TICKS(10)) == pdTRUE) 
        {
            params = current_params;
            xSemaphoreGive(modbus_params_mutex);
        }
        
        //--- Register angle ---
        size_t frame_len = modbus_rtu_build_write_single_register(MODBUS_SLAVE_ADR, MODBUS_ANGLE_REG, params.angle, frame, sizeof(frame));
        if (frame_len == 0) 
        {
            ESP_LOGE(TAG, "Failed to build write request");
            vTaskDelay(pdMS_TO_TICKS(task_period_ms));
            continue;
        }

        int written = uart_write_bytes(MODBUS_RTU_UART, frame, frame_len);
        if (written != (int)frame_len)
        {
            ESP_LOGE(TAG, "UART write failed");
            vTaskDelay(pdMS_TO_TICKS(task_period_ms));
            continue;
        }
        
        vTaskDelay(pdMS_TO_TICKS(5)); // достаточно для 9600 baud

        int resp_len = uart_read_bytes(MODBUS_RTU_UART, response, sizeof(response), pdMS_TO_TICKS(100));
        if (resp_len <= 0)
        {
            ESP_LOGE(TAG, "No response from slave %d", MODBUS_RTU_UART);
            vTaskDelay(pdMS_TO_TICKS(task_period_ms));
            continue;
        }

        // Парсим ответ
        modbus_err_t err = modbus_rtu_parse_write_single_register(response, resp_len, MODBUS_SLAVE_ADR, MODBUS_ANGLE_REG, value);

        if (err != MODBUS_ERR_OK)
        {
            ESP_LOGE(TAG, "Parse error %d for slave %d, reg %d", 
                        err, MODBUS_SLAVE_ADR, MODBUS_ANGLE_REG);
        }
        else
        {
            ESP_LOGI(TAG, "Write OK: slave=%d, reg=%d, value=0x%04X", 
                        MODBUS_SLAVE_ADR, MODBUS_ANGLE_REG, value);
        }

        vTaskDelay(pdMS_TO_TICKS(task_period_ms));
    }
}
//------------------------------------------------------------------------------
// Инициализация UART для Modbus RTU
esp_err_t uart_init(void) 
{
    uart_config_t cfg = 
    {
        .baud_rate = MODBUS_RTU_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(MODBUS_RTU_UART, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(MODBUS_RTU_UART, MODBUS_RTU_TX_PIN,
                                 MODBUS_RTU_RX_PIN, MODBUS_RTU_RTS_PIN, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_mode(MODBUS_RTU_UART, UART_MODE_RS485_HALF_DUPLEX));
    ESP_ERROR_CHECK(uart_driver_install(MODBUS_RTU_UART, 256, 0, 0, NULL, 0));

    ESP_LOGI(TAG, "Modbus RTU initialized on UART%d", MODBUS_RTU_UART);
    return ESP_OK;
}
//------------------------------------------------------------------------------
