set(SOURCES
    main.c
    parameters/parameters.c
    wifiServer/wifi_control.c
)

set(INCLUDES
    "."
    wifiServer
    parameters
)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS ${INCLUDES}
                       PRIV_REQUIRES spi_flash
                       REQUIRES esp_driver_gpio esp_http_server driver esp_wifi esp_netif esp_event nvs_flash)
