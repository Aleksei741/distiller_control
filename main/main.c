#include <string.h>

#include <parameters.h>
#include <wifi_control.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "esp_http_server.h"

#define LED_GPIO 2  // Пин для светодиода

static const char *TAG = "main";

// Веб-страница
const char html_page[] = "<html>\
<head><title>ESP32 Control</title></head>\
<body>\
<h2>ESP32 Web Server</h2>\
<button onclick=\"fetch('/led/on')\">Turn ON</button>\
<button onclick=\"fetch('/led/off')\">Turn OFF</button>\
</body></html>";

// Функция обработки запроса к главной странице
esp_err_t index_handler(httpd_req_t *req) 
{
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Функция обработки запроса управления светодиодом
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

// Запуск HTTP-сервера
httpd_handle_t start_webserver(void) 
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) 
    {
        ESP_LOGW(TAG, "Регистрируем index_uri led_uri");
        httpd_uri_t index_uri = { .uri = "/", .method = HTTP_GET, .handler = index_handler };
        httpd_uri_t led_on_uri = { .uri = "/led/on", .method = HTTP_GET, .handler = led_handler };
        httpd_uri_t led_off_uri = { .uri = "/led/off", .method = HTTP_GET, .handler = led_handler };
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &led_on_uri);
        httpd_register_uri_handler(server, &led_off_uri);
    }
    return server;
}

void app_main(void) 
{
    parameters_init();
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    
    wifi_settings_t sta_cfg;
    wifi_settings_t ap_cfg;
    load_wifi_sta_settings(&sta_cfg);
    load_wifi_ap_settings(&ap_cfg);

    wifi_start_ap_sta(&sta_cfg, &ap_cfg);

    start_webserver();
}
