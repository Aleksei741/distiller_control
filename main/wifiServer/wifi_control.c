//******************************************************************************
//include
//******************************************************************************
#include "wifi_control.h"
#include "parameters.h"

#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/ip4_addr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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
static const char *TAG = "[wifi]";
static wifi_settings_t g_sta_cfg;
static wifi_settings_t g_ap_cfg;

static SemaphoreHandle_t scan_mutex = NULL;
static scanned_ap_info_t scan_results[MAX_SCAN_APS];
static int scan_count = 0;
static bool scan_requested = false;
static bool scan_done = false;
//------------------------------------------------------------------------------
// Local Class
//------------------------------------------------------------------------------

//******************************************************************************
// Function prototype
//******************************************************************************
static void wifi_sta_scan_task(void *arg);
//******************************************************************************
// Function
//******************************************************************************
void wifi_init(void)
{
    // 1. Инициализация системных компонентов
    
    // Инициализация TCP/IP стека
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Создание цикла обработки событий по умолчанию
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 2. Создание сетевых интерфейсов по умолчанию
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    // 3. Инициализация Wi-Fi драйвера
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    scan_mutex = xSemaphoreCreateMutex();
    xTaskCreate(wifi_sta_scan_task, "wifi_ap_scan_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Wi-Fi initialization successful.");
}
//------------------------------------------------------------------------------
void wifi_start_ap_sta(const wifi_settings_t *cfg_sta, const wifi_settings_t *cfg_ap)
{
    // Запоминаем STA конфиг
    strncpy(g_sta_cfg.ssid, cfg_sta->ssid, sizeof(g_sta_cfg.ssid));
    strncpy(g_sta_cfg.pass, cfg_sta->pass, sizeof(g_sta_cfg.pass));

    // Запоминаем AP конфиг
    strncpy(g_ap_cfg.ssid, cfg_ap->ssid, sizeof(g_ap_cfg.ssid));
    strncpy(g_ap_cfg.pass, cfg_ap->pass, sizeof(g_ap_cfg.pass));

    // --- Конфигурация STA ---
    wifi_config_t sta_cfg = {0};
    strcpy((char*)sta_cfg.sta.ssid, g_sta_cfg.ssid);
    strcpy((char*)sta_cfg.sta.password, g_sta_cfg.pass);

    // --- Конфигурация AP ---
    wifi_config_t ap_cfg = 
    {
        .ap = {.max_connection = 4, .authmode = WIFI_AUTH_WPA2_PSK}
    };
    strcpy((char*)ap_cfg.ap.ssid, g_ap_cfg.ssid);
    strcpy((char*)ap_cfg.ap.password, g_ap_cfg.pass);
    if (strlen(g_ap_cfg.pass) == 0) 
        ap_cfg.ap.authmode = WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    bool sta_enabled = (strlen((const char*)sta_cfg.sta.ssid) > 0);

    if (sta_enabled) 
    {        
        ESP_LOGI(TAG, "wifi_start_ap_sta. STA SSID=%s, PASS=%s", 
            sta_cfg.sta.ssid, sta_cfg.sta.password);
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
    } 
    else
    {
        wifi_config_t empty_sta = {0};
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &empty_sta));
        ESP_LOGW(TAG, "wifi_start_ap_sta. STA enabled for empty SSID");
    }

    ESP_LOGI(TAG, "wifi_start_ap_sta. AP SSID=%s, PASS=%s", 
            ap_cfg.ap.ssid, ap_cfg.ap.password);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP,  &ap_cfg));

    ESP_ERROR_CHECK(esp_wifi_start());

    esp_netif_t* ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (ap_netif) {
        // Назначаем статический IP для AP
        esp_netif_ip_info_t ip_info;
        memset(&ip_info, 0, sizeof(esp_netif_ip_info_t));
        
        // IP адрес точки доступа: 192.168.4.1
        ip_info.ip.addr = ipaddr_addr("192.168.4.1");
        ip_info.gw.addr = ipaddr_addr("192.168.4.1");     // Шлюз = сам AP
        ip_info.netmask.addr = ipaddr_addr("255.255.255.0");
        
        esp_netif_set_ip_info(ap_netif, &ip_info);
        
        ESP_LOGI(TAG, "AP IP address set to: 192.168.4.1");
    }

    if (sta_enabled) 
    {
        ESP_ERROR_CHECK(esp_wifi_connect());
        ESP_LOGI(TAG, "Connecting to STA network...");
    } 
    else 
    {
        ESP_LOGI(TAG, "Running in AP-only mode");
    }
}
//------------------------------------------------------------------------------
void wifi_reinit_sta(const wifi_settings_t *new_cfg_sta)
{
    if (!new_cfg_sta) return;

    // 1. Сохраняем новый конфиг
    strncpy(g_sta_cfg.ssid, new_cfg_sta->ssid, sizeof(g_sta_cfg.ssid));
    strncpy(g_sta_cfg.pass, new_cfg_sta->pass, sizeof(g_sta_cfg.pass));

    // 2. Подготовка структуры конфигурации STA
    wifi_config_t sta_cfg = {0};
    strcpy((char*)sta_cfg.sta.ssid, g_sta_cfg.ssid);
    strcpy((char*)sta_cfg.sta.password, g_sta_cfg.pass);

    ESP_LOGI(TAG, "wifi_reinit_sta. STA SSID=%s, PASS=%s", 
             sta_cfg.sta.ssid, sta_cfg.sta.password);

    // 3. Разрываем текущее соединение STA, если было
    ESP_ERROR_CHECK(esp_wifi_disconnect());

    // 4. Устанавливаем новый конфиг
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));

    // 5. Подключаемся к новой сети
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "Reconnecting to new STA network...");
}
//------------------------------------------------------------------------------
static void wifi_sta_scan_task(void *arg)
{
    wifi_scan_config_t cfg = {0};
    esp_err_t err;

    while (1) 
    {
        xSemaphoreTake(scan_mutex, portMAX_DELAY);
        bool do_scan = scan_requested;
        xSemaphoreGive(scan_mutex);

        if (do_scan) 
        {
            wifi_ap_record_t ap_info;
            err = esp_wifi_sta_get_ap_info(&ap_info);

            if (err == ESP_OK)
            {
                ESP_LOGW(TAG, "STA is connected to %s — disconnecting before scan",
                         (char*)ap_info.ssid);

                esp_wifi_disconnect();        // корректно отключаем
                vTaskDelay(pdMS_TO_TICKS(300)); // даём стэку отвалиться
            }

            ESP_LOGI(TAG, "Starting WiFi scan...");
            err = esp_wifi_scan_start(&cfg, true);
            if (err == ESP_OK) 
            {
                uint16_t ap_num = MAX_SCAN_APS;
                wifi_ap_record_t ap_records[MAX_SCAN_APS];

                if (esp_wifi_scan_get_ap_records(&ap_num, ap_records) == ESP_OK) 
                {
                    xSemaphoreTake(scan_mutex, portMAX_DELAY);

                    scan_count = ap_num;
                    for (int i = 0; i < ap_num; i++) 
                    {
                        strncpy(scan_results[i].ssid, (char *)ap_records[i].ssid, sizeof(scan_results[i].ssid)-1);
                        scan_results[i].ssid[sizeof(scan_results[i].ssid)-1] = '\0';
                        scan_results[i].rssi     = ap_records[i].rssi;
                        scan_results[i].authmode = ap_records[i].authmode;
                    }

                    scan_requested = false;
                    scan_done = true;

                    xSemaphoreGive(scan_mutex);

                    ESP_LOGI(TAG, "WiFi scan done, found %d APs", ap_num);
                }
            } 
            else 
            {
                ESP_LOGE(TAG, "WiFi scan failed: %s", esp_err_to_name(err));

                xSemaphoreTake(scan_mutex, portMAX_DELAY);
                scan_count = 0;
                scan_requested = false;
                scan_done = true;
                xSemaphoreGive(scan_mutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
//------------------------------------------------------------------------------
int get_wifi_sta_scan_results(scanned_ap_info_t *aps)
{
    xSemaphoreTake(scan_mutex, portMAX_DELAY);
    for (int i = 0; i < scan_count; i++) 
    {
        aps[i] = scan_results[i];
    }
    xSemaphoreGive(scan_mutex);

    return scan_count;
}
//------------------------------------------------------------------------------
void wifi_sta_request_scan(void)
{
    xSemaphoreTake(scan_mutex, portMAX_DELAY);
    scan_requested = true;
    scan_done = false;
    xSemaphoreGive(scan_mutex);
}
//------------------------------------------------------------------------------
int wifi_sta_is_scan_done(void)
{
    xSemaphoreTake(scan_mutex, portMAX_DELAY);
    bool done = scan_done;
    xSemaphoreGive(scan_mutex);
    return (int)done;
}
//------------------------------------------------------------------------------
void wifi_reinit_ap(const wifi_settings_t *new_cfg_ap)
{
    if (!new_cfg_ap) return;

    // 1. Сохраняем новые настройки в глобальные переменные
    strncpy(g_ap_cfg.ssid, new_cfg_ap->ssid, sizeof(g_ap_cfg.ssid));
    strncpy(g_ap_cfg.pass, new_cfg_ap->pass, sizeof(g_ap_cfg.pass));

    // 2. Формируем структуру конфигурации AP
    wifi_config_t ap_cfg = {
        .ap = {.max_connection = 4, .authmode = WIFI_AUTH_WPA2_PSK}
    };

    strcpy((char*)ap_cfg.ap.ssid, g_ap_cfg.ssid);
    strcpy((char*)ap_cfg.ap.password, g_ap_cfg.pass);

    if (strlen(g_ap_cfg.pass) == 0)
        ap_cfg.ap.authmode = WIFI_AUTH_OPEN;

    ESP_LOGI(TAG, "wifi_reinit_ap. AP SSID=%s, PASS=%s", 
             ap_cfg.ap.ssid, ap_cfg.ap.password);

    // 3. Применяем новые настройки AP без остановки Wi‑Fi
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));

    // 4. Обновляем IP для AP (если нужен статический)
    esp_netif_t* ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (ap_netif) {
        esp_netif_ip_info_t ip_info;
        memset(&ip_info, 0, sizeof(ip_info));

        ip_info.ip.addr = ipaddr_addr("192.168.4.1");
        ip_info.gw.addr = ipaddr_addr("192.168.4.1");
        ip_info.netmask.addr = ipaddr_addr("255.255.255.0");

        esp_netif_set_ip_info(ap_netif, &ip_info);
        ESP_LOGI(TAG, "AP IP address set to: 192.168.4.1");
    }

    ESP_LOGI(TAG, "AP configuration updated successfully");
}
//------------------------------------------------------------------------------