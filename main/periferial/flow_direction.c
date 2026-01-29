//******************************************************************************
//include
//******************************************************************************
#include "led.h"
#include "esp_log.h"          // ESP_LOGI

#include "flow_direction.h"
#include "parameters.h"
#include <modbus_master.h>
//******************************************************************************
// Constants
//******************************************************************************

//******************************************************************************
// Type
//******************************************************************************

//******************************************************************************
// Variables
//******************************************************************************
//------------------------------------------------------------------------------
// Global
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local Variable
//------------------------------------------------------------------------------
uint16_t angle_flow[3] = {0, 0, 0};

static const char *TAG = "[flow_direction]";
//------------------------------------------------------------------------------
// Local Class
//------------------------------------------------------------------------------

//******************************************************************************
// Function prototype
//******************************************************************************

//******************************************************************************
// Function
//******************************************************************************
void flow_direction_init(void)
{
    load_external_flow_direction(PARAMETERS_POSITION_FLOW_DIRECTION_ANGLE_1, &angle_flow[0]);    
    load_external_flow_direction(PARAMETERS_POSITION_FLOW_DIRECTION_ANGLE_2, &angle_flow[1]);
    load_external_flow_direction(PARAMETERS_POSITION_FLOW_DIRECTION_ANGLE_3, &angle_flow[2]);

    ESP_LOGI(TAG, "Initialized angles: %d, %d, %d", angle_flow[0], angle_flow[1], angle_flow[2]);
}
//------------------------------------------------------------------------------
void flow_direction_set(position_flow_direction_e position)
{
    modbus_master_set_value(MODBUS_FLOW_DIRECTION_ANGLE, angle_flow[position]);
    ESP_LOGI(TAG, "Set flow direction angle position %d to %d", position, angle_flow[position]);
}
//------------------------------------------------------------------------------
void flow_direction_set_parameters_angle(position_flow_direction_e position, uint16_t angle)
{
    angle_flow[position] = angle;

    parameters_position_flow_direction_e position_setings;
    if(position == POSITION_FLOW_DIRECTION_ANGLE_1)
        position_setings = PARAMETERS_POSITION_FLOW_DIRECTION_ANGLE_1;
    else if(position == POSITION_FLOW_DIRECTION_ANGLE_2)
        position_setings = PARAMETERS_POSITION_FLOW_DIRECTION_ANGLE_2;  
    else //if(position == POSITION_FLOW_DIRECTION_ANGLE_3)
        position_setings = PARAMETERS_POSITION_FLOW_DIRECTION_ANGLE_3;
        
    save_external_flow_direction(position_setings, angle);

    ESP_LOGI(TAG, "Set parameters flow direction angle position %d to %d", position, angle);
}
//------------------------------------------------------------------------------
uint16_t flow_direction_get_parameters_angle(position_flow_direction_e position)
{
    ESP_LOGI(TAG, "Get parameters flow direction angle position %d: %d", position, angle_flow[position]);
    return angle_flow[position];
}
//------------------------------------------------------------------------------
