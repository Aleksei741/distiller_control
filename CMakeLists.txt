cmake_minimum_required(VERSION 3.22)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(distiller_control)

# =========================================================
# Добавляем компонент spiffs

idf_component_set_property(spiffs PRIV_REQUIRES "spi_flash")

# Указываем команду для создания образа файловой системы
idf_component_add_build_dependency(spiffs)

# Добавляем правило для создания образа (filesystem image)
# file_path - путь к папке с вашими файлами (spiffs)
# partition_name - имя раздела из partitions.csv (storage)
# target_name - имя, под которым образ будет известен в idf.py (storage_image)
idf_create_partition_image(
    file_path "spiffs"
    partition_name "storage"
    target_name "storage_image"
)

# Убеждаемся, что образ собирается перед прошивкой
idf_build_set_dependency(storage_image storage)