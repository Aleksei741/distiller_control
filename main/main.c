#include <string.h>

#include <parameters.h>
#include <wifi_control.h>
#include <webServer.h>
#include <led.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "esp_http_server.h"

void app_main(void) 
{
    parameters_init(); 
    wifi_init();
    led_init();
    
    wifi_settings_t sta_cfg;
    wifi_settings_t ap_cfg;
    load_wifi_sta_settings(&sta_cfg);
    load_wifi_ap_settings(&ap_cfg);

    wifi_start_ap_sta(&sta_cfg, &ap_cfg);

    init_webserver();
    start_webserver();
}
