//******************************************************************************
//include
//******************************************************************************
#include "parameters.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_err.h" 
//******************************************************************************
// Cinstants
//******************************************************************************
#define KEY_STA_SSID                        "ssid_sta"
#define KEY_STA_PASS                        "pass_sta"
#define KEY_AP_SSID                         "ssid_ap"
#define KEY_AP_PASS                         "pass_ap"
//******************************************************************************
// Type
//******************************************************************************
typedef struct 
{
    const char *type;
    const char *key_ssid;
    const char *key_pass;
    const char *def_ssid;
    const char *def_pass;
} wifi_config_desc_t;
//******************************************************************************
// Variables
//******************************************************************************
//------------------------------------------------------------------------------
// Global
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local Variable
//------------------------------------------------------------------------------
static const char *TAG = "[parameters]";
//------------------------------------------------------------------------------
// Local Class
//------------------------------------------------------------------------------

//******************************************************************************
// Function prototype
//******************************************************************************
esp_err_t load_wifi_settings(const wifi_config_desc_t *cfg, wifi_settings_t *out);
esp_err_t save_wifi_settings(const wifi_config_desc_t *cfg, const wifi_settings_t *in);

//******************************************************************************
// Function
//******************************************************************************

//------------------------------------------------------------------------------
// Parameters wifi sta
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
esp_err_t load_wifi_settings(const wifi_config_desc_t *cfg, wifi_settings_t *out)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open("wifi_cfg", NVS_READONLY, &h);

    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "load_wifi_%s_settings: nvs_open. default SSID=%s, PASS=%s", 
            cfg->type, cfg->def_ssid, cfg->def_pass);

        strncpy(out->ssid, cfg->def_ssid, sizeof(out->ssid));
        out->ssid[sizeof(out->ssid)-1] = '\0';

        strncpy(out->pass, cfg->def_pass, sizeof(out->pass));
        out->pass[sizeof(out->pass)-1] = '\0';

        save_wifi_settings(cfg, out);
        return ESP_OK;
    }

    size_t s1 = sizeof(out->ssid);
    size_t s2 = sizeof(out->pass);

    if (nvs_get_str(h, cfg->key_ssid, out->ssid, &s1) != ESP_OK) 
    {
        ESP_LOGE(TAG, "load_wifi_%s_settings: nvs_get_str. default SSID=%s", 
            cfg->type, cfg->def_ssid);

        strncpy(out->ssid, cfg->def_ssid, sizeof(out->ssid));
        out->ssid[sizeof(out->ssid)-1] = '\0';
    }

    if (nvs_get_str(h, cfg->key_pass, out->pass, &s2) != ESP_OK) 
    {
        ESP_LOGE(TAG, "load_wifi_%s_settings: nvs_get_str. default PASS=%s", 
            cfg->type, cfg->def_pass);
            
        strncpy(out->pass, cfg->def_pass, sizeof(out->pass));
        out->pass[sizeof(out->pass)-1] = '\0';
    }

    ESP_LOGI(TAG, "load_wifi_%s_settings. SSID=%s, PASS=%s", 
        cfg->type, out->ssid, out->pass);

    nvs_close(h);
    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t save_wifi_settings(const wifi_config_desc_t *cfg, const wifi_settings_t *in)
{
    nvs_handle_t h;
    nvs_open("wifi_cfg", NVS_READWRITE, &h);
    nvs_set_str(h, cfg->key_ssid, in->ssid);
    nvs_set_str(h, cfg->key_pass, in->pass);
    nvs_commit(h);
    nvs_close(h);

    ESP_LOGI(TAG, "save_wifi_%s_settings. SSID=%s, PASS=%s", 
        cfg->type, in->ssid, in->pass);

    return ESP_OK;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void parameters_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_LOGW(TAG, "NVS partition is full or has new version, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "NVS initialized successfully");
}
//------------------------------------------------------------------------------
esp_err_t load_wifi_sta_settings(wifi_settings_t *out)
{
    const wifi_config_desc_t WIFI_STA_CFG = 
    {
        .type="sta",
        .key_ssid = KEY_STA_SSID,
        .key_pass = KEY_STA_PASS,
        .def_ssid = DEFAULT_PARAM_STA_WIFI_SSID,
        .def_pass = DEFAULT_PARAM_STA_WIFI_PASS
    };

    return load_wifi_settings(&WIFI_STA_CFG, out);
}
//------------------------------------------------------------------------------
esp_err_t save_wifi_sta_settings(const wifi_settings_t *in)
{
    const wifi_config_desc_t WIFI_STA_CFG = 
    {
        .type="sta",
        .key_ssid = KEY_STA_SSID,
        .key_pass = KEY_STA_PASS,
        .def_ssid = "",
        .def_pass = ""
    };

    return save_wifi_settings(&WIFI_STA_CFG, in);
}
//------------------------------------------------------------------------------
esp_err_t load_wifi_ap_settings(wifi_settings_t *out)
{
    const wifi_config_desc_t WIFI_AP_CFG = 
    {
        .type="ap",
        .key_ssid = KEY_AP_SSID,
        .key_pass = KEY_AP_PASS,
        .def_ssid = DEFAULT_PARAM_AP_WIFI_SSID,
        .def_pass = DEFAULT_PARAM_AP_WIFI_PASS
    };

    return load_wifi_settings(&WIFI_AP_CFG, out);
}
//------------------------------------------------------------------------------
esp_err_t save_wifi_ap_settings(const wifi_settings_t *in)
{
    const wifi_config_desc_t WIFI_STA_CFG = 
    {
        .type="ap",
        .key_ssid = KEY_AP_SSID,
        .key_pass = KEY_AP_PASS,
        .def_ssid = "",
        .def_pass = ""
    };

    return save_wifi_settings(&WIFI_STA_CFG, in);
}
//------------------------------------------------------------------------------
