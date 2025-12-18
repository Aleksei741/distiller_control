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
    char buf[128];

    if (!get_dc_status(&status)) 
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "{");

    // Column Temperature
    snprintf(buf, sizeof(buf), "\"temperature_column\": %.2f", status.temperature_column);
    httpd_resp_sendstr_chunk(req, buf);

    httpd_resp_sendstr_chunk(req, ", ");

    // Kube Temperature
    snprintf(buf, sizeof(buf), "\"temperature_kube\": %.2f", status.temperature_kube);
    httpd_resp_sendstr_chunk(req, buf);

    httpd_resp_sendstr_chunk(req, ", ");

    // Radiator Temperature
    snprintf(buf, sizeof(buf), "\"temperature_radiator\": %.2f", status.temperature_radiator);
    httpd_resp_sendstr_chunk(req, buf);

    httpd_resp_sendstr_chunk(req, "}");
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}
//------------------------------------------------------------------------------
