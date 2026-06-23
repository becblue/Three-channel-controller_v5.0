#include "log_manager.h"

#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "app_types.h"
#include "persistent_log.h"
#include "reset_reason_store.h"
#include "usart.h"

typedef enum
{
    LOG_PRINT_IDLE = 0,
    LOG_PRINT_HEADER,
    LOG_PRINT_RECORD,
    LOG_PRINT_END
} LogPrintState_t;

static AppLogRecord_t g_records[APP_LOG_MAX_RECORDS];
static uint16_t g_record_start;
static uint16_t g_record_count;
static uint32_t g_uptime_s;
static uint32_t g_last_time_tick;

static LogPrintState_t g_print_state;
static uint16_t g_print_page;
static uint16_t g_print_page_count;
static uint16_t g_print_page_record_count;
static uint16_t g_print_record_offset;
static char g_uart_line[96];

static uint16_t LogManager_GetPhysicalIndex(uint16_t logical_index);

static void LogManager_AddRecordToRam(const AppLogRecord_t *record)
{
    uint16_t write_index;

    if (g_record_count < APP_LOG_MAX_RECORDS)
    {
        write_index = LogManager_GetPhysicalIndex(g_record_count);
        g_record_count++;
    }
    else
    {
        write_index = g_record_start;
        g_record_start = (uint16_t)((g_record_start + 1U) % APP_LOG_MAX_RECORDS);
    }

    g_records[write_index] = *record;
}

static void LogManager_RecordResetReason(void)
{
    ResetReasonSnapshot_t reset_snapshot = ResetReasonStore_GetSnapshot();

    LogManager_Record(LOG_EVENT_RESET_REASON,
                      reset_snapshot.software_reason,
                      reset_snapshot.hardware_flags);

    if ((reset_snapshot.hardware_flags & RESET_HW_FLAG_IWDG) != 0U)
    {
        uint32_t detail = ((uint32_t)reset_snapshot.software_detail << 16) |
                          (uint32_t)reset_snapshot.software_reason;

        LogManager_Record(LOG_EVENT_WDT_RESET, reset_snapshot.software_reason, detail);
    }
}

static uint32_t LogManager_ElapsedMs(uint32_t now, uint32_t last)
{
    return now - last;
}

static uint16_t LogManager_GetPhysicalIndex(uint16_t logical_index)
{
    return (uint16_t)((g_record_start + logical_index) % APP_LOG_MAX_RECORDS);
}

static const char *LogManager_GetEventName(LogEventId_t event_id)
{
    switch (event_id)
    {
        case LOG_EVENT_BOOT:
            return "BOOT";
        case LOG_EVENT_RESET_REASON:
            return "RESET";
        case LOG_EVENT_WDT_RESET:
            return "WDT";
        case LOG_EVENT_CHANNEL_ACTION:
            return "CH_ACT";
        case LOG_EVENT_RELAY_ACTION_FAILED:
            return "RLY_FAIL";
        case LOG_EVENT_FAULT_SET:
            return "FLT_SET";
        case LOG_EVENT_FAULT_CLEAR:
            return "FLT_CLR";
        case LOG_EVENT_LOG_CLEARED:
            return "LOG_CLR";
        default:
            return "UNKNOWN";
    }
}

static char LogManager_GetFaultChar(uint16_t fault_id)
{
    if (fault_id < (uint16_t)APP_FAULT_COUNT)
    {
        return (char)('A' + fault_id);
    }

    return '?';
}

static void LogManager_SendLine(const char *line)
{
    (void)HAL_UART_Transmit(&huart3, (uint8_t *)line, (uint16_t)strlen(line), 20U);
}

static void LogManager_FormatRecord(const AppLogRecord_t *record, char *buffer, uint16_t buffer_size)
{
    if (record->event_id == (uint16_t)LOG_EVENT_CHANNEL_ACTION)
    {
        (void)snprintf(buffer,
                       buffer_size,
                       "%010lus %-8s CH%u %s\r\n",
                       (unsigned long)record->time_s,
                       LogManager_GetEventName((LogEventId_t)record->event_id),
                       (unsigned int)record->param,
                       (record->detail == (uint32_t)APP_ACTION_OPEN) ? "OPEN" : "CLOSE");
        return;
    }

    if ((record->event_id == (uint16_t)LOG_EVENT_FAULT_SET) ||
        (record->event_id == (uint16_t)LOG_EVENT_FAULT_CLEAR))
    {
        (void)snprintf(buffer,
                       buffer_size,
                       "%010lus %-8s FLT_%c\r\n",
                       (unsigned long)record->time_s,
                       LogManager_GetEventName((LogEventId_t)record->event_id),
                       LogManager_GetFaultChar(record->param));
        return;
    }

    if (record->event_id == (uint16_t)LOG_EVENT_RESET_REASON)
    {
        (void)snprintf(buffer,
                       buffer_size,
                       "%010lus %-8s HW%lu SW%u\r\n",
                       (unsigned long)record->time_s,
                       LogManager_GetEventName((LogEventId_t)record->event_id),
                       (unsigned long)record->detail,
                       (unsigned int)record->param);
        return;
    }

    if (record->event_id == (uint16_t)LOG_EVENT_WDT_RESET)
    {
        (void)snprintf(buffer,
                       buffer_size,
                       "%010lus %-8s SW%u D%lu\r\n",
                       (unsigned long)record->time_s,
                       LogManager_GetEventName((LogEventId_t)record->event_id),
                       (unsigned int)record->param,
                       (unsigned long)record->detail);
        return;
    }

    (void)snprintf(buffer,
                   buffer_size,
                   "%010lus %-8s P%u D%lu\r\n",
                   (unsigned long)record->time_s,
                   LogManager_GetEventName((LogEventId_t)record->event_id),
                   (unsigned int)record->param,
                   (unsigned long)record->detail);
}

void LogManager_Init(void)
{
    g_record_start = 0U;
    g_record_count = 0U;
    g_uptime_s = 0U;
    g_last_time_tick = 0U;
    g_print_state = LOG_PRINT_IDLE;
    g_print_page = 0U;
    g_print_page_count = 0U;
    g_print_page_record_count = 0U;
    g_print_record_offset = 0U;

    PersistentLog_Init();
    PersistentLog_LoadRecent(g_records, APP_LOG_MAX_RECORDS, &g_record_count);

    LogManager_Record(LOG_EVENT_BOOT, 0U, 0U);
    LogManager_RecordResetReason();
}

void LogManager_Task(uint32_t tick_ms)
{
    PersistentLog_Task(tick_ms);

    while (LogManager_ElapsedMs(tick_ms, g_last_time_tick) >= APP_LOG_TIME_UNIT_MS)
    {
        g_last_time_tick += APP_LOG_TIME_UNIT_MS;
        g_uptime_s++;
    }

    if (g_print_state == LOG_PRINT_IDLE)
    {
        return;
    }

    if (g_print_state == LOG_PRINT_HEADER)
    {
        (void)snprintf(g_uart_line,
                       sizeof(g_uart_line),
                       "\r\nLOG PAGE %u/%u  COUNT %u\r\n",
                       (unsigned int)(g_print_page + 1U),
                       (unsigned int)g_print_page_count,
                       (unsigned int)g_record_count);
        LogManager_SendLine(g_uart_line);
        g_print_state = LOG_PRINT_RECORD;
        return;
    }

    if (g_print_state == LOG_PRINT_RECORD)
    {
        uint16_t logical_index;
        uint16_t physical_index;

        if (g_print_record_offset >= g_print_page_record_count)
        {
            g_print_state = LOG_PRINT_END;
            return;
        }

        logical_index = (uint16_t)((g_print_page * APP_LOG_PAGE_SIZE) + g_print_record_offset);
        physical_index = LogManager_GetPhysicalIndex(logical_index);
        LogManager_FormatRecord(&g_records[physical_index], g_uart_line, sizeof(g_uart_line));
        LogManager_SendLine(g_uart_line);
        g_print_record_offset++;
        return;
    }

    LogManager_SendLine("--END--\r\n");
    g_print_state = LOG_PRINT_IDLE;
}

void LogManager_Record(LogEventId_t event_id, uint16_t param, uint32_t detail)
{
    AppLogRecord_t record;

    if (APP_LOG_ENABLE == 0U)
    {
        return;
    }

    record.sequence = PersistentLog_AllocateSequence();
    record.time_s = g_uptime_s;
    record.event_id = (uint16_t)event_id;
    record.param = param;
    record.detail = detail;
    record.extra = 0U;

    LogManager_AddRecordToRam(&record);
    PersistentLog_Enqueue(&record);
}

void LogManager_Clear(void)
{
    g_record_start = 0U;
    g_record_count = 0U;
    g_print_state = LOG_PRINT_IDLE;
    LogManager_Record(LOG_EVENT_LOG_CLEARED, 0U, 0U);
}

uint16_t LogManager_GetRecordCount(void)
{
    return g_record_count;
}

uint16_t LogManager_GetPageCount(void)
{
    uint16_t page_count;

    page_count = (uint16_t)((g_record_count + APP_LOG_PAGE_SIZE - 1U) / APP_LOG_PAGE_SIZE);
    return (page_count == 0U) ? 1U : page_count;
}

void LogManager_PrintPage(uint16_t page_index)
{
    uint16_t remaining;

    if (APP_LOG_ENABLE == 0U)
    {
        return;
    }

    g_print_page_count = LogManager_GetPageCount();

    if (page_index >= g_print_page_count)
    {
        page_index = (uint16_t)(g_print_page_count - 1U);
    }

    g_print_page = page_index;
    g_print_record_offset = 0U;

    if (g_record_count == 0U)
    {
        g_print_page_record_count = 0U;
    }
    else
    {
        remaining = (uint16_t)(g_record_count - (g_print_page * APP_LOG_PAGE_SIZE));
        g_print_page_record_count = (remaining > APP_LOG_PAGE_SIZE) ? APP_LOG_PAGE_SIZE : remaining;
    }

    g_print_state = LOG_PRINT_HEADER;
}

void LogManager_StopPrint(void)
{
    g_print_state = LOG_PRINT_IDLE;
    LogManager_SendLine("\r\nLOG EXIT\r\n");
}
