#pragma once
//******************************************************************************
// Секция include
//******************************************************************************
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
//******************************************************************************
// Секция определения констант
//******************************************************************************
#define MODBUS_MAX_COILS_READ       2000U
#define MODBUS_MAX_COILS_WRITE      2000U
#define MODBUS_MAX_REGISTERS_READ   125U
#define MODBUS_MAX_REGISTERS_WRITE  123U
//******************************************************************************
// Секция определения типов
//******************************************************************************
/**
 * @brief Коды ошибок Modbus
 */
typedef enum 
{
    MODBUS_ERR_OK = 0,               ///< Успех
    MODBUS_ERR_NULL_PTR,             ///< Указатель NULL
    MODBUS_ERR_INVALID_ARG,          ///< Неверный аргумент (например, count=0)
    MODBUS_ERR_BUFFER_TOO_SMALL,     ///< Буфер слишком мал
    MODBUS_ERR_FRAME_TOO_SHORT,      ///< Фрейм короче минимального
    MODBUS_ERR_SLAVE_ID_MISMATCH,    ///< Несовпадение slave ID / unit ID
    MODBUS_ERR_FUNCTION_MISMATCH,    ///< Несовпадение функции
    MODBUS_ERR_TRANSACTION_MISMATCH, ///< Несовпадение transaction ID (TCP)
    MODBUS_ERR_CRC,                  ///< Ошибка CRC (только RTU)
    MODBUS_ERR_BYTE_COUNT,           ///< Неверное количество байт в ответе
    MODBUS_ERR_COIL_VALUE,           ///< Недопустимое значение coil (не 0x00 или 0xFF)
    MODBUS_ERR_ADDR_MISMATCH,        ///< Несовпадение адреса в ответе на запись
    MODBUS_ERR_COUNT_MISMATCH,       ///< Несовпадение количества в ответе на запись
    MODBUS_ERR_PROTOCOL_ID,          ///< Неверный Protocol ID (TCP)
} modbus_err_t;

/**
 * @brief Типы функций Modbus
 */
typedef enum 
{
    MODBUS_FUNC_READ_COILS = 0x01,
    MODBUS_FUNC_READ_DISCRETE_INPUTS = 0x02,
    MODBUS_FUNC_READ_HOLDING_REGISTERS = 0x03,
    MODBUS_FUNC_READ_INPUT_REGISTERS = 0x04,
    MODBUS_FUNC_WRITE_SINGLE_COIL = 0x05,
    MODBUS_FUNC_WRITE_SINGLE_REGISTER = 0x06,
    MODBUS_FUNC_WRITE_MULTIPLE_COILS = 0x0F,
    MODBUS_FUNC_WRITE_MULTIPLE_REGISTERS = 0x10,
} modbus_func_t;

typedef uint16_t modbus_reg_t;
//******************************************************************************
// Секция определения глобальных переменных
//******************************************************************************

//******************************************************************************
// Секция прототипов глобальных функций
//******************************************************************************

//******************************************************************************
// Секция определения макросов
//******************************************************************************

#ifdef __cplusplus
}
#endif
//******************************************************************************
// ENF OF FILE
//******************************************************************************