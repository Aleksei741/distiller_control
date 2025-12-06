@echo off
REM ----------------------------
REM add_dependency.bat
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

REM Устанавливаем переменные среды для ESP-IDF
set "IDF_PATH=%~1"
set "IDF_TOOLS_PATH=%~2"

echo Using IDF_PATH=%IDF_PATH%
echo Using IDF_TOOLS_PATH=%IDF_TOOLS_PATH%

REM Активируем окружение ESP-IDF
call "%IDF_PATH%\export.bat"

REM Добавляем зависимости
idf.py add-dependency "espressif/mdns=*"

echo Dependencies installed successfully.
pause