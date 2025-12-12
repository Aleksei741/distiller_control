#pragma once
//******************************************************************************
// Секция include
//******************************************************************************

//******************************************************************************
// Секция определения констант
//******************************************************************************

//******************************************************************************
// Секция определения типов
//******************************************************************************
typedef struct {
    uint32_t time;
    uint8_t heater;
    float temperature_kube;
    float temperature_column;
} statistic_t;

typedef enum {
    PSRAM_STAT_OK = 0,             // всё нормально
    PSRAM_STAT_OLD_INDEX,          // клиент прислал индекс, данные уже перезаписаны
    PSRAM_STAT_INVALID_INDEX       // индекс больше latest_index
} psram_stat_status_t;
//******************************************************************************
// Секция определения глобальных переменных
//******************************************************************************

//******************************************************************************
// Секция прототипов глобальных функций
//******************************************************************************
bool psram_statistic_init();
void psram_statistic_free();

void psram_statistic_lock_write();
void psram_statistic_unlock_write();

size_t psram_statistic_get_first_index();
size_t psram_statistic_get_latest_index();
size_t psram_statistic_get_count();

void psram_statistic_push(statistic_t *elem);

bool psram_statistic_read_by_global_index(size_t global_index, statistic_t *out);
size_t psram_statistic_read_range_by_global_index(size_t start_global, size_t end_global, statistic_t *out_array, size_t max_count);
psram_stat_status_t psram_stat_check_global_index(size_t client_index, size_t *adjusted_index);

//******************************************************************************
// Секция определения макросов
//******************************************************************************

//******************************************************************************
// ENF OF FILE
//******************************************************************************