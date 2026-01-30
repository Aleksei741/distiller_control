//******************************************************************************
//include
//******************************************************************************
#include <stdio.h>              // fopen, fread, fclose
#include <string.h>             // strstr, strcmp, strlen

#include "esp_log.h"            // ESP_LOGI, ESP_LOGW, ESP_LOGE
#include "esp_err.h"            // esp_err_t

#include "api_PID_handler.h"
#include "PID.h"
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
static const char *TAG = "[api_PID_handler]";

//******************************************************************************
// Function prototype
//******************************************************************************

//******************************************************************************
// Function
//******************************************************************************
esp_err_t api_set_PID_handler(httpd_req_t *req)
{
    char query[128] = {0};
    char str_index[32] = {0};
    char name_module[32] = {0};
    char name_parameter[32] = {0};
    double value = 0;

    PID_t pid = {0};
    dc_parametes_e dc_param;

    // Прочитать query
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad query");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "api_set_PID_handler Flow direction query: %s", query);

    // Извлечь name_module
    if (httpd_query_key_value(query, "module", name_module, sizeof(name_module)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing name_module");
        return ESP_FAIL;
    }

    // Извлечь name_parameter
    if (httpd_query_key_value(query, "parameter", name_parameter, sizeof(name_parameter)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing name_parameter");
        return ESP_FAIL;
    }
    
    // Извлечь value
    if (httpd_query_key_value(query, "value", str_index, sizeof(str_index)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing value");
        return ESP_FAIL;
    }
    value = strtod(str_index, NULL);

    ESP_LOGI(TAG, "api_set_PID_handler Set parameters flow direction module: %s, parameter: %s, value: %.2f", name_module, name_parameter, value);

    if(strcmp(name_module, "autoclave_mode") == 0)
    {
        dc_param = DC_PARAM_AUTOCLAVE_MODE_PID_PARAMETERS;
    }
    else
    {
        ESP_LOGE(TAG, "Invalid name_module value: %s", name_module);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid name_module value");
        return ESP_FAIL;
    }

    if(get_dc_parameters(dc_param, &pid))
    {
        ESP_LOGE(TAG, "Failed to get_dc_parameters");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get PID parameters");
        return ESP_FAIL;
    }
    
    if(strcmp(name_parameter, "Kp") == 0)
        pid.param.Kp = (float)value;
    else if(strcmp(name_parameter, "Ki") == 0)
        pid.param.Ki = (float)value;
    else if(strcmp(name_parameter, "Kd") == 0)
        pid.param.Kd = (float)value;
    else if(strcmp(name_parameter, "out_min") == 0)
        pid.param.out_min = (float)value;
    else if(strcmp(name_parameter, "out_max") == 0)
        pid.param.out_max = (float)value;
    else if(strcmp(name_parameter, "dt") == 0)
        pid.param.dt = (float)value;
    else if(strcmp(name_parameter, "integr_min") == 0)
        pid.param.integr_min = (float)value;
    else if(strcmp(name_parameter, "integr_max") == 0)
        pid.param.integr_max = (float)value;
    else if(strcmp(name_parameter, "d_filter") == 0)
        pid.param.d_filter = (float)value;
    else
    {
        ESP_LOGE(TAG, "Invalid name_parameter value: %s", name_parameter);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid name_parameter value");
        return ESP_FAIL;
    }

    set_dc_parameters(DC_PARAM_AUTOCLAVE_MODE_PID_PARAMETERS, &pid);

    ESP_LOGI(TAG, "Set autoclave mode PID parameter %s to %.2f", name_parameter, value);

    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, NULL, 0); // отправка пустого тела

    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t autoclave_mode_get_PID_handler(httpd_req_t *req)
{
    PID_t pid = {0};
    char query[128] = {0};
    char name_module[32] = {0};
    dc_parametes_e dc_param;

    // Прочитать query
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad query");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "api_set_PID_handler Flow direction query: %s", query);

    // Извлечь name_module
    if (httpd_query_key_value(query, "name_module", name_module, sizeof(name_module)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing name_module");
        return ESP_FAIL;
    }

    if(strcmp(name_module, "autoclave_mode") == 0)
    {
        dc_param = DC_PARAM_AUTOCLAVE_MODE_PID_PARAMETERS;
    }
    else
    {
        ESP_LOGE(TAG, "Invalid name_module value: %s", name_module);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid name_module value");
        return ESP_FAIL;
    }

    get_dc_parameters(dc_param, &pid);

    uint8_t buf[72] = {0};
    memcpy(&buf[0], &pid.param.Kp, sizeof(float));
    memcpy(&buf[4], &pid.param.Ki, sizeof(float));
    memcpy(&buf[8], &pid.param.Kd, sizeof(float));
    memcpy(&buf[12], &pid.param.out_min, sizeof(float));
    memcpy(&buf[16], &pid.param.out_max, sizeof(float));
    memcpy(&buf[20], &pid.param.dt, sizeof(float));
    memcpy(&buf[24], &pid.param.integr_min, sizeof(float));
    memcpy(&buf[28], &pid.param.integr_max, sizeof(float));
    memcpy(&buf[32], &pid.param.d_filter, sizeof(float));
    memcpy(&buf[36], &pid.status.p_term, sizeof(float));
    memcpy(&buf[40], &pid.status.i_term, sizeof(float));
    memcpy(&buf[44], &pid.status.d_term, sizeof(float));
    memcpy(&buf[48], &pid.status.integral, sizeof(float));
    memcpy(&buf[52], &pid.status.prev_error, sizeof(float));
    memcpy(&buf[56], &pid.status.prev_deriv, sizeof(float));
    memcpy(&buf[60], &pid.status.setpoint, sizeof(float));
    memcpy(&buf[64], &pid.status.measurement, sizeof(float));
    memcpy(&buf[68], &pid.status.output, sizeof(float));
    
    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_send(req, (const char *)buf, sizeof(buf));

    httpd_resp_set_hdr(req, "Connection", "close");
    return ESP_OK;
}