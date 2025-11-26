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