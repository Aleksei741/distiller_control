//******************************************************************************
//include
//******************************************************************************
#include <stdio.h>              // fopen, fread, fclose
#include <string.h>             // strstr, strcmp, strlen

#include "esp_log.h"            // ESP_LOGI, ESP_LOGW, ESP_LOGE
#include "esp_err.h"            // esp_err_t
#include "esp_spiffs.h"         // SPIFFS
#include "esp_spiffs.h"         // Альтернативные SPIFFS функции
#include "esp_http_server.h"    // HTTP сервер

#include "webServer.h"
#include "distiler_control_handler.h"
#include "LED_handler.h"
#include "statistic_handler.h"
#include "tempsensor_handler.h"
#include "wifi_handler.h"
//******************************************************************************
// Cinstants
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
static const char *TAG = "[webServer]";

//------------------------------------------------------------------------------
// Local Class
//------------------------------------------------------------------------------

//******************************************************************************
// Function prototype
//******************************************************************************
void init_spiffs(void);
esp_err_t send_file(httpd_req_t *req, const char *path, const char *content_type);
esp_err_t index_handler(httpd_req_t *req);
esp_err_t css_handler(httpd_req_t *req);
esp_err_t js_handler(httpd_req_t *req);
esp_err_t chartjs_handler(httpd_req_t *req);
esp_err_t ico_handler(httpd_req_t *req);
static esp_err_t not_found_uri_handle(httpd_req_t *req);
//******************************************************************************
// Function
//******************************************************************************
void init_webserver(void)
{
    ESP_LOGI(TAG, "Webserver initialized");
    init_spiffs();
}
//------------------------------------------------------------------------------
httpd_handle_t start_webserver(void) 
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 36; // Увеличение количества URI обработчиков
    config.lru_purge_enable  = true; // Включение очистки наименее используемых соединений

    if (httpd_start(&server, &config) == ESP_OK) 
    {
        ESP_LOGW(TAG, "Registering URI handlers");

        // Регистрация URI обработчиков HTML страниц и ресурсов
        httpd_uri_t index_uri = { .uri = "/", .method = HTTP_GET, .handler = index_handler };
        httpd_uri_t css_uri = { .uri = "/style.css", .method = HTTP_GET, .handler = css_handler };
        httpd_uri_t js_uri = { .uri = "/app.js", .method = HTTP_GET, .handler = js_handler };
        httpd_uri_t chartjs_uri = { .uri = "/chart.umd.min.js", .method = HTTP_GET, .handler = chartjs_handler };
        httpd_uri_t ico_uri = { .uri = "/favicon.ico", .method = HTTP_GET, .handler = ico_handler };
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &css_uri);
        httpd_register_uri_handler(server, &js_uri);
        httpd_register_uri_handler(server, &chartjs_uri);
        httpd_register_uri_handler(server, &ico_uri);

        // Регистрация URI обработчиков для управления WiFi
        httpd_uri_t wifi_ap_uri = { .uri="/api/wifi/ap", .method=HTTP_GET, .handler=wifi_set_ap_handler };
        httpd_uri_t wifi_sta_uri = { .uri="/api/wifi/sta", .method=HTTP_GET, .handler=wifi_set_sta_handler };
        httpd_uri_t wifi_scan_start_uri  = { .uri="/api/wifi/sta/scan/start",  .method=HTTP_GET, .handler=wifi_sta_start_scan_handler };
        httpd_uri_t wifi_scan_isdone_uri = { .uri="/api/wifi/sta/scan/isdone", .method=HTTP_GET, .handler=wifi_sta_scan_isdone_handler };
        httpd_uri_t wifi_scan_result_uri = { .uri="/api/wifi/sta/scan/result", .method=HTTP_GET, .handler=wifi_sta_scan_result_handler };
        httpd_register_uri_handler(server, &wifi_ap_uri);
        httpd_register_uri_handler(server, &wifi_sta_uri);
        httpd_register_uri_handler(server, &wifi_scan_start_uri);
        httpd_register_uri_handler(server, &wifi_scan_isdone_uri);
        httpd_register_uri_handler(server, &wifi_scan_result_uri);

        // Регистрация URI обработчиков для работы с датчиком температуры
        httpd_uri_t tempsensor_get_rom_uri = { .uri="/api/tempsensor/rom", .method=HTTP_GET, .handler=tempsensor_rom_handler };
        httpd_uri_t tempsensor_rom_cube_uri = { .uri="/api/tempsensor/rom/cube", .method=HTTP_GET, .handler=tempsensor_set_rom_cube_handler };
        httpd_uri_t tempsensor_rom_column_uri = { .uri="/api/tempsensor/rom/column", .method=HTTP_GET, .handler=tempsensor_set_rom_column_handler };
        httpd_register_uri_handler(server, &tempsensor_get_rom_uri);
        httpd_register_uri_handler(server, &tempsensor_rom_cube_uri);
        httpd_register_uri_handler(server, &tempsensor_rom_column_uri);

        // Регистрация URI обработчиков для работы со статистикой
        httpd_uri_t lock_statistic_buffer_uri = { .uri="/api/statistic/lock", .method=HTTP_GET, .handler=lock_statistic_buffer_handler };
        httpd_uri_t unlock_statistic_buffer_uri = { .uri="/api/statistic/unlock", .method=HTTP_GET, .handler=unlock_statistic_buffer_handler };
        httpd_uri_t get_status_statistic_buffer_uri = { .uri="/api/statistic/status", .method=HTTP_GET, .handler=get_status_statistic_buffer_handler };
        httpd_uri_t get_statistic_uri = { .uri="/api/statistic/", .method=HTTP_GET, .handler=get_statistic_handler };
        httpd_register_uri_handler(server, &lock_statistic_buffer_uri);
        httpd_register_uri_handler(server, &unlock_statistic_buffer_uri);
        httpd_register_uri_handler(server, &get_status_statistic_buffer_uri);
        httpd_register_uri_handler(server, &get_statistic_uri);

        // Регистрация URI обработчика для получения статуса управления дистиллятором
        httpd_uri_t distiller_status_uri = { .uri="/api/distiller/status", .method=HTTP_GET, .handler=get_status_distiler_control_handler };
        httpd_uri_t distiller_ten_uri = { .uri="/api/distiller/ten", .method=HTTP_GET, .handler=set_ten_power_handler };
        httpd_register_uri_handler(server, &distiller_status_uri);
        httpd_register_uri_handler(server, &distiller_ten_uri);

        httpd_register_uri_handler(server, &(httpd_uri_t){.uri = "*", .method = HTTP_GET, .handler = not_found_uri_handle});
        httpd_register_uri_handler(server, &(httpd_uri_t){.uri = "*", .method = HTTP_HEAD, .handler = not_found_uri_handle});
    }

    return server;
}
//------------------------------------------------------------------------------
// Local Function
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void init_spiffs(void)
{
    ESP_LOGI(TAG, "Mounting SPIFFS...");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
        return;
    }

    size_t total = 0, used = 0;
    esp_spiffs_info(NULL, &total, &used);
    ESP_LOGI(TAG, "SPIFFS total: %d, used: %d", total, used);
}
//------------------------------------------------------------------------------
esp_err_t send_file(httpd_req_t *req, const char *path, const char *content_type)
{
    FILE *file = fopen(path, "r");
    if (!file) {
        ESP_LOGE("HTTP", "Не открыл файл: %s", path);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, content_type);

    char buffer[1024];
    size_t read_bytes;

    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        esp_err_t ret = httpd_resp_send_chunk(req, buffer, read_bytes);
        if (ret != ESP_OK) {
            fclose(file);
            httpd_resp_sendstr_chunk(req, NULL);
            return ESP_FAIL;
        }
    }

    fclose(file);
    httpd_resp_sendstr_chunk(req, NULL); // завершение передачи
    return ESP_OK;
}

//------------------------------------------------------------------------------
// URI Handlers
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
esp_err_t index_handler(httpd_req_t *req)
{
    return send_file(req, "/spiffs/index.html", "text/html");
}
//------------------------------------------------------------------------------
esp_err_t css_handler(httpd_req_t *req)
{
    return send_file(req, "/spiffs/style.css", "text/css");
}
//------------------------------------------------------------------------------
esp_err_t js_handler(httpd_req_t *req)
{
    return send_file(req, "/spiffs/app.js", "application/javascript");
}
//------------------------------------------------------------------------------
esp_err_t chartjs_handler(httpd_req_t *req)
{
    return send_file(req, "/spiffs/chart.umd.min.js", "application/javascript");
}
//------------------------------------------------------------------------------
esp_err_t ico_handler(httpd_req_t *req)
{
    return send_file(req, "/spiffs/favicon.ico", "image/x-icon");
}
//------------------------------------------------------------------------------
static esp_err_t not_found_uri_handle(httpd_req_t *req)
{
    ESP_LOGW(TAG, "not_found_uri_handle");
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_set_hdr(req, "Connection", "close");
    return httpd_resp_send(req, NULL, 0);
}
//------------------------------------------------------------------------------

// Для JSON
// httpd_resp_set_type(req, "application/json");

// Для HTML
// httpd_resp_set_type(req, "text/html");

// Для простого текста
// httpd_resp_set_type(req, "text/plain");

// Для JavaScript
// httpd_resp_set_type(req, "application/javascript");

// Для CSS
// httpd_resp_set_type(req, "text/css");