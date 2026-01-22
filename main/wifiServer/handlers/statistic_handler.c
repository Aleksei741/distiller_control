//******************************************************************************
//include
//******************************************************************************
#include "statistic_handler.h"

#include <stdio.h>              // fopen, fread, fclose
#include <string.h>             // strstr, strcmp, strlen

#include "esp_log.h"            // ESP_LOGI, ESP_LOGW, ESP_LOGE
#include "esp_err.h"            // esp_err_t

#include "statistic.h"
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
static const char *TAG = "[statistic_handlers]";

//******************************************************************************
// Function prototype
//******************************************************************************

//******************************************************************************
// Function
//******************************************************************************
esp_err_t lock_statistic_buffer_handler(httpd_req_t *req)
{
    psram_statistic_lock_write();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"lock_statistic\": \"true\"}");

    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t unlock_statistic_buffer_handler(httpd_req_t *req)
{
    psram_statistic_unlock_write();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"lock_statistic\": \"false\"}");

    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t get_status_statistic_buffer_handler(httpd_req_t *req)
{
    char buf[128];
    size_t firtst_index = psram_statistic_get_first_index();
    size_t latest_index = psram_statistic_get_latest_index();
    size_t size_data = psram_statistic_get_count();

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "{");

    // Column Temperature
    snprintf(buf, sizeof(buf), "\"firtst_index\": %zu", firtst_index);
    httpd_resp_sendstr_chunk(req, buf);

    httpd_resp_sendstr_chunk(req, ", ");

    // Kube Temperature
    snprintf(buf, sizeof(buf), "\"latest_index\": %zu", latest_index);
    httpd_resp_sendstr_chunk(req, buf);

    httpd_resp_sendstr_chunk(req, ", ");

    // Radiator Temperature
    snprintf(buf, sizeof(buf), "\"size_available_data\": %zu", size_data);
    httpd_resp_sendstr_chunk(req, buf);

    httpd_resp_sendstr_chunk(req, "}");
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t get_statistic_handler(httpd_req_t *req)
{
    char query[128] = {0};
    char str_index[32] = {0};
    size_t first_index = 0;
    size_t latest_index = 0;
    statistic_t statistic;
    esp_err_t result = ESP_OK;

    // Прочитать query
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad query");
        result = ESP_FAIL;
        goto finish;
    }

    ESP_LOGI(TAG, "Statistic query: %s", query);

    // Извлечь first_index
    if (httpd_query_key_value(query, "first_index", str_index, sizeof(str_index)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing firts_index");
        result = ESP_FAIL;
        goto finish;
    }
    first_index = strtoul(str_index, NULL, 10);

    // Извлечь latest_index
    if (httpd_query_key_value(query, "latest_index", str_index, sizeof(str_index)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing latest_index");
        result = ESP_FAIL;
        goto finish;
    }
    latest_index = strtoul(str_index, NULL, 10);

    // Валидируем диапазон: если first_index >= latest_index — нечего отправлять
    if (first_index >= latest_index) 
    {
        httpd_resp_set_type(req, "application/octet-stream");
        httpd_resp_send_chunk(req, NULL, 0); // Правильное завершение пустого chunked-ответа
        goto finish;
    }

    // Начинаем ответ chunked
    if (httpd_resp_set_type(req, "application/octet-stream") != ESP_OK) { result = ESP_FAIL; goto finish; }

    // Итерация по индексам и отправка. При любой ошибке отправки — прерываем цикл.
    for (size_t idx = first_index; idx < latest_index; ++idx) 
    {
        bool ok = psram_statistic_read(idx, &statistic);
        if (!ok) continue;   // элемент отсутствует (слишком стар или недоступен) — просто пропускаем

        uint8_t packet[13];

        // time
        memcpy(&packet[0], &statistic.time, 4);

        // heater (uint16_t)
        memcpy(&packet[4], &statistic.heater, 1);

        // floats — копируем как есть (ESP32 little-endian, как и ПК)
        memcpy(&packet[5], &statistic.temperature_column, 4);
        memcpy(&packet[9], &statistic.temperature_kube, 4);

        // Отправляем 13 байт
        if (httpd_resp_send_chunk(req, (const char*)packet, 13) != ESP_OK) 
        {
            ESP_LOGW(TAG, "Client disconnected during binary send");
            result = ESP_FAIL;
            goto finish;
        }
    }

    if (httpd_resp_send_chunk(req, NULL, 0) != ESP_OK) { result = ESP_FAIL; goto finish; }

finish:
    // ГАРАНТИРОВАННО вызываем unlock один раз (если это требование протокола)
    psram_statistic_unlock_write();

    return result;
}
//------------------------------------------------------------------------------
