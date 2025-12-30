//******************************************************************************
//include
//******************************************************************************
#include "modbus_rtu_discrete_input.h"
#include "modbus_crc.h"
#include <string.h>
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

//------------------------------------------------------------------------------
// Local Class
//------------------------------------------------------------------------------

//******************************************************************************
// Function prototype
//******************************************************************************

//******************************************************************************
// Function
//******************************************************************************
size_t modbus_rtu_build_discrete_inputs(uint8_t slave_id, uint16_t start_addr, uint16_t count, uint8_t *frame, size_t frame_size)
{
    if (!frame || frame_size < 8) return 0;
    if (count == 0 || count > MODBUS_MAX_COILS_READ) return 0; // reuse constant

    frame[0] = slave_id;
    frame[1] = 0x02; // Read Discrete Inputs
    frame[2] = (start_addr >> 8) & 0xFF;
    frame[3] = start_addr & 0xFF;
    frame[4] = (count >> 8) & 0xFF;
    frame[5] = count & 0xFF;

    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = (crc >> 8) & 0xFF;
    return 8;
}
//------------------------------------------------------------------------------
modbus_err_t modbus_rtu_parse_discrete_inputs(const uint8_t *frame, size_t frame_len, uint8_t slave_id, uint16_t count, uint8_t *inputs)
{
    if (!frame || !inputs) return MODBUS_ERR_NULL_PTR;
    if (frame_len < 5) return MODBUS_ERR_FRAME_TOO_SHORT;
    if (frame[0] != slave_id) return MODBUS_ERR_SLAVE_ID_MISMATCH;
    if (frame[1] != 0x02) return MODBUS_ERR_FUNCTION_MISMATCH;
    if (frame_len < 2) return MODBUS_ERR_FRAME_TOO_SHORT;

    uint16_t crc_received = (frame[frame_len - 1] << 8) | frame[frame_len - 2];
    uint16_t crc_calc = modbus_crc16(frame, frame_len - 2);
    if (crc_received != crc_calc) return MODBUS_ERR_CRC;

    uint8_t byte_count = frame[2];
    if (byte_count == 0 || (byte_count * 8 < count)) return MODBUS_ERR_BYTE_COUNT;
    if (frame_len != (size_t)(3 + byte_count + 2)) return MODBUS_ERR_FRAME_TOO_SHORT;

    memcpy(inputs, &frame[3], byte_count);
    return MODBUS_ERR_OK;
}
//------------------------------------------------------------------------------
