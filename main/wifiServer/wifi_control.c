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

//------------------------------------------------------------------------------
// Local Class
//------------------------------------------------------------------------------

//******************************************************************************
// Function prototype
//******************************************************************************

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

    bool sta_enabled = (strlen((const char*)sta_cfg.sta.ssid) > 0);

    if (sta_enabled) 
    {        
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
        
        ESP_LOGI(TAG, "wifi_start_ap_sta. STA SSID=%s, PASS=%s", 
            sta_cfg.sta.ssid, sta_cfg.sta.password);

        ESP_LOGI(TAG, "wifi_start_ap_sta. AP SSID=%s, PASS=%s", 
            ap_cfg.ap.ssid, ap_cfg.ap.password);
        
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP,  &ap_cfg));
    } 
    else 
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_LOGW(TAG, "wifi_start_ap_sta. STA disabled (empty SSID)");
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP,  &ap_cfg));
    }
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
int wifi_scan_ap(scanned_ap_info_t *ap_list, int max_count)
{
    // Проверка на корректность входных данных
    if (ap_list == NULL || max_count <= 0) 
    {
        return 0;
    }
    
    // Конфигурация сканирования
    wifi_scan_config_t scan_config = {
        .ssid = NULL,       // Сканировать все SSID
        .bssid = NULL,      // Сканировать все BSSID
        .channel = 0,       // Сканировать все каналы
        .show_hidden = false // Игнорировать скрытые сети
    };

    uint16_t ap_count = max_count;
    wifi_ap_record_t ap_records[max_count];
    esp_err_t err;

    ESP_LOGI(TAG, "wifi_scan_ap scan start");
    
    // Запуск сканирования (блокирующее: true)
    err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi_scan_ap scan failed: %s", esp_err_to_name(err));
        return 0;
    }

    // Получение количества найденных AP
    err = esp_wifi_scan_get_ap_num(&ap_count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi_scan_ap error get AP number: %s", esp_err_to_name(err));
        return 0;
    }
    
    // Ограничение количества результатов
    if (ap_count > max_count) {
        ap_count = max_count;
    }

    // Получение записей AP
    err = esp_wifi_scan_get_ap_records(&ap_count, ap_records);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi_scan_ap error get AP records: %s", esp_err_to_name(err));
        return 0;
    }

    ESP_LOGI(TAG, "wifi_scan_ap Found %d access points", ap_count);
    
    // Копирование данных в выходную структуру и очистка
    for (int i = 0; i < ap_count; i++) {
        // Копирование SSID
        strncpy(ap_list[i].ssid, (char *)ap_records[i].ssid, sizeof(ap_list[i].ssid) - 1);
        ap_list[i].ssid[sizeof(ap_list[i].ssid) - 1] = '\0';

        // Копирование RSSI (уровня сигнала)
        ap_list[i].rssi = ap_records[i].rssi;
        
        // Копирование режима авторизации
        ap_list[i].authmode = ap_records[i].authmode;

        ESP_LOGD(TAG, "wifi_scan_ap SSID: %s, RSSI: %d, Authmode: %d", 
                 ap_list[i].ssid, ap_list[i].rssi, ap_list[i].authmode);
    }
    
    // Очистка результатов сканирования
    // esp_wifi_scan_get_ap_records() обычно очищает, но можно добавить явный вызов

    // Может быть использован для асинхронного сканирования
    // esp_wifi_scan_stop(); 

    return ap_count;
}
