//******************************************************************************
//include
//******************************************************************************
#include "parameters.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_err.h" 
//******************************************************************************
// Macros
//******************************************************************************
#define LOG_ROM(prefix, rom) \
    ESP_LOGI(TAG, prefix " ROM = %02x %02x %02x %02x %02x %02x %02x %02x", \
             rom[0], rom[1], rom[2], rom[3], rom[4], rom[5], rom[6], rom[7])

//******************************************************************************
// Cinstants
//******************************************************************************
#define KEY_STA_SSID                        "1"
#define KEY_STA_PASS                        "2"
#define KEY_AP_SSID                         "3"
#define KEY_AP_PASS                         "4"

#define KEY_COLUMN_TEMP_SENSOR_ROM          "5"
#define KEY_KUBE_TEMP_SENSOR_ROM            "6"
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
// Parameters wifi 
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
esp_err_t load_wifi_settings(const wifi_config_desc_t *cfg, wifi_settings_t *out)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open("dc", NVS_READONLY, &h);

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
    nvs_open("dc", NVS_READWRITE, &h);
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

    esp_err_t err = load_wifi_settings(&WIFI_AP_CFG, out);
    if (err != ESP_OK) return err;

    if (strlen(out->pass) < MIN_WIFI_PASS_LEN) 
    {
        ESP_LOGW(TAG, "load_wifi_ap_settings: Password too short, use default: %s", 
            DEFAULT_PARAM_AP_WIFI_PASS);
        strncpy(out->pass, DEFAULT_PARAM_AP_WIFI_PASS, sizeof(out->pass));
    }

    return err;
}
//------------------------------------------------------------------------------
esp_err_t save_wifi_ap_settings(wifi_settings_t *in)
{
    const wifi_config_desc_t WIFI_STA_CFG = 
    {
        .type="ap",
        .key_ssid = KEY_AP_SSID,
        .key_pass = KEY_AP_PASS,
        .def_ssid = "",
        .def_pass = ""
    };

    if(in && strlen(in->pass) < MIN_WIFI_PASS_LEN) 
    {
        ESP_LOGW(TAG, "save_wifi_ap_settings: Password too short, not saving.");
        strncpy(in->pass, DEFAULT_PARAM_AP_WIFI_PASS, sizeof(in->pass));
    }

    return save_wifi_settings(&WIFI_STA_CFG, in);
}

//------------------------------------------------------------------------------
// Parameters temperature sensor
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
esp_err_t load_column_rom_temperature_sensor(uint8_t *out_rom)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open("dc", NVS_READONLY, &h);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "load_column_rom_temperature_sensor: nvs_open failed err: %d", err);
        return err;
    }

    size_t required_size = 8;
    err = nvs_get_blob(h, KEY_COLUMN_TEMP_SENSOR_ROM, out_rom, &required_size);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "load_column_rom_temperature_sensor: no saved ROM err: %d", err);
        nvs_close(h);
        return err;
    }

    LOG_ROM("Loaded COLUMN", out_rom);

    nvs_close(h);
    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t save_column_rom_temperature_sensor(const uint8_t *in_rom)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open("dc", NVS_READWRITE, &h);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "save_column_rom_temperature_sensor: nvs_open failed err: %d", err);
        return err;
    }

    err = nvs_set_blob(h, KEY_COLUMN_TEMP_SENSOR_ROM, in_rom, 8);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "save_column_rom_temperature_sensor: nvs_set_blob failed err: %d", err);
        nvs_close(h);
        return err;
    }

    err = nvs_commit(h);
    nvs_close(h);

    LOG_ROM("Saved COLUMN", in_rom);

    return err;
}
//------------------------------------------------------------------------------
esp_err_t load_kube_rom_temperature_sensor(uint8_t *out_rom)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open("dc", NVS_READONLY, &h);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "load_kube_rom_temperature_sensor: nvs_open failed err: %d", err);
        return err;
    }

    size_t required_size = 8;
    err = nvs_get_blob(h, KEY_KUBE_TEMP_SENSOR_ROM, out_rom, &required_size);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "load_kube_rom_temperature_sensor: no saved ROM err: %d", err);
        nvs_close(h);
        return err;
    }

    LOG_ROM("Loaded KUBE", out_rom);

    nvs_close(h);
    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t save_kube_rom_temperature_sensor(const uint8_t *in_rom)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open("dc", NVS_READWRITE, &h);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "save_kube_rom_temperature_sensor: nvs_open failed err: %d", err);
        return err;
    }

    err = nvs_set_blob(h, KEY_KUBE_TEMP_SENSOR_ROM, in_rom, 8);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "save_kube_rom_temperature_sensor: nvs_set_blob failed err: %d", err);
        nvs_close(h);
        return err;
    }

    err = nvs_commit(h);
    nvs_close(h);

    LOG_ROM("Saved KUBE", in_rom);

    return err;
}

