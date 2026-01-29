//******************************************************************************
//include
//******************************************************************************
#include "flow_direction_handler.h"

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
static const char *TAG = "[flow_direction_handler]";

//******************************************************************************
// Function prototype
//******************************************************************************

//******************************************************************************
// Function
//******************************************************************************
esp_err_t set_position_flow_direction_handler(httpd_req_t *req) 
{
    char query[128] = {0};
    char str_index[32] = {0};
    uint32_t position = 0;

    // Прочитать query
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad query");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Flow direction query: %s", query);

    // Извлечь position
    if (httpd_query_key_value(query, "posotion", str_index, sizeof(str_index)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing firts_index");
        return ESP_FAIL;
    }
    position = strtoul(str_index, NULL, 10);

    if(position < 1 || position > 3) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid position value");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Set flow direction position to: %d", position);

    send_dc_command(DC_COMMAND_SET_FLOW_DIRECTION, &position);

    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, NULL, 0); // отправка пустого тела

    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t set_parameters_flow_direction_handler(httpd_req_t *req) 
{
    char query[128] = {0};
    char str_index[32] = {0};
    int position = 0;
    uint16_t angle = 0;

    // Прочитать query
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad query");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Flow direction query: %s", query);

    // Извлечь position
    if (httpd_query_key_value(query, "posotion", str_index, sizeof(str_index)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing firts_index");
        return ESP_FAIL;
    }
    position = strtoul(str_index, NULL, 10);

    // Извлечь angle
    if (httpd_query_key_value(query, "angle", str_index, sizeof(str_index)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing angle");
        return ESP_FAIL;
    }
    angle = strtoul(str_index, NULL, 10);

    if(position < 1 || position > 3) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid position value");
        return ESP_FAIL;
    }

    if(angle > 360) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid angle value");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Set parameters flow direction position %d angle %d", position, angle);

    if(position == 1)
        set_dc_parameters(DC_PARAM_FLOW_DIRECTION_ANGLE_1, &angle);
    else if(position == 2)
        set_dc_parameters(DC_PARAM_FLOW_DIRECTION_ANGLE_2, &angle);
    else if(position == 3)
        set_dc_parameters(DC_PARAM_FLOW_DIRECTION_ANGLE_3, &angle);

    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, NULL, 0); // отправка пустого тела

    return ESP_OK;
}
//------------------------------------------------------------------------------
esp_err_t get_parameters_flow_direction_handler(httpd_req_t *req)
{
    uint16_t angle_1 = 0;
    uint16_t angle_2 = 0;   
    uint16_t angle_3 = 0;

    get_dc_parameters(DC_PARAM_FLOW_DIRECTION_ANGLE_1, &angle_1);
    get_dc_parameters(DC_PARAM_FLOW_DIRECTION_ANGLE_2, &angle_2);
    get_dc_parameters(DC_PARAM_FLOW_DIRECTION_ANGLE_3, &angle_3);

    ESP_LOGI(TAG, "Get parameters flow direction angle_1=%d angle_2=%d angle_3=%d", 
        angle_1, angle_2, angle_3);

    uint8_t buf[12] = {0};
    uint32_t _angle_1 = (uint32_t) angle_1;
    memcpy(&buf[0], &_angle_1, sizeof(uint32_t));
    uint32_t _angle_2 = (uint32_t) angle_2;
    memcpy(&buf[4], &_angle_2, sizeof(uint32_t));
    uint32_t _angle_3 = (uint32_t) angle_3;
    memcpy(&buf[8], &_angle_3, sizeof(uint32_t));

    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_send(req, (const char *)buf, sizeof(buf));

    httpd_resp_set_hdr(req, "Connection", "close");
    return ESP_OK;
}