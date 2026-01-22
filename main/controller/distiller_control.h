#pragma once
//******************************************************************************
// Секция include
//******************************************************************************
#include <stdbool.h>
#include <stdint.h>
//******************************************************************************
// Секция определения констант
//******************************************************************************

//******************************************************************************
// Секция определения типов
//******************************************************************************
typedef enum
{
    DC_MANUAL_CONTROL,       // Ручное управление
    DC_WASH_DISTILLATION,    // Авто: перегон браги → спирт сырец (first distillation / wash run)
    DC_SPIRIT_RECTIFICATION  // Авто: перегон спиртсырца → очищенный спирт (rectification / spirit run)
} dc_mode_e;

typedef enum
{
    DC_REQUEST_INIT_COLUMN_ROM,
    DC_REQUEST_INIT_KUBE_ROM,
    DC_SET_MODE,
    DC_SET_TEN_POWER,
} dc_command_e;

typedef enum
{
    DC_COLUMN_ROM,
    DC_KUBE_ROM,
} dc_parametes_e;

typedef struct
{
    dc_mode_e mode;
    float temperature_column;
    float temperature_kube;
    float temperature_radiator;
    float ten_power;
    float voltage_220V;
} dc_status_t;

typedef struct 
{
    dc_command_e command;
    union
    {
        uint8_t data[4];
        float valuef;
        uint32_t valuei;
    };
} dc_input_command_t;

typedef struct 
{    

} dc_params_t;
//******************************************************************************
// Секция определения глобальных переменных
//******************************************************************************

//******************************************************************************
// Секция прототипов глобальных функций
//******************************************************************************
void init_distiller_control();

bool get_dc_parameters(dc_parametes_e param, void *out_params);
bool set_dc_parameters(dc_parametes_e param, void *in_params);
bool get_dc_status(dc_status_t *out_status); 
void send_dc_command(dc_command_e command, void* value);
//******************************************************************************
// Секция определения макросов
//******************************************************************************

//******************************************************************************
// ENF OF FILE
//******************************************************************************