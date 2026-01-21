set(SOURCES
    main.c
    controller/distiller_control.c
    controller/algorithm/PID.c    
    controller/algorithm/Measure220V.c
    controller/algorithm/statistic_sampler.c
    parameters/parameters.c
    wifiServer/wifi_control.c
    wifiServer/webServer.c
    wifiServer/mDNS_.c
    wifiServer/captive_dns.c
    wifiServer/handlers/distiler_control_handler.c
    wifiServer/handlers/LED_handler.c
    wifiServer/handlers/statistic_handler.c
    wifiServer/handlers/tempsensor_handler.c
    wifiServer/handlers/wifi_handler.c
    periferial/led.c
    periferial/temperature_sensor.c
    periferial/statistic.c
    periferial/drivers/ds18b20.c
    periferial/adc_drv.c
    periferial/modbus/modbus_master.c
    periferial/modbus/driver/modbus_crc.c
    periferial/modbus/driver/modbus_rtu_coils.c
    periferial/modbus/driver/modbus_rtu_discrete_input.c
    periferial/modbus/driver/modbus_rtu_holding_registers.c
    periferial/modbus/driver/modbus_rtu_input_registers.c    
    periferial/ten_ctrl.c
)

set(INCLUDES
    "."
    controller
    controller/algorithm
    wifiServer
    wifiServer/handlers
    parameters
    periferial
    periferial/drivers
    periferial/modbus
    periferial/modbus/driver
)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS ${INCLUDES}
                       PRIV_REQUIRES spiffs
                       REQUIRES esp_driver_uart esp_driver_gpio esp_http_server driver esp_wifi esp_netif esp_event nvs_flash wpa_supplicant vfs lwip esp_adc)