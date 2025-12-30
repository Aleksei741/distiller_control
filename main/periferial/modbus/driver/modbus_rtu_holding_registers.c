//******************************************************************************
//include
//******************************************************************************
#include "modbus_rtu_holding_registers.h"
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
static void registers_to_bytes(const modbus_reg_t *regs, uint16_t count, uint8_t *bytes);
static void bytes_to_registers(const uint8_t *bytes, uint16_t count, modbus_reg_t *regs);
//******************************************************************************
// Function
//******************************************************************************
size_t modbus_rtu_build_read_holding_registers(uint8_t slave_id, uint16_t start_addr, uint16_t count, uint8_t *frame, size_t frame_size)
{
    if (!frame || frame_size < 8) return 0;
    if (count == 0 || count > MODBUS_MAX_REGISTERS_READ) return 0;

    frame[0] = slave_id;
    frame[1] = 0x03;
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
modbus_err_t modbus_rtu_parse_read_holding_registers(const uint8_t *frame, size_t frame_len, uint8_t slave_id, uint16_t count, modbus_reg_t *registers)
{
    if (!frame || !registers) return MODBUS_ERR_NULL_PTR;
    if (frame_len < 5) return MODBUS_ERR_FRAME_TOO_SHORT;
    if (frame[0] != slave_id) return MODBUS_ERR_SLAVE_ID_MISMATCH;
    if (frame[1] != 0x03) return MODBUS_ERR_FUNCTION_MISMATCH;
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
size_t modbus_rtu_build_write_single_register(uint8_t slave_id, uint16_t addr, modbus_reg_t value, uint8_t *frame, size_t frame_size)
{
    if (!frame || frame_size < 8) return 0;
    frame[0] = slave_id;
    frame[1] = 0x06;
    frame[2] = (addr >> 8) & 0xFF;
    frame[3] = addr & 0xFF;
    frame[4] = (value >> 8) & 0xFF;
    frame[5] = value & 0xFF;
    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = (crc >> 8) & 0xFF;
    return 8;
}
//------------------------------------------------------------------------------
modbus_err_t modbus_rtu_parse_write_single_register(const uint8_t *frame, size_t frame_len, uint8_t slave_id, uint16_t addr, modbus_reg_t value)
{
    if (!frame) return MODBUS_ERR_NULL_PTR;
    if (frame_len != 8) return MODBUS_ERR_FRAME_TOO_SHORT;
    if (frame[0] != slave_id) return MODBUS_ERR_SLAVE_ID_MISMATCH;
    if (frame[1] != 0x06) return MODBUS_ERR_FUNCTION_MISMATCH;

    uint16_t crc_received = (frame[7] << 8) | frame[6];
    uint16_t crc_calc = modbus_crc16(frame, 6);
    if (crc_received != crc_calc) return MODBUS_ERR_CRC;

    uint16_t resp_addr = (frame[2] << 8) | frame[3];
    modbus_reg_t resp_val = (frame[4] << 8) | frame[5];
    if (resp_addr != addr) return MODBUS_ERR_ADDR_MISMATCH;
    if (resp_val != value) return MODBUS_ERR_COUNT_MISMATCH; // reuse for value mismatch
    return MODBUS_ERR_OK;
}
//------------------------------------------------------------------------------
size_t modbus_rtu_build_write_multiple_registers(uint8_t slave_id, uint16_t start_addr, uint16_t count, const modbus_reg_t *registers, uint8_t *frame, size_t frame_size)
{
    if (!frame || !registers || frame_size < 9) return 0;
    if (count == 0 || count > MODBUS_MAX_REGISTERS_WRITE) return 0;
    uint8_t byte_count = count * 2;
    if (frame_size < (size_t)(9 + byte_count - 1)) return 0;

    frame[0] = slave_id;
    frame[1] = 0x10;
    frame[2] = (start_addr >> 8) & 0xFF;
    frame[3] = start_addr & 0xFF;
    frame[4] = (count >> 8) & 0xFF;
    frame[5] = count & 0xFF;
    frame[6] = byte_count;
    registers_to_bytes(registers, count, &frame[7]);

    uint16_t crc = modbus_crc16(frame, 7 + byte_count);
    frame[7 + byte_count] = crc & 0xFF;
    frame[8 + byte_count] = (crc >> 8) & 0xFF;
    return 9 + byte_count;
}
//------------------------------------------------------------------------------
modbus_err_t modbus_rtu_parse_write_multiple_registers(const uint8_t *frame, size_t frame_len, uint8_t slave_id, uint16_t start_addr, uint16_t count)
{
    if (!frame) return MODBUS_ERR_NULL_PTR;
    if (frame_len != 12) return MODBUS_ERR_FRAME_TOO_SHORT;
    if (frame[0] != slave_id) return MODBUS_ERR_SLAVE_ID_MISMATCH;
    if (frame[1] != 0x10) return MODBUS_ERR_FUNCTION_MISMATCH;

    uint16_t crc_received = (frame[11] << 8) | frame[10];
    uint16_t crc_calc = modbus_crc16(frame, 10);
    if (crc_received != crc_calc) return MODBUS_ERR_CRC;

    uint16_t resp_addr = (frame[2] << 8) | frame[3];
    uint16_t resp_count = (frame[4] << 8) | frame[5];
    if (resp_addr != start_addr) return MODBUS_ERR_ADDR_MISMATCH;
    if (resp_count != count) return MODBUS_ERR_COUNT_MISMATCH;
    return MODBUS_ERR_OK;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Вспомогательная функция: копирует 16-битные регистры в байтовый буфер (big-endian)
static void registers_to_bytes(const modbus_reg_t *regs, uint16_t count, uint8_t *bytes) 
{
    for (uint16_t i = 0; i < count; i++)
     {
        bytes[i * 2] = (regs[i] >> 8) & 0xFF;
        bytes[i * 2 + 1] = regs[i] & 0xFF;
    }
}
//------------------------------------------------------------------------------
// Вспомогательная функция: копирует байты в 16-битные регистры (big-endian)
static void bytes_to_registers(const uint8_t *bytes, uint16_t count, modbus_reg_t *regs) 
{
    for (uint16_t i = 0; i < count; i++) 
    {
        regs[i] = (bytes[i * 2] << 8) | bytes[i * 2 + 1];
    }
}
//------------------------------------------------------------------------------
