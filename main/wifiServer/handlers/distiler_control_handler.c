//******************************************************************************
//include
//******************************************************************************
#include "distiler_control_handler.h"

#include <stdio.h>              // fopen, fread, fclose
#include <string.h>             // strstr, strcmp, strlen

#include "esp_log.h"            // ESP_LOGI, ESP_LOGW, ESP_LOGE
#include "esp_err.h"            // esp_err_t

#include "distiller_control.h"
//******************************************************************************
// Constants
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
static const char *TAG = "[distiler_control_handlers]";

//******************************************************************************
// Function prototype
//******************************************************************************

//******************************************************************************
// Function
//******************************************************************************
esp_err_t get_status_distiler_control_handler(httpd_req_t *req)
{
    dc_status_t status;

    if (!get_dc_status(&status)) 
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }    

    uint8_t buf[28] = {0};
    uint32_t mode = (uint32_t) status.mode;
    memcpy(&buf[0],  &mode,                       sizeof(uint32_t));
    memcpy(&buf[4],  &status.temperature_column,  sizeof(float));
    memcpy(&buf[8],  &status.temperature_kube,    sizeof(float));
    memcpy(&buf[12], &status.temperature_radiator,sizeof(float));
    memcpy(&buf[16], &status.ten_power,           sizeof(float));
    memcpy(&buf[20], &status.voltage_220V,        sizeof(float));
    uint32_t fan = (uint32_t) status.fan;
    memcpy(&buf[24], &fan,                        sizeof(uint32_t));

    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, (const char *)buf, sizeof(buf));

    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t set_ten_power_handler(httpd_req_t *req)
{
    char query[128] = {0};
    char str_index[32] = {0};
    uint32_t power = 0;

    // Прочитать query
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) 
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad query");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Ten power query: %s", query);

    // Извлечь power
    if (httpd_query_key_value(query, "power", str_index, sizeof(str_index)) != ESP_OK) 
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing firts_index");
        return ESP_FAIL;
    }
    power = strtoul(str_index, NULL, 10);
    send_dc_command(DC_SET_TEN_POWER, &power);

    httpd_resp_set_hdr(req, "Connection", "close"); 
    httpd_resp_send(req, NULL, 0); // отправка пустого тела

    return ESP_OK;
}
//------------------------------------------------------------------------------
