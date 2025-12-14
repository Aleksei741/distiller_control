set(SOURCES
    main.c
    controller/distiller_control.c
    parameters/parameters.c
    wifiServer/wifi_control.c
    wifiServer/webServer.c
    wifiServer/mDNS_.c
    wifiServer/captive_dns.c
    periferial/led.c
    periferial/temperature_sensor.c
    periferial/statistic.c
    periferial/drivers/ds18b20.c
    periferial/adc_drv.c
    controller/algorithm/PID.c    
    controller/algorithm/Measure220V.c
    controller/algorithm/statistic_sampler.c
)

set(INCLUDES
    "."
    controller
    controller/algorithm
    wifiServer
    parameters
    periferial
    periferial/drivers
)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS ${INCLUDES}
                       PRIV_REQUIRES spiffs
                       REQUIRES esp_driver_gpio esp_http_server driver esp_wifi esp_netif esp_event nvs_flash wpa_supplicant vfs lwip esp_adc)
