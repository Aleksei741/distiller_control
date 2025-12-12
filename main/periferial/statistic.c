//******************************************************************************
//include
//******************************************************************************
#include "statistic.h"
#include "esp_log.h"          // ESP_LOGI
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//******************************************************************************
// Constants
//******************************************************************************
#define SIZE_RESERVE_BUFFER 10
//******************************************************************************
// Type
//******************************************************************************
typedef struct {
    statistic_t *buffer;   // указатель на PSRAM
    size_t capacity;       // количество элементов в массиве
    size_t head;           // индекс для записи следующего элемента
    size_t count;          // текущий размер буфера
    size_t global_index;   // глобальный индекс следующего элемента
} psram_ring_t;
//******************************************************************************
// Variables
//******************************************************************************
//------------------------------------------------------------------------------
// Global
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local Variable
//------------------------------------------------------------------------------
static const char *TAG = "CIRC_STATITISTIC";

static SemaphoreHandle_t psram_mutex = NULL;
static psram_ring_t param_ring;

static statistic_t reserve_buffer[SIZE_RESERVE_BUFFER] = {0};
static size_t reserve_buffer_count = 0;
static bool block_write = false;
//******************************************************************************
// Function prototype
//******************************************************************************
inline void psram_statistic_push_(statistic_t *elem);
//******************************************************************************
// Function
//******************************************************************************
// Инициализация буфера в PSRAM (выделяем максимально возможное место)
bool psram_statistic_init() 
{
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    // Выравнивание под sizeof(statistic_t)
    size_t element_size = sizeof(statistic_t);
    size_t count = free_psram / element_size;

    if (count == 0) {
        ESP_LOGE(TAG, "Not enough PSRAM to allocate even one element!");
        return false;
    }

    // Выделяем память
    param_ring.buffer = (statistic_t *)heap_caps_malloc(count * element_size, MALLOC_CAP_SPIRAM);
    if (!param_ring.buffer) {
        ESP_LOGE(TAG, "Failed to allocate PSRAM!");
        return false;
    }

    param_ring.capacity = count;
    param_ring.head = 0;
    param_ring.count = 0;

    psram_mutex = xSemaphoreCreateMutex();
    if (psram_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create PSRAM mutex!");
        free(param_ring.buffer);
        param_ring.buffer = NULL;
        return false;
    }

    ESP_LOGI(TAG, "PSRAM ring buffer initialized: capacity = %d elements, element size = %d bytes",
             (int)param_ring.capacity, (int)element_size);
    return true;
}
//------------------------------------------------------------------------------
void psram_statistic_free() 
{
    if (!param_ring.buffer) return;

    if (psram_mutex) 
    {
        xSemaphoreTake(psram_mutex, portMAX_DELAY);
        free(param_ring.buffer);
        param_ring.buffer = NULL;
        param_ring.capacity = 0;
        param_ring.head = 0;
        param_ring.count = 0;
        xSemaphoreGive(psram_mutex);
        vSemaphoreDelete(psram_mutex);
        psram_mutex = NULL;
    }
}
//------------------------------------------------------------------------------
void psram_statistic_lock_write()
{
    if (xSemaphoreTake(psram_mutex, portMAX_DELAY) == pdTRUE) 
    {
        block_write = true;
        xSemaphoreGive(psram_mutex);
    }
}
//------------------------------------------------------------------------------
void psram_statistic_unlock_write()
{
    if (xSemaphoreTake(psram_mutex, portMAX_DELAY) == pdTRUE) 
    {
        block_write = false;
        xSemaphoreGive(psram_mutex);
    }
}
//------------------------------------------------------------------------------
// Получение первого
size_t psram_statistic_get_first_index()
{
    if (!param_ring.buffer || !psram_mutex) return 0;

    size_t first = 0;
    if (xSemaphoreTake(psram_mutex, portMAX_DELAY) == pdTRUE)
    {
        if (param_ring.count > 0)
            first = param_ring.global_index - param_ring.count;
        else
            first = param_ring.global_index;
        
        xSemaphoreGive(psram_mutex);
    }
    return first;
}
//------------------------------------------------------------------------------
// Получение последнего индекса
size_t psram_statistic_get_latest_index() 
{
    size_t latest = 0;
    if (psram_mutex && xSemaphoreTake(psram_mutex, portMAX_DELAY) == pdTRUE) 
    {
        latest = param_ring.global_index;
        xSemaphoreGive(psram_mutex);
    }
    return latest;
}
//------------------------------------------------------------------------------
size_t psram_statistic_get_count()
{
    if (!param_ring.buffer || !psram_mutex)
        return 0;

    size_t count = 0;

    if (xSemaphoreTake(psram_mutex, portMAX_DELAY) == pdTRUE)
    {
        count = param_ring.count;
        xSemaphoreGive(psram_mutex);
    }

    return count;
}
//------------------------------------------------------------------------------
// Добавление нового элемента (циклическое перезаписывание)
void psram_statistic_push(statistic_t *elem) 
{
    if (!param_ring.buffer || !elem || !psram_mutex) return;
    
    if (xSemaphoreTake(psram_mutex, portMAX_DELAY) == pdTRUE) 
    {
        if(block_write)
        {
            if(reserve_buffer_count >= SIZE_RESERVE_BUFFER)
            {
                block_write = false;

                for(cnt = 0; cnt < SIZE_RESERVE_BUFFER; cnt++)
                {
                    psram_statistic_push_(&reserve_buffer[cnt]);
                }
                reserve_buffer_count = 0;
            }
            else
            {
                reserve_buffer[reserve_buffer_count++] = *elem;
            }
        }
        else
        {
            psram_statistic_push_(elem);
        }

        xSemaphoreGive(psram_mutex);
    }
}
//------------------------------------------------------------------------------
// Чтение элемента по индексу
bool psram_statistic_read(size_t index, statistic_t *out) 
{
    if (!param_ring.buffer || !out || !psram_mutex) return false;

    bool result = false;

    if (xSemaphoreTake(psram_mutex, portMAX_DELAY) == pdTRUE) 
    {
        if (param_ring.count == 0) 
        {
            result = false;
        } 
        else 
        {
            size_t first_global_index = param_ring.global_index - param_ring.count;
            if (index < first_global_index || index >= param_ring.global_index) 
            {
                result = false;
            } 
            else 
            {
                // вычисляем позицию в циклическом буфере
                size_t idx = (param_ring.head + param_ring.capacity - (param_ring.global_index - index)) % param_ring.capacity;
                *out = param_ring.buffer[idx];
                result = true;
            }
        }
        xSemaphoreGive(psram_mutex);
    }

    return result;
}
//------------------------------------------------------------------------------
// Чтение элементов по диапазону индексов
size_t psram_statistic_read_range_by_global_index(size_t start_global, size_t end_global, statistic_t *out_array, size_t max_count)
{
    if (!param_ring.buffer || !out_array || !psram_mutex) return 0;

    size_t read_count = 0;
    if (xSemaphoreTake(psram_mutex, portMAX_DELAY) == pdTRUE) 
    {
        size_t first_global_index = (param_ring.count < param_ring.capacity) ? 0 : param_ring.global_index - param_ring.count;
        size_t latest_global_index = param_ring.global_index - 1;

        // корректируем границы
        if (start_global < first_global_index) start_global = first_global_index;
        if (end_global > latest_global_index + 1) end_global = latest_global_index + 1;

        read_count = end_global - start_global;
        if (read_count > max_count) read_count = max_count;

        for (size_t i = 0; i < read_count; i++) 
        {
            size_t idx = (param_ring.head + param_ring.capacity - (param_ring.global_index - (start_global + i))) % param_ring.capacity;
            out_array[i] = param_ring.buffer[idx];
        }

        xSemaphoreGive(psram_mutex);
    }
    return read_count;
}

//------------------------------------------------------------------------------
psram_stat_status_t psram_stat_check_global_index(size_t client_index, size_t *adjusted_index) 
{
    if (!param_ring.buffer || !psram_mutex) return PSRAM_STAT_INVALID_INDEX;

    psram_stat_status_t status = PSRAM_STAT_OK;

    if (xSemaphoreTake(psram_mutex, portMAX_DELAY) == pdTRUE) 
    {
        size_t first_global_index = (param_ring.count < param_ring.capacity) ? 0 
                                  : param_ring.global_index - param_ring.count;
        size_t latest_global_index = param_ring.global_index - 1;

        if (client_index < first_global_index) 
        {
            if (adjusted_index) *adjusted_index = first_global_index;
            status = PSRAM_STAT_OLD_INDEX;
        } else if (client_index > latest_global_index) 
        {
            if (adjusted_index) *adjusted_index = latest_global_index;
            status = PSRAM_STAT_INVALID_INDEX;
        } else 
        {
            if (adjusted_index) *adjusted_index = client_index;
            status = PSRAM_STAT_OK;
        }

        xSemaphoreGive(psram_mutex);
    }

    return status;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// local functions
//------------------------------------------------------------------------------
inline void psram_statistic_push_(statistic_t *elem) 
{    
    param_ring.buffer[param_ring.head] = *elem; 
    param_ring.head = (param_ring.head + 1) % param_ring.capacity; 
    if (param_ring.count < param_ring.capacity)
        param_ring.count++; 

    param_ring.global_index++; // увеличиваем глобальный индекс     
}