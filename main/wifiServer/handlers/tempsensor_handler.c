//******************************************************************************
//include
//******************************************************************************
#include "tempsensor_handler.h"

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
static const char *TAG = "[tempsensor_handlers]";

//******************************************************************************
// Function prototype
//******************************************************************************

//******************************************************************************
// Function
//******************************************************************************
esp_err_t tempsensor_rom_handler(httpd_req_t *req)
{
    uint8_t rom[8];
    char buf[256];

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "{");

    // Column
    get_dc_parameters(DC_COLUMN_ROM, rom);
    snprintf(buf, sizeof(buf),
             "\"column\": \"0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\"",
             rom[0], rom[1], rom[2], rom[3], rom[4], rom[5], rom[6], rom[7]);
    httpd_resp_sendstr_chunk(req, buf);

    httpd_resp_sendstr_chunk(req, ", ");

    // Kube
    get_dc_parameters(DC_KUBE_ROM, rom);
    snprintf(buf, sizeof(buf),
             "\"kube\": \"0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\"",
             rom[0], rom[1], rom[2], rom[3], rom[4], rom[5], rom[6], rom[7]);
    httpd_resp_sendstr_chunk(req, buf);

    httpd_resp_sendstr_chunk(req, "}");
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t tempsensor_set_rom_cube_handler(httpd_req_t *req)
{
    send_dc_command(DC_REQUEST_INIT_KUBE_ROM);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\": \"kube ROM set\"}");
    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t tempsensor_set_rom_column_handler(httpd_req_t *req)
{
    send_dc_command(DC_REQUEST_INIT_COLUMN_ROM);    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\": \"column ROM set\"}");
    return ESP_OK;
}
//------------------------------------------------------------------------------