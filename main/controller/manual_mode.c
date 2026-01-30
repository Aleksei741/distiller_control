//******************************************************************************
//include
//******************************************************************************
#include "manual_mode.h"
#include <ten_ctrl.h>
#include <modbus_master.h>
#include <flow_direction.h>

#include "esp_log.h"
//******************************************************************************
// Constants
//******************************************************************************
#define DEFAULT_VALUE 0xff
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
static char *TAG = "[manual_mode]";

static uint8_t heater_power = 0;
static position_flow_direction_e flow_direction_position = 0;
//------------------------------------------------------------------------------
// Local Class
//------------------------------------------------------------------------------

//******************************************************************************
// Function prototype
//******************************************************************************

//******************************************************************************
// Function
//******************************************************************************
void manual_mode_set_heater_power(uint8_t p)
{
    heater_power = p;
    ESP_LOGI(TAG, "Set heater power to: %d%%", p);
}
//------------------------------------------------------------------------------
void manual_mode_set_flow_direction(uint8_t p)
{
    if(p == 0)
        flow_direction_position = POSITION_FLOW_DIRECTION_ANGLE_1;
    else if(p == 1)
        flow_direction_position = POSITION_FLOW_DIRECTION_ANGLE_2;
    else if(p == 2)
        flow_direction_position = POSITION_FLOW_DIRECTION_ANGLE_3;

    ESP_LOGI(TAG, "Set flow direction to position: %d", p);
}
//------------------------------------------------------------------------------
void manual_mode_process(dc_status_t* status)
{
    if(status == NULL)
        return;

    if(status->mode != DC_MODE_MANUAL_CONTROL)
    {
        heater_power = 0;
        return;
    }

    status->ten_power = heater_power;
    status->flow_direction = flow_direction_position;

    flow_direction_set(flow_direction_position);
    heater_set_power(heater_power);
}