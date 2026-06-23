#include "persistent_log.h"

#include <string.h>

#include "app_config.h"
#include "flash_w25q128.h"

#define PERSIST_LOG_BASE_ADDRESS       0UL
#define PERSIST_LOG_AREA_SIZE          APP_FLASH_LOG_AREA_SIZE
#define PERSIST_LOG_RECORD_SIZE        APP_FLASH_LOG_RECORD_SIZE
#define PERSIST_LOG_RECORD_MAGIC       0x4C4F4735UL
#define PERSIST_LOG_QUEUE_SIZE         32U
#define PERSIST_LOG_WRITE_INTERVAL_MS  10U

typedef struct
{
    uint32_t magic;
    uint32_t sequence;
    uint32_t time_s;
    uint16_t event_id;
    uint16_t param;
    uint32_t detail;
    uint32_t extra;
    uint32_t crc;
    uint32_t reserved;
} PersistentLogRawRecord_t;

static uint8_t g_persist_ready;
static uint32_t g_next_address;
static uint32_t g_next_sequence;
static uint32_t g_latest_clear_sequence;
static uint32_t g_latest_sequence;
static uint32_t g_last_write_tick;
static AppLogRecord_t g_queue[PERSIST_LOG_QUEUE_SIZE];
static uint8_t g_queue_head;
static uint8_t g_queue_tail;
static uint8_t g_queue_count;
static uint8_t g_scan_buffer[W25Q128_PAGE_SIZE];

static uint32_t PersistentLog_CalcCrc(const PersistentLogRawRecord_t *raw)
{
    return raw->magic ^
           raw->sequence ^
           raw->time_s ^
           ((uint32_t)raw->event_id << 16) ^
           (uint32_t)raw->param ^
           raw->detail ^
           raw->extra ^
           raw->reserved ^
           0xA5A55A5AUL;
}

static uint8_t PersistentLog_IsErasedRecord(const uint8_t *data)
{
    uint8_t index;

    for (index = 0U; index < PERSIST_LOG_RECORD_SIZE; index++)
    {
        if (data[index] != 0xFFU)
        {
            return 0U;
        }
    }

    return 1U;
}

static uint8_t PersistentLog_ParseRaw(const uint8_t *data, AppLogRecord_t *record)
{
    PersistentLogRawRecord_t raw;

    if (PersistentLog_IsErasedRecord(data) != 0U)
    {
        return 0U;
    }

    (void)memcpy(&raw, data, sizeof(raw));

    if ((raw.magic != PERSIST_LOG_RECORD_MAGIC) ||
        (raw.crc != PersistentLog_CalcCrc(&raw)))
    {
        return 0U;
    }

    record->sequence = raw.sequence;
    record->time_s = raw.time_s;
    record->event_id = raw.event_id;
    record->param = raw.param;
    record->detail = raw.detail;
    record->extra = raw.extra;
    return 1U;
}

static void PersistentLog_MakeRaw(const AppLogRecord_t *record, PersistentLogRawRecord_t *raw)
{
    raw->magic = PERSIST_LOG_RECORD_MAGIC;
    raw->sequence = record->sequence;
    raw->time_s = record->time_s;
    raw->event_id = record->event_id;
    raw->param = record->param;
    raw->detail = record->detail;
    raw->extra = record->extra;
    raw->reserved = 0xFFFFFFFFUL;
    raw->crc = PersistentLog_CalcCrc(raw);
}

static void PersistentLog_UpdateScanState(const AppLogRecord_t *record, uint32_t address)
{
    if (record->sequence >= g_latest_sequence)
    {
        g_latest_sequence = record->sequence;
        g_next_address = address + PERSIST_LOG_RECORD_SIZE;

        if (g_next_address >= PERSIST_LOG_AREA_SIZE)
        {
            g_next_address = 0U;
        }
    }

    if ((record->event_id == (uint16_t)LOG_EVENT_LOG_CLEARED) &&
        (record->sequence > g_latest_clear_sequence))
    {
        g_latest_clear_sequence = record->sequence;
    }
}

static void PersistentLog_ScanSummary(void)
{
    uint32_t address;

    g_next_address = 0U;
    g_next_sequence = 1U;
    g_latest_sequence = 0U;
    g_latest_clear_sequence = 0U;

    for (address = 0U; address < PERSIST_LOG_AREA_SIZE; address += W25Q128_PAGE_SIZE)
    {
        uint16_t offset;

        if (W25Q128_Read(PERSIST_LOG_BASE_ADDRESS + address, g_scan_buffer, W25Q128_PAGE_SIZE) != W25Q_RESULT_OK)
        {
            g_persist_ready = 0U;
            return;
        }

        for (offset = 0U; offset < W25Q128_PAGE_SIZE; offset += PERSIST_LOG_RECORD_SIZE)
        {
            AppLogRecord_t record;

            if (PersistentLog_ParseRaw(&g_scan_buffer[offset], &record) != 0U)
            {
                PersistentLog_UpdateScanState(&record, address + offset);
            }
        }
    }

    if (g_latest_sequence != 0U)
    {
        g_next_sequence = g_latest_sequence + 1U;
    }
}

static uint32_t PersistentLog_GetLoadThreshold(uint16_t max_records)
{
    uint32_t threshold = g_latest_clear_sequence + 1U;

    if ((g_latest_sequence >= max_records) &&
        ((g_latest_sequence - (uint32_t)max_records + 1UL) > threshold))
    {
        threshold = g_latest_sequence - (uint32_t)max_records + 1UL;
    }

    return threshold;
}

static void PersistentLog_InsertLoadedRecord(AppLogRecord_t *records, uint16_t max_records, uint16_t *count, const AppLogRecord_t *record)
{
    uint16_t index;

    if (*count >= max_records)
    {
        return;
    }

    index = *count;

    while ((index > 0U) && (records[index - 1U].sequence > record->sequence))
    {
        records[index] = records[index - 1U];
        index--;
    }

    records[index] = *record;
    (*count)++;
}

static uint8_t PersistentLog_IsQueueFull(void)
{
    return (g_queue_count >= PERSIST_LOG_QUEUE_SIZE) ? 1U : 0U;
}

static uint8_t PersistentLog_Dequeue(AppLogRecord_t *record)
{
    if (g_queue_count == 0U)
    {
        return 0U;
    }

    *record = g_queue[g_queue_tail];
    g_queue_tail = (uint8_t)((g_queue_tail + 1U) % PERSIST_LOG_QUEUE_SIZE);
    g_queue_count--;
    return 1U;
}

static void PersistentLog_RequeueFront(const AppLogRecord_t *record)
{
    if (g_queue_count >= PERSIST_LOG_QUEUE_SIZE)
    {
        return;
    }

    if (g_queue_tail == 0U)
    {
        g_queue_tail = PERSIST_LOG_QUEUE_SIZE - 1U;
    }
    else
    {
        g_queue_tail--;
    }

    g_queue[g_queue_tail] = *record;
    g_queue_count++;
}

static uint8_t PersistentLog_IsAddressAtSectorStart(uint32_t address)
{
    return ((address % W25Q128_SECTOR_SIZE) == 0U) ? 1U : 0U;
}

static uint8_t PersistentLog_NeedEraseCurrentSector(void)
{
    uint8_t header[PERSIST_LOG_RECORD_SIZE];
    uint8_t index;

    if (W25Q128_Read(PERSIST_LOG_BASE_ADDRESS + g_next_address, header, sizeof(header)) != W25Q_RESULT_OK)
    {
        return 1U;
    }

    for (index = 0U; index < sizeof(header); index++)
    {
        if (header[index] != 0xFFU)
        {
            return 1U;
        }
    }

    return 0U;
}

static void PersistentLog_MoveToNextSector(void)
{
    g_next_address = ((g_next_address / W25Q128_SECTOR_SIZE) + 1UL) * W25Q128_SECTOR_SIZE;

    if (g_next_address >= PERSIST_LOG_AREA_SIZE)
    {
        g_next_address = 0U;
    }
}

static uint8_t PersistentLog_WriteRecord(const AppLogRecord_t *record)
{
    PersistentLogRawRecord_t raw;

    if (PersistentLog_IsAddressAtSectorStart(g_next_address) != 0U)
    {
        if (PersistentLog_NeedEraseCurrentSector() != 0U)
        {
            if (W25Q128_SectorErase(PERSIST_LOG_BASE_ADDRESS + g_next_address) != W25Q_RESULT_OK)
            {
                return 0U;
            }
        }
    }
    else if (PersistentLog_NeedEraseCurrentSector() != 0U)
    {
        PersistentLog_MoveToNextSector();

        if (W25Q128_SectorErase(PERSIST_LOG_BASE_ADDRESS + g_next_address) != W25Q_RESULT_OK)
        {
            return 0U;
        }
    }

    PersistentLog_MakeRaw(record, &raw);

    if (W25Q128_PageProgram(PERSIST_LOG_BASE_ADDRESS + g_next_address,
                            (const uint8_t *)&raw,
                            sizeof(raw)) != W25Q_RESULT_OK)
    {
        return 0U;
    }

    g_next_address += PERSIST_LOG_RECORD_SIZE;

    if (g_next_address >= PERSIST_LOG_AREA_SIZE)
    {
        g_next_address = 0U;
    }

    return 1U;
}

void PersistentLog_Init(void)
{
    g_persist_ready = 0U;
    g_next_address = 0U;
    g_next_sequence = 1U;
    g_latest_clear_sequence = 0U;
    g_latest_sequence = 0U;
    g_last_write_tick = 0U;
    g_queue_head = 0U;
    g_queue_tail = 0U;
    g_queue_count = 0U;

    W25Q128_Init();

    if (W25Q128_IsReady() == 0U)
    {
        return;
    }

    g_persist_ready = 1U;
    PersistentLog_ScanSummary();
}

uint8_t PersistentLog_IsReady(void)
{
    return g_persist_ready;
}

uint32_t PersistentLog_AllocateSequence(void)
{
    uint32_t sequence = g_next_sequence;

    if (g_next_sequence < 0xFFFFFFFFUL)
    {
        g_next_sequence++;
    }

    return sequence;
}

void PersistentLog_Enqueue(const AppLogRecord_t *record)
{
    if ((record == 0) || (g_persist_ready == 0U) || (PersistentLog_IsQueueFull() != 0U))
    {
        return;
    }

    g_queue[g_queue_head] = *record;
    g_queue_head = (uint8_t)((g_queue_head + 1U) % PERSIST_LOG_QUEUE_SIZE);
    g_queue_count++;
}

void PersistentLog_Task(uint32_t tick_ms)
{
    AppLogRecord_t record;

    if ((g_persist_ready == 0U) || (g_queue_count == 0U))
    {
        return;
    }

    if ((tick_ms - g_last_write_tick) < PERSIST_LOG_WRITE_INTERVAL_MS)
    {
        return;
    }

    g_last_write_tick = tick_ms;

    if (PersistentLog_Dequeue(&record) == 0U)
    {
        return;
    }

    if (PersistentLog_WriteRecord(&record) == 0U)
    {
        PersistentLog_RequeueFront(&record);
    }
}

void PersistentLog_LoadRecent(AppLogRecord_t *records, uint16_t max_records, uint16_t *record_count)
{
    uint32_t address;
    uint32_t threshold;

    if ((records == 0) || (record_count == 0))
    {
        return;
    }

    *record_count = 0U;

    if (g_persist_ready == 0U)
    {
        return;
    }

    threshold = PersistentLog_GetLoadThreshold(max_records);

    for (address = 0U; address < PERSIST_LOG_AREA_SIZE; address += W25Q128_PAGE_SIZE)
    {
        uint16_t offset;

        if (W25Q128_Read(PERSIST_LOG_BASE_ADDRESS + address, g_scan_buffer, W25Q128_PAGE_SIZE) != W25Q_RESULT_OK)
        {
            g_persist_ready = 0U;
            *record_count = 0U;
            return;
        }

        for (offset = 0U; offset < W25Q128_PAGE_SIZE; offset += PERSIST_LOG_RECORD_SIZE)
        {
            AppLogRecord_t record;

            if ((PersistentLog_ParseRaw(&g_scan_buffer[offset], &record) != 0U) &&
                (record.sequence >= threshold))
            {
                PersistentLog_InsertLoadedRecord(records, max_records, record_count, &record);
            }
        }
    }
}
