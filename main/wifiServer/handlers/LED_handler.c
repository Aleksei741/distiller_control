//******************************************************************************
//include
//******************************************************************************
#include "LED_handler.h"

#include <stdio.h>              // fopen, fread, fclose
#include <string.h>             // strstr, strcmp, strlen

#include "esp_log.h"            // ESP_LOGI, ESP_LOGW, ESP_LOGE
#include "esp_err.h"            // esp_err_t

#include "led.h"
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
static const char *TAG = "[LED_handlers]";

//******************************************************************************
// Function prototype
//******************************************************************************

//******************************************************************************
// Function
//******************************************************************************
esp_err_t get_led_status_handler(httpd_req_t *req) 
{
    char buffer[64];
    int is_led_on = led_status();

    int len = snprintf(buffer, sizeof(buffer), 
                      "{\"led\": %s}", 
                      is_led_on ? "true" : "false");
    
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
esp_err_t set_led_handler(httpd_req_t *req) 
{
    char buffer[64];
    
    // Переключаем LED
    int state = led_status();
    int new_state = !state;
    led_set(new_state);
    
    int len = snprintf(buffer, sizeof(buffer), 
                      "{\"led\": %s}", 
                      new_state ? "true" : "false");
    
    if (len < 0 || len >= sizeof(buffer))
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buffer, len);
    
    return ESP_OK;
}