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
#include "led.h"
#include "wifi_control.h"
#include "parameters.h"
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
#define LED_GPIO 2  // Пин для светодиода

// Веб-страница
const char html_page[] = "<html>\
<head><title>ESP32 Control</title></head>\
<body>\
<h2>ESP32 Web Server</h2>\
<button onclick=\"fetch('/led/on')\">Turn ON</button>\
<button onclick=\"fetch('/led/off')\">Turn OFF</button>\
</body></html>";

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
esp_err_t led_handler(httpd_req_t *req);
esp_err_t chartjs_handler(httpd_req_t *req);

esp_err_t get_led_status_handler(httpd_req_t *req);
esp_err_t set_led_handler(httpd_req_t *req);

esp_err_t wifi_set_sta_handler(httpd_req_t *req);
esp_err_t wifi_scan_sta_handler(httpd_req_t *req);
//******************************************************************************
// Function
//******************************************************************************
void init_webserver(void)
{
    ESP_LOGI(TAG, "Webserver initialized");
    init_spiffs();

    //gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
}
//------------------------------------------------------------------------------
httpd_handle_t start_webserver(void) 
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) 
    {
        ESP_LOGW(TAG, "Registering URI handlers");

        // Регистрация URI обработчиков HTML страниц и ресурсов
        httpd_uri_t index_uri = { .uri = "/", .method = HTTP_GET, .handler = index_handler };
        httpd_uri_t css_uri = { .uri = "/style.css", .method = HTTP_GET, .handler = css_handler };
        httpd_uri_t js_uri = { .uri = "/app.js", .method = HTTP_GET, .handler = js_handler };
        httpd_uri_t chartjs_uri = { .uri = "/chart.umd.min.js", .method = HTTP_GET, .handler = chartjs_handler };
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &css_uri);
        httpd_register_uri_handler(server, &js_uri);
        httpd_register_uri_handler(server, &chartjs_uri);

        // Регистрация URI обработчиков для управления светодиодом
        httpd_uri_t led_status_uri = { .uri = "/api/led/status", .method = HTTP_GET, .handler = get_led_status_handler };
        httpd_uri_t led_toggle_uri = { .uri = "/api/led/toggle", .method = HTTP_GET, .handler = set_led_handler };        
        httpd_register_uri_handler(server, &led_status_uri);
        httpd_register_uri_handler(server, &led_toggle_uri);

        // Регистрация URI обработчиков для управления WiFi
        httpd_uri_t wifi_set_sta_uri = { .uri = "/api/wifi/sta", .method = HTTP_GET, .handler = wifi_set_sta_handler };
        httpd_uri_t uri_wifi_scan_uri = {.uri = "/api/wifi/sta/scan", .method = HTTP_GET, .handler = wifi_scan_sta_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &wifi_set_sta_uri);
        httpd_register_uri_handler(server, &uri_wifi_scan_uri);
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
// LED Handlers
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------    
esp_err_t get_led_status_handler(httpd_req_t *req) 
{
    char buffer[64];
    int is_led_on = led_status();

    int len = snprintf(buffer, sizeof(buffer), 
                      "{\"led\": %s}", 
                      is_led_on ? "true" : "false");
    
    if (len < 0 || len >= sizeof(buffer)) 
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buffer, len);

    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t set_led_handler(httpd_req_t *req) 
{
    char buffer[64];
    
    // Переключаем LED
    int state = led_status();
    int new_state = !state;
    led_set(new_state);
    
    int len = snprintf(buffer, sizeof(buffer), 
                      "{\"led\": %s}", 
                      new_state ? "true" : "false");
    
    if (len < 0 || len >= sizeof(buffer))
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buffer, len);
    
    return ESP_OK;
}
//------------------------------------------------------------------------------
// wifi handlers
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
esp_err_t wifi_set_sta_handler(httpd_req_t *req)
{
    char query[128] = {0};
    char ssid[64] = {0};
    char pass[64] = {0};

    // Получаем длину query-параметров
    size_t query_len = httpd_req_get_url_query_len(req);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\": \"ok\"}");

    if (query_len == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No query params");
        return ESP_FAIL;
    }

    if (query_len + 1 > sizeof(query)) {
        httpd_resp_send_err(req, HTTPD_414_URI_TOO_LONG, "Query too long");
        return ESP_FAIL;
    }

    // Читаем строку вида "ssid=22222222&pass=1111111"
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad query");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Query: %s", query);

    // Извлекаем ssid
    if (httpd_query_key_value(query, "ssid", ssid, sizeof(ssid)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing ssid");
        return ESP_FAIL;
    }

    // Извлекаем pass
    if (httpd_query_key_value(query, "pass", pass, sizeof(pass)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing pass");
        return ESP_FAIL;
    }

    // Логируем
    ESP_LOGI(TAG, "SSID: %s", ssid);
    ESP_LOGI(TAG, "PASS: %s", pass);

    wifi_settings_t wifi_settings;
    strncpy(wifi_settings.ssid, ssid, sizeof(wifi_settings.ssid) - 1);
    strncpy(wifi_settings.pass, pass, sizeof(wifi_settings.pass) - 1);      

    // Сохраняем настройки WiFi
    save_wifi_sta_settings(&wifi_settings);
    wifi_reinit_sta(&wifi_settings);

    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t wifi_scan_sta_handler(httpd_req_t *req)
{
    scanned_ap_info_t aps[MAX_SCAN_APS];
    int count = wifi_scan_ap(aps, MAX_SCAN_APS);

    // Открываем JSON вручную
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "[");

    for (int i = 0; i < count; i++) {
        char buf[128];
        snprintf(buf, sizeof(buf),
            "{\"ssid\":\"%s\",\"rssi\":%d,\"auth\":%d}%s",
            aps[i].ssid,
            aps[i].rssi,
            aps[i].authmode,
            (i + 1 < count) ? "," : ""
        );
        httpd_resp_sendstr_chunk(req, buf);
    }

    httpd_resp_sendstr_chunk(req, "]");
    httpd_resp_sendstr_chunk(req, NULL); // завершение

    return ESP_OK;
}



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