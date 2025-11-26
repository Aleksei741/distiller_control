@echo off
REM ----------------------------
REM install_idf.bat
REM Параметры:
REM   %1 - IDF_PATH (C:\esp\esp-idf)
REM   %2 - IDF_TOOLS_PATH (C:\esp\tools)
REM ----------------------------

if "%~1"=="" (
    echo Error: IDF_PATH parameter is missing
    exit /b 1
)

if "%~2"=="" (
    echo Error: IDF_TOOLS_PATH parameter is missing
    exit /b 1
)

set "IDF_PATH=%~1"
set "IDF_TOOLS_PATH=%~2"

REM Получаем родительскую директорию IDF_PATH
for %%I in ("%IDF_PATH%") do set "IDF_PARENT=%%~dpI"

echo IDF_PATH = %IDF_PATH%
echo IDF_TOOLS_PATH = %IDF_TOOLS_PATH%
echo IDF_PARENT = %IDF_PARENT%

REM Создаем родительскую папку если не существует
if not exist "%IDF_PARENT%" (
    echo Creating parent directory "%IDF_PARENT%"
    mkdir "%IDF_PARENT%"
)

REM Удаляем старые папки, если есть
if exist "%IDF_PATH%" (
    echo Removing old esp-idf folder "%IDF_PATH%"
    rmdir /S /Q "%IDF_PATH%"
)

if exist "%IDF_TOOLS_PATH%" (
    echo Removing old tools folder "%IDF_TOOLS_PATH%"
    rmdir /S /Q "%IDF_TOOLS_PATH%"
)

REM Клонируем ESP-IDF
echo Cloning ESP-IDF into "%IDF_PATH%"
git clone --recursive https://github.com/espressif/esp-idf.git "%IDF_PATH%"

REM Переходим в папку esp-idf
cd /d "%IDF_PATH%"

REM Переключаемся на релизную ветку
echo Switching to release/v6.0
git fetch --all
git switch release/v6.0

REM Вызываем install.bat
echo Running install.bat
call install.bat

echo ESP-IDF installation completed!
