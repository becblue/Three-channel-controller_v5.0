#ifndef __PERSISTENT_LOG_H__
#define __PERSISTENT_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "log_manager.h"

void PersistentLog_Init(void);
uint8_t PersistentLog_IsReady(void);
uint32_t PersistentLog_AllocateSequence(void);
void PersistentLog_Enqueue(const AppLogRecord_t *record);
void PersistentLog_Task(uint32_t tick_ms);
void PersistentLog_LoadRecent(AppLogRecord_t *records, uint16_t max_records, uint16_t *record_count);

#ifdef __cplusplus
}
#endif

#endif /* __PERSISTENT_LOG_H__ */
