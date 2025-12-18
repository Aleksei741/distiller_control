//******************************************************************************
//include
//******************************************************************************
#include "wifi_handler.h"

#include <stdio.h>              // fopen, fread, fclose
#include <string.h>             // strstr, strcmp, strlen

#include "esp_log.h"            // ESP_LOGI, ESP_LOGW, ESP_LOGE
#include "esp_err.h"            // esp_err_t

#include "parameters.h"
#include "wifi_control.h"
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
static const char *TAG = "[wifi_handlers]";

//******************************************************************************
// Function prototype
//******************************************************************************

//******************************************************************************
// Function
//******************************************************************************
esp_err_t wifi_set_ap_handler(httpd_req_t *req)
{
    char query[128] = {0};
    char ssid[64] = {0};
    char pass[64] = {0};

    // Получаем длину query-параметров
    size_t query_len = httpd_req_get_url_query_len(req);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\": \"ok\"}");

    if (query_len == 0) 
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No query params");
        return ESP_FAIL;
    }

    if (query_len + 1 > sizeof(query)) 
    {
        httpd_resp_send_err(req, HTTPD_414_URI_TOO_LONG, "Query too long");
        return ESP_FAIL;
    }

    // Читаем строку вида "ssid=22222222&pass=1111111"
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) 
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad query");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Query: %s", query);

    // Извлекаем ssid
    if (httpd_query_key_value(query, "ssid", ssid, sizeof(ssid)) != ESP_OK) 
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing ssid");
        return ESP_FAIL;
    }

    // Извлекаем pass
    if (httpd_query_key_value(query, "pass", pass, sizeof(pass)) != ESP_OK) 
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing pass");
        return ESP_FAIL;
    }

    // Логируем
    ESP_LOGI(TAG, "SSID: %s", ssid);
    ESP_LOGI(TAG, "PASS: %s", pass);

    wifi_settings_t wifi_settings;
    strncpy(wifi_settings.ssid, ssid, sizeof(wifi_settings.ssid) - 1);
    strncpy(wifi_settings.pass, pass, sizeof(wifi_settings.pass) - 1);      

    // Сохраняем настройки WiFi
    save_wifi_ap_settings(&wifi_settings);
    wifi_reinit_ap(&wifi_settings);

    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t wifi_set_sta_handler(httpd_req_t *req)
{
    char query[128] = {0};
    char ssid[64] = {0};
    char pass[64] = {0};

    // Получаем длину query-параметров
    size_t query_len = httpd_req_get_url_query_len(req);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\": \"ok\"}");

    if (query_len == 0) 
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No query params");
        return ESP_FAIL;
    }

    if (query_len + 1 > sizeof(query)) 
    {
        httpd_resp_send_err(req, HTTPD_414_URI_TOO_LONG, "Query too long");
        return ESP_FAIL;
    }

    // Читаем строку вида "ssid=22222222&pass=1111111"
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) 
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad query");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Query: %s", query);

    // Извлекаем ssid
    if (httpd_query_key_value(query, "ssid", ssid, sizeof(ssid)) != ESP_OK) 
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing ssid");
        return ESP_FAIL;
    }

    // Извлекаем pass
    if (httpd_query_key_value(query, "pass", pass, sizeof(pass)) != ESP_OK) 
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing pass");
        return ESP_FAIL;
    }

    // Логируем
    ESP_LOGI(TAG, "SSID: %s", ssid);
    ESP_LOGI(TAG, "PASS: %s", pass);

    wifi_settings_t wifi_settings;
    strncpy(wifi_settings.ssid, ssid, sizeof(wifi_settings.ssid) - 1);
    strncpy(wifi_settings.pass, pass, sizeof(wifi_settings.pass) - 1);      

    // Сохраняем настройки WiFi
    save_wifi_sta_settings(&wifi_settings);
    wifi_reinit_sta(&wifi_settings);

    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t wifi_sta_start_scan_handler(httpd_req_t *req)
{
    wifi_sta_request_scan();

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"scan_started\"}");
    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t wifi_sta_scan_isdone_handler(httpd_req_t *req)
{
    char buffer[64];
    int is_done = wifi_sta_is_scan_done();

    int len = snprintf(buffer, sizeof(buffer), 
                      "{\"wifi_sta_isdone\": %s}", 
                      is_done ? "true" : "false");
    
    if (len < 0 || len >= sizeof(buffer))
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buffer, len);

    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t wifi_sta_scan_result_handler(httpd_req_t *req)
{
    scanned_ap_info_t aps[MAX_SCAN_APS];
    int count = get_wifi_sta_scan_results(aps);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "[");

    for (int i = 0; i < count; i++) 
    {
        char buf[128];
        snprintf(buf, sizeof(buf),
            "{\"ssid\":\"%s\",\"rssi\":%d,\"auth\":%d}%s",
            aps[i].ssid,
            aps[i].rssi,
            aps[i].authmode,
            (i+1<count)?",":""
        );
        httpd_resp_sendstr_chunk(req, buf);
    }

    httpd_resp_sendstr_chunk(req, "]");
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}
//------------------------------------------------------------------------------