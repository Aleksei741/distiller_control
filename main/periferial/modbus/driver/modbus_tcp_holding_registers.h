#pragma once
//******************************************************************************
// Секция include
//******************************************************************************
#include "modbus_common.h"

#ifdef __cplusplus
extern "C" {
#endif
//******************************************************************************
// Секция определения констант
//******************************************************************************

//******************************************************************************
// Секция определения типов
//******************************************************************************

//******************************************************************************
// Секция определения глобальных переменных
//******************************************************************************

//******************************************************************************
// Секция прототипов глобальных функций
//******************************************************************************
// Чтение coils (0x01)
size_t modbus_tcp_build_read_holding_registers(uint16_t transaction_id, uint8_t unit_id, uint16_t start_addr, uint16_t count, uint8_t *frame, size_t frame_size);
modbus_err_t modbus_tcp_parse_read_holding_registers(const uint8_t *frame, size_t frame_len, uint16_t transaction_id, uint8_t unit_id, uint16_t count, modbus_reg_t *registers);

// Запись одного coil (0x05)
size_t modbus_tcp_build_write_single_register(uint16_t transaction_id, uint8_t unit_id, uint16_t addr, modbus_reg_t value, uint8_t *frame, size_t frame_size);
modbus_err_t modbus_tcp_parse_write_single_register(const uint8_t *frame, size_t frame_len, uint16_t transaction_id, uint8_t unit_id, uint16_t addr, modbus_reg_t value);

// Запись нескольких coils (0x0F)
size_t modbus_tcp_build_write_multiple_registers(uint16_t transaction_id, uint8_t unit_id, uint16_t start_addr, uint16_t count, const modbus_reg_t *registers, uint8_t *frame, size_t frame_size);
modbus_err_t modbus_tcp_parse_write_multiple_registers(const uint8_t *frame, size_t frame_len, uint16_t transaction_id, uint8_t unit_id, uint16_t start_addr, uint16_t count);
//******************************************************************************
// Секция определения макросов
//******************************************************************************

#ifdef __cplusplus
}
#endif
//******************************************************************************
// ENF OF FILE
//******************************************************************************