//******************************************************************************
//include
//******************************************************************************
#include "modbus_tcp_coils.h"
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
size_t modbus_tcp_build_read_coils(uint16_t transaction_id, uint8_t unit_id, uint16_t start_addr, uint16_t count, uint8_t *frame, size_t frame_size)
{
    if (!frame || frame_size < 12) return 0;
    if (count == 0 || count > MODBUS_MAX_COILS_READ) return 0;

    frame[0] = (transaction_id >> 8) & 0xFF;
    frame[1] = transaction_id & 0xFF;
    frame[2] = 0x00; // Protocol ID high
    frame[3] = 0x00; // Protocol ID low
    frame[4] = 0x00; // Length high
    frame[5] = 0x06; // Length low
    frame[6] = unit_id;
    frame[7] = 0x01;
    frame[8] = (start_addr >> 8) & 0xFF;
    frame[9] = start_addr & 0xFF;
    frame[10] = (count >> 8) & 0xFF;
    frame[11] = count & 0xFF;
    return 12;
}
//------------------------------------------------------------------------------
modbus_err_t modbus_tcp_parse_read_coils(const uint8_t *frame, size_t frame_len, uint16_t transaction_id, uint8_t unit_id, uint16_t count, uint8_t *coils)
{
    if (!frame || !coils) return MODBUS_ERR_NULL_PTR;
    if (frame_len < 9) return MODBUS_ERR_FRAME_TOO_SHORT;

    uint16_t recv_tid = (frame[0] << 8) | frame[1];
    if (recv_tid != transaction_id) return MODBUS_ERR_TRANSACTION_MISMATCH;
    if (frame[2] != 0x00 || frame[3] != 0x00) return MODBUS_ERR_PROTOCOL_ID;
    if (frame[6] != unit_id) return MODBUS_ERR_SLAVE_ID_MISMATCH;
    if (frame[7] != 0x01) return MODBUS_ERR_FUNCTION_MISMATCH;

    uint8_t byte_count = frame[8];
    if (byte_count == 0 || (byte_count * 8 < count)) return MODBUS_ERR_BYTE_COUNT;
    if (frame_len != (size_t)(9 + byte_count)) return MODBUS_ERR_FRAME_TOO_SHORT;

    memcpy(coils, &frame[9], byte_count);
    return MODBUS_ERR_OK;
}
//------------------------------------------------------------------------------
size_t modbus_tcp_build_write_single_coil(uint16_t transaction_id, uint8_t unit_id, uint16_t addr, bool value, uint8_t *frame, size_t frame_size)
{
    if (!frame || frame_size < 12) return 0;
    frame[0] = (transaction_id >> 8) & 0xFF;
    frame[1] = transaction_id & 0xFF;
    frame[2] = 0x00;
    frame[3] = 0x00;
    frame[4] = 0x00;
    frame[5] = 0x06;
    frame[6] = unit_id;
    frame[7] = 0x05;
    frame[8] = (addr >> 8) & 0xFF;
    frame[9] = addr & 0xFF;
    frame[10] = value ? 0xFF : 0x00;
    frame[11] = 0x00;
    return 12;
}
//------------------------------------------------------------------------------
modbus_err_t modbus_tcp_parse_write_single_coil( const uint8_t *frame, size_t frame_len, uint16_t transaction_id, uint8_t unit_id, uint16_t addr, bool value)
{
    if (!frame) return MODBUS_ERR_NULL_PTR;
    if (frame_len != 12) return MODBUS_ERR_FRAME_TOO_SHORT;

    uint16_t recv_tid = (frame[0] << 8) | frame[1];
    if (recv_tid != transaction_id) return MODBUS_ERR_TRANSACTION_MISMATCH;
    if (frame[6] != unit_id) return MODBUS_ERR_SLAVE_ID_MISMATCH;
    if (frame[7] != 0x05) return MODBUS_ERR_FUNCTION_MISMATCH;

    uint16_t resp_addr = (frame[8] << 8) | frame[9];
    uint8_t resp_val = frame[10];
    if (resp_addr != addr) return MODBUS_ERR_ADDR_MISMATCH;
    if (resp_val != 0x00 && resp_val != 0xFF) return MODBUS_ERR_COIL_VALUE;
    if ((value && resp_val != 0xFF) || (!value && resp_val != 0x00)) return MODBUS_ERR_COIL_VALUE;
    return MODBUS_ERR_OK;
}
//------------------------------------------------------------------------------
size_t modbus_tcp_build_write_multiple_coils(uint16_t transaction_id, uint8_t unit_id, uint16_t start_addr, uint16_t count, const uint8_t *coils, uint8_t *frame, size_t frame_size)
{
    if (!frame || !coils || frame_size < 13) return 0;
    if (count == 0 || count > MODBUS_MAX_COILS_WRITE) return 0;
    uint8_t byte_count = (count + 7) / 8;
    if (frame_size < (size_t)(13 + byte_count - 1)) return 0;

    frame[0] = (transaction_id >> 8) & 0xFF;
    frame[1] = transaction_id & 0xFF;
    frame[2] = 0x00;
    frame[3] = 0x00;
    frame[4] = 0x00;
    frame[5] = 7 + byte_count;
    frame[6] = unit_id;
    frame[7] = 0x0F;
    frame[8] = (start_addr >> 8) & 0xFF;
    frame[9] = start_addr & 0xFF;
    frame[10] = (count >> 8) & 0xFF;
    frame[11] = count & 0xFF;
    frame[12] = byte_count;
    memcpy(&frame[13], coils, byte_count);
    return 13 + byte_count;
}
//------------------------------------------------------------------------------
modbus_err_t modbus_tcp_parse_write_multiple_coils(const uint8_t *frame, size_t frame_len, uint16_t transaction_id, uint8_t unit_id, uint16_t start_addr, uint16_t count)
{
    if (!frame) return MODBUS_ERR_NULL_PTR;
    if (frame_len < 16) return MODBUS_ERR_FRAME_TOO_SHORT;

    uint16_t recv_tid = (frame[0] << 8) | frame[1];
    if (recv_tid != transaction_id) return MODBUS_ERR_TRANSACTION_MISMATCH;
    if (frame[6] != unit_id) return MODBUS_ERR_SLAVE_ID_MISMATCH;
    if (frame[7] != 0x0F) return MODBUS_ERR_FUNCTION_MISMATCH;

    uint16_t resp_addr = (frame[8] << 8) | frame[9];
    uint16_t resp_count = (frame[10] << 8) | frame[11];
    if (resp_addr != start_addr) return MODBUS_ERR_ADDR_MISMATCH;
    if (resp_count != count) return MODBUS_ERR_COUNT_MISMATCH;
    return MODBUS_ERR_OK;
}
//------------------------------------------------------------------------------