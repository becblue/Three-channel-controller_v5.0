#ifndef __LOG_MANAGER_H__
#define __LOG_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
    LOG_EVENT_BOOT = 0,
    LOG_EVENT_RESET_REASON,
    LOG_EVENT_WDT_RESET,
    LOG_EVENT_CHANNEL_ACTION,
    LOG_EVENT_RELAY_ACTION_FAILED,
    LOG_EVENT_FAULT_SET,
    LOG_EVENT_FAULT_CLEAR,
    LOG_EVENT_LOG_CLEARED
} LogEventId_t;

typedef struct
{
    uint32_t time_s;
    uint16_t event_id;
    uint16_t param;
    uint32_t detail;
} AppLogRecord_t;

void LogManager_Init(void);
void LogManager_Task(uint32_t tick_ms);
void LogManager_Record(LogEventId_t event_id, uint16_t param, uint32_t detail);
void LogManager_Clear(void);

#ifdef __cplusplus
}
#endif

#endif /* __LOG_MANAGER_H__ */
