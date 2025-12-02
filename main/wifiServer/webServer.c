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
#include "driver/gpio.h"        // GPIO управление

#include "webServer.h"          // Твой заголовок
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
//******************************************************************************
// Function
//******************************************************************************
void init_webserver(void)
{
    ESP_LOGI(TAG, "Webserver initialized");
    init_spiffs();

    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
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
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &css_uri);
        httpd_register_uri_handler(server, &js_uri);

        // Регистрация URI обработчиков для управления светодиодом
        httpd_uri_t led_on_uri = { .uri = "/led/on", .method = HTTP_GET, .handler = led_handler };
        httpd_uri_t led_off_uri = { .uri = "/led/off", .method = HTTP_GET, .handler = led_handler };        
        httpd_register_uri_handler(server, &led_on_uri);
        httpd_register_uri_handler(server, &led_off_uri);
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
esp_err_t led_handler(httpd_req_t *req) 
{
    ESP_LOGW(TAG, "led_handler");
    if (strstr(req->uri, "/led/on")) {

        gpio_set_level(LED_GPIO, 1);
    } else if (strstr(req->uri, "/led/off")) {
        gpio_set_level(LED_GPIO, 0);
    }
    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
//------------------------------------------------------------------------------    
