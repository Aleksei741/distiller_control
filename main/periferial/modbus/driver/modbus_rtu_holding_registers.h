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
// Чтение holding registers (0x03)
size_t modbus_rtu_build_read_holding_registers(uint8_t slave_id, uint16_t start_addr, uint16_t count, uint8_t *frame, size_t frame_size);
modbus_err_t modbus_rtu_parse_read_holding_registers(const uint8_t *frame, size_t frame_len, uint8_t slave_id, uint16_t count, modbus_reg_t *registers);

// Запись одного регистра (0x06)
size_t modbus_rtu_build_write_single_register(uint8_t slave_id, uint16_t addr, modbus_reg_t value, uint8_t *frame, size_t frame_size);
modbus_err_t modbus_rtu_parse_write_single_register(const uint8_t *frame, size_t frame_len, uint8_t slave_id, uint16_t addr, modbus_reg_t value);

// Запись нескольких регистров (0x10)
size_t modbus_rtu_build_write_multiple_registers(uint8_t slave_id, uint16_t start_addr, uint16_t count, const modbus_reg_t *registers, uint8_t *frame, size_t frame_size);
modbus_err_t modbus_rtu_parse_write_multiple_registers(const uint8_t *frame, size_t frame_len, uint8_t slave_id, uint16_t start_addr, uint16_t count);
//******************************************************************************
// Секция определения макросов
//******************************************************************************


#ifdef __cplusplus
}
#endif
//******************************************************************************
// ENF OF FILE
//******************************************************************************