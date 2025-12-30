//******************************************************************************
//include
//******************************************************************************
#include "modbus_rtu_input_registers.h"
#include "modbus_crc.h"
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
static void bytes_to_registers(const uint8_t *bytes, uint16_t count, modbus_reg_t *regs);
//******************************************************************************
// Function
//******************************************************************************
size_t modbus_rtu_build_read_input_registers(uint8_t slave_id, uint16_t start_addr, uint16_t count, uint8_t *frame, size_t frame_size)
{
    if (!frame || frame_size < 8) return 0;
    if (count == 0 || count > MODBUS_MAX_REGISTERS_READ) return 0;

    frame[0] = slave_id;
    frame[1] = 0x04; // Read Input Registers
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
modbus_err_t modbus_rtu_parse_read_input_registers(const uint8_t *frame, size_t frame_len, uint8_t slave_id, uint16_t count, modbus_reg_t *registers)
{
    if (!frame || !registers) return MODBUS_ERR_NULL_PTR;
    if (frame_len < 5) return MODBUS_ERR_FRAME_TOO_SHORT;
    if (frame[0] != slave_id) return MODBUS_ERR_SLAVE_ID_MISMATCH;
    if (frame[1] != 0x04) return MODBUS_ERR_FUNCTION_MISMATCH;
    if (frame_len < 2) return MODBUS_ERR_FRAME_TOO_SHORT;

    uint16_t crc_received = (frame[frame_len - 1] << 8) | frame[frame_len - 2];
    uint16_t crc_calc = modbus_crc16(frame, frame_len - 2);
    if (crc_received != crc_calc) return MODBUS_ERR_CRC;

    uint8_t byte_count = frame[2];
    if (byte_count != count * 2) return MODBUS_ERR_BYTE_COUNT;
    if (frame_len != (size_t)(3 + byte_count + 2)) return MODBUS_ERR_FRAME_TOO_SHORT;

    bytes_to_registers(&frame[3], count, registers);
    return MODBUS_ERR_OK;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void bytes_to_registers(const uint8_t *bytes, uint16_t count, modbus_reg_t *regs)
{
    for (uint16_t i = 0; i < count; i++)
    {
        regs[i] = (bytes[i * 2] << 8) | bytes[i * 2 + 1];
    }
}
//------------------------------------------------------------------------------