#pragma once
//******************************************************************************
// Секция include
//******************************************************************************
#include <stdbool.h>
#include <stdint.h>

#include "flow_direction.h"
//******************************************************************************
// Секция определения констант
//******************************************************************************

//******************************************************************************
// Секция определения типов
//******************************************************************************
typedef enum
{
    DC_MODE_MANUAL_CONTROL,       // Ручное управление
    DC_MODE_WASH_DISTILLATION,    // Авто: перегон браги → спирт сырец (first distillation / wash run)
    DC_MODE_SPIRIT_RECTIFICATION, // Авто: перегон спиртсырца → очищенный спирт (rectification / spirit run)
    DC_MODE_AUTOCLAVE             // Авто: автоклавный режим
} dc_mode_e;

typedef enum
{
    DC_COMMAND_REQUEST_INIT_COLUMN_ROM,
    DC_COMMAND_REQUEST_INIT_KUBE_ROM,
    DC_COMMAND_SET_MODE,
    DC_COMMAND_SET_TEN_POWER,
    DC_COMMAND_SET_CUBE_TEMPERATURE,
    DC_COMMAND_SET_FLOW_DIRECTION,
} dc_command_e;

typedef enum
{
    DC_PARAM_COLUMN_ROM,
    DC_PARAM_KUBE_ROM,
    DC_PARAM_FLOW_DIRECTION_ANGLE_1,
    DC_PARAM_FLOW_DIRECTION_ANGLE_2,
    DC_PARAM_FLOW_DIRECTION_ANGLE_3,
    DC_PARAM_AUTOCLAVE_MODE_PID_PARAMETERS,
} dc_parametes_e;

typedef struct
{
    dc_mode_e mode;
    position_flow_direction_e flow_direction;
    float temperature_column;
    float temperature_kube;
    float temperature_radiator;
    float ten_power;
    float voltage_220V;
    uint8_t fan;
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