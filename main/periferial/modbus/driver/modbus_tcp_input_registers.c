//******************************************************************************
//include
//******************************************************************************
#include "modbus_tcp_input_registers.h"
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

//******************************************************************************
// Function
//******************************************************************************
static void bytes_to_registers(const uint8_t *bytes, uint16_t count, modbus_reg_t *regs) 
{
    for (uint16_t i = 0; i < count; i++) 
    {
        regs[i] = (bytes[i * 2] << 8) | bytes[i * 2 + 1];
    }
}
//------------------------------------------------------------------------------
size_t modbus_tcp_build_read_input_registers( uint16_t transaction_id, uint8_t unit_id, uint16_t start_addr, uint16_t count, uint8_t *frame, size_t frame_size)
{
    if (!frame || frame_size < 12) return 0;
    if (count == 0 || count > MODBUS_MAX_REGISTERS_READ) return 0;

    frame[0] = (transaction_id >> 8) & 0xFF;
    frame[1] = transaction_id & 0xFF;
    frame[2] = 0x00; // Protocol ID
    frame[3] = 0x00;
    frame[4] = 0x00; // Length high
    frame[5] = 0x06; // Length low
    frame[6] = unit_id;
    frame[7] = 0x04; // Read Input Registers
    frame[8] = (start_addr >> 8) & 0xFF;
    frame[9] = start_addr & 0xFF;
    frame[10] = (count >> 8) & 0xFF;
    frame[11] = count & 0xFF;
    return 12;
}
//------------------------------------------------------------------------------
modbus_err_t modbus_tcp_parse_read_input_registers( const uint8_t *frame, size_t frame_len, uint16_t transaction_id, uint8_t unit_id, uint16_t count, modbus_reg_t *registers)
{
    if (!frame || !registers) return MODBUS_ERR_NULL_PTR;
    if (frame_len < 9) return MODBUS_ERR_FRAME_TOO_SHORT;

    uint16_t recv_tid = (frame[0] << 8) | frame[1];
    if (recv_tid != transaction_id) return MODBUS_ERR_TRANSACTION_MISMATCH;
    if (frame[2] != 0x00 || frame[3] != 0x00) return MODBUS_ERR_PROTOCOL_ID;
    if (frame[6] != unit_id) return MODBUS_ERR_SLAVE_ID_MISMATCH;
    if (frame[7] != 0x04) return MODBUS_ERR_FUNCTION_MISMATCH;

    uint8_t byte_count = frame[8];
    if (byte_count != count * 2) return MODBUS_ERR_BYTE_COUNT;
    if (frame_len != (size_t)(9 + byte_count)) return MODBUS_ERR_FRAME_TOO_SHORT;

    bytes_to_registers(&frame[9], count, registers);
    return MODBUS_ERR_OK;
}
//------------------------------------------------------------------------------
