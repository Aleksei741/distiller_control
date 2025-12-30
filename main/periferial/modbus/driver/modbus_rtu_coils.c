//******************************************************************************
//include
//******************************************************************************
#include "modbus_rtu_coils.h"
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
size_t modbus_rtu_build_read_coils(uint8_t slave_id, uint16_t start_addr, uint16_t count, uint8_t *frame, size_t frame_size)
{
    if (!frame || frame_size < 8) return 0;
    if (count == 0 || count > MODBUS_MAX_COILS_READ) return 0;

    frame[0] = slave_id;
    frame[1] = 0x01;
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
modbus_err_t modbus_rtu_parse_read_coils(const uint8_t *frame, size_t frame_len, uint8_t slave_id, uint16_t count, uint8_t *coils)
{
    if (!frame || !coils) return MODBUS_ERR_NULL_PTR;
    if (frame_len < 5) return MODBUS_ERR_FRAME_TOO_SHORT;
    if (frame[0] != slave_id) return MODBUS_ERR_SLAVE_ID_MISMATCH;
    if (frame[1] != 0x01) return MODBUS_ERR_FUNCTION_MISMATCH;
    if (frame_len < 2) return MODBUS_ERR_FRAME_TOO_SHORT;

    uint16_t crc_received = (frame[frame_len - 1] << 8) | frame[frame_len - 2];
    uint16_t crc_calc = modbus_crc16(frame, frame_len - 2);
    if (crc_received != crc_calc) return MODBUS_ERR_CRC;

    uint8_t byte_count = frame[2];
    if (byte_count == 0 || (byte_count * 8 < count)) return MODBUS_ERR_BYTE_COUNT;
    if (frame_len != (size_t)(3 + byte_count + 2)) return MODBUS_ERR_FRAME_TOO_SHORT;

    memcpy(coils, &frame[3], byte_count);
    return MODBUS_ERR_OK;
}
//------------------------------------------------------------------------------
size_t modbus_rtu_build_write_single_coil(uint8_t slave_id, uint16_t addr, bool value, uint8_t *frame, size_t frame_size)
{
    if (!frame || frame_size < 8) return 0;
    frame[0] = slave_id;
    frame[1] = 0x05;
    frame[2] = (addr >> 8) & 0xFF;
    frame[3] = addr & 0xFF;
    frame[4] = value ? 0xFF : 0x00;
    frame[5] = 0x00;
    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = (crc >> 8) & 0xFF;
    return 8;
}
//------------------------------------------------------------------------------
modbus_err_t modbus_rtu_parse_write_single_coil(const uint8_t *frame, size_t frame_len, uint8_t slave_id, uint16_t addr, bool value)
{
    if (!frame) return MODBUS_ERR_NULL_PTR;
    if (frame_len != 8) return MODBUS_ERR_FRAME_TOO_SHORT;
    if (frame[0] != slave_id) return MODBUS_ERR_SLAVE_ID_MISMATCH;
    if (frame[1] != 0x05) return MODBUS_ERR_FUNCTION_MISMATCH;

    uint16_t crc_received = (frame[7] << 8) | frame[6];
    uint16_t crc_calc = modbus_crc16(frame, 6);
    if (crc_received != crc_calc) return MODBUS_ERR_CRC;

    uint16_t resp_addr = (frame[2] << 8) | frame[3];
    uint8_t resp_val = frame[4];
    if (resp_addr != addr) return MODBUS_ERR_ADDR_MISMATCH;
    if (resp_val != 0x00 && resp_val != 0xFF) return MODBUS_ERR_COIL_VALUE;
    if ((value && resp_val != 0xFF) || (!value && resp_val != 0x00)) return MODBUS_ERR_COIL_VALUE;
    return MODBUS_ERR_OK;
}
//------------------------------------------------------------------------------
size_t modbus_rtu_build_write_multiple_coils(uint8_t slave_id, uint16_t start_addr, uint16_t count, const uint8_t *coils, uint8_t *frame, size_t frame_size)
{
    if (!frame || !coils || frame_size < 9) return 0;
    if (count == 0 || count > MODBUS_MAX_COILS_WRITE) return 0;
    uint8_t byte_count = (count + 7) / 8;
    if (frame_size < (size_t)(9 + byte_count - 1)) return 0;

    frame[0] = slave_id;
    frame[1] = 0x0F;
    frame[2] = (start_addr >> 8) & 0xFF;
    frame[3] = start_addr & 0xFF;
    frame[4] = (count >> 8) & 0xFF;
    frame[5] = count & 0xFF;
    frame[6] = byte_count;
    memcpy(&frame[7], coils, byte_count);

    uint16_t crc = modbus_crc16(frame, 7 + byte_count);
    frame[7 + byte_count] = crc & 0xFF;
    frame[8 + byte_count] = (crc >> 8) & 0xFF;
    return 9 + byte_count;
}
//------------------------------------------------------------------------------
modbus_err_t modbus_rtu_parse_write_multiple_coils(const uint8_t *frame, size_t frame_len, uint8_t slave_id, uint16_t start_addr, uint16_t count)
{
    if (!frame) return MODBUS_ERR_NULL_PTR;
    if (frame_len != 12) return MODBUS_ERR_FRAME_TOO_SHORT;
    if (frame[0] != slave_id) return MODBUS_ERR_SLAVE_ID_MISMATCH;
    if (frame[1] != 0x0F) return MODBUS_ERR_FUNCTION_MISMATCH;

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