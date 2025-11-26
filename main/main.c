#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "esp_http_server.h"

#define WIFI_SSID "ESP32_Server"
#define WIFI_PASS "12345678"
#define LED_GPIO 2  // Пин для светодиода

static const char *TAG = "wifi_server";

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

// Настройка Wi-Fi в режиме точки доступа
void wifi_init_softap(void) 
{
    static bool wifi_ap_initialized = false;
    if (wifi_ap_initialized) 
    {
        ESP_LOGW(TAG, "Wi-Fi AP уже инициализирован, пропускаем.");
        return;
    }
    wifi_ap_initialized = true;

    ESP_LOGI(TAG, "Инициализация Wi-Fi...");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_AP);
    wifi_config_t wifi_config = 
    {
        .ap = { .ssid = WIFI_SSID, .password = WIFI_PASS, .ssid_len = strlen(WIFI_SSID), .max_connection = 4, .authmode = WIFI_AUTH_WPA_WPA2_PSK }
    };
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();
    
    ESP_LOGI(TAG, "Wi-Fi AP запущен: SSID=%s, PASS=%s", WIFI_SSID, WIFI_PASS);
}

void app_main(void) {
    nvs_flash_init();
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    wifi_init_softap();
    start_webserver();
}
