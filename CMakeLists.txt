cmake_minimum_required(VERSION 3.16)

# Подключаем ESP-IDF
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(distiller_control)

# Указываем каталог для SPIFFS/LittleFS
set(SPIFFS_IMAGE_DIR "${CMAKE_SOURCE_DIR}/spiffs")

# В ESP-IDF v6.0 используйте spiffs или littlefs компоненты
# Добавляем spiffs_create_partition_image для создания образа файловой системы
spiffs_create_partition_image(storage ${SPIFFS_IMAGE_DIR} FLASH_IN_PROJECT)