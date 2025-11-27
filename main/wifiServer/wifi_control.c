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
void wifi_start_ap_sta(const wifi_settings_t *cfg_sta, const wifi_settings_t *cfg_ap)
{
    // Запоминаем STA конфиг
    strncpy(g_sta_cfg.ssid, cfg_sta->ssid, sizeof(g_sta_cfg.ssid));
    strncpy(g_sta_cfg.pass, cfg_sta->pass, sizeof(g_sta_cfg.pass));

    // Запоминаем AP конфиг
    strncpy(g_ap_cfg.ssid, cfg_ap->ssid, sizeof(g_ap_cfg.ssid));
    strncpy(g_ap_cfg.pass, cfg_ap->pass, sizeof(g_ap_cfg.pass));

    // Базовая системная инициализация
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

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

    ESP_LOGI(TAG, "wifi_start_ap_sta. STA SSID=%s, PASS=%s", 
        sta_cfg.sta.ssid, sta_cfg.sta.password);

    ESP_LOGI(TAG, "wifi_start_ap_sta. AP SSID=%s, PASS=%s", 
        ap_cfg.ap.ssid, ap_cfg.ap.password);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP,  &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}

//------------------------------------------------------------------------------
void wifi_reinit_sta(const wifi_settings_t *new_cfg_sta)
{
    strncpy(g_sta_cfg.ssid, new_cfg_sta->ssid, sizeof(g_sta_cfg.ssid));
    strncpy(g_sta_cfg.pass, new_cfg_sta->pass, sizeof(g_sta_cfg.pass));

    ESP_ERROR_CHECK(esp_wifi_stop());

    wifi_config_t sta_cfg = {0};
    strcpy((char*)sta_cfg.sta.ssid, g_sta_cfg.ssid);
    strcpy((char*)sta_cfg.sta.password, g_sta_cfg.pass);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}
//------------------------------------------------------------------------------
void wifi_reinit_ap(const wifi_settings_t *new_cfg_ap)
{
    // Сохраняем строки в глобальные переменные
    strncpy(g_ap_cfg.ssid, new_cfg_ap->ssid, sizeof(g_ap_cfg.ssid));
    strncpy(g_ap_cfg.pass, new_cfg_ap->pass, sizeof(g_ap_cfg.pass));

    wifi_config_t ap_cfg = {
        .ap = {.max_connection = 4, .authmode = WIFI_AUTH_WPA2_PSK}
    };

    strcpy((char*)ap_cfg.ap.ssid, g_ap_cfg.ssid);
    strcpy((char*)ap_cfg.ap.password, g_ap_cfg.pass);

    if (strlen(g_ap_cfg.pass) == 0)
        ap_cfg.ap.authmode = WIFI_AUTH_OPEN;

    // Просто применяем новые настройки AP
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));

    // Дополнительно можно "перезапустить" рекламу AP без остановки STA:
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_start());
}

