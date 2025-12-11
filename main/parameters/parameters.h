#pragma once
//******************************************************************************
// Секция include
//******************************************************************************
#include "esp_err.h" 
//******************************************************************************
// Секция определения констант
//******************************************************************************
#define DEFAULT_PARAM_STA_WIFI_SSID         ""
#define DEFAULT_PARAM_STA_WIFI_PASS         ""
#define DEFAULT_PARAM_AP_WIFI_SSID          "distiller_control"
#define DEFAULT_PARAM_AP_WIFI_PASS          "123456789"
#define MIN_WIFI_PASS_LEN 8
//******************************************************************************
// Секция определения типов
//******************************************************************************
typedef struct 
{
    char ssid[64];
    char pass[64];
} wifi_settings_t;

typedef struct 
{
    uint16_t zero_lvl;
    double ref_voltage;
    double ref_ESR;
} meas_220V_settings_t;
//******************************************************************************
// Секция определения глобальных переменных
//******************************************************************************

//******************************************************************************
// Секция прототипов глобальных функций
//******************************************************************************
void parameters_init(void);

esp_err_t load_wifi_sta_settings(wifi_settings_t *out);
esp_err_t save_wifi_sta_settings(const wifi_settings_t *in);
esp_err_t load_wifi_ap_settings(wifi_settings_t *out);
esp_err_t save_wifi_ap_settings(wifi_settings_t *in);

esp_err_t load_column_rom_temperature_sensor(uint8_t *out_rom);
esp_err_t save_column_rom_temperature_sensor(const uint8_t *in_rom);
esp_err_t load_kube_rom_temperature_sensor(uint8_t *out_rom);
esp_err_t save_kube_rom_temperature_sensor(const uint8_t *in_rom);

esp_err_t load_calibration_220V(meas_220V_settings_t *calib);
esp_err_t save_calibration_220V(const meas_220V_settings_t *calib);
//******************************************************************************
// Секция определения макросов
//******************************************************************************

//******************************************************************************
// ENF OF FILE
//******************************************************************************