#ifndef __WATCHDOG_MANAGER_H__
#define __WATCHDOG_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
    WATCHDOG_PANIC_NONE = 0,
    WATCHDOG_PANIC_MAIN_LOOP_TIMEOUT,
    WATCHDOG_PANIC_TASK_MISSED,
    WATCHDOG_PANIC_TICK_ERROR,
    WATCHDOG_PANIC_STATE_ERROR
} WatchdogPanicReason_t;

typedef enum
{
    WATCHDOG_TASK_INPUT = 0,
    WATCHDOG_TASK_RELAY,
    WATCHDOG_TASK_FEEDBACK,
    WATCHDOG_TASK_SAFETY,
    WATCHDOG_TASK_OUTPUT,
    WATCHDOG_TASK_COUNT
} WatchdogTaskId_t;

void WatchdogManager_Init(void);
void WatchdogManager_Task(uint32_t tick_ms);
void WatchdogManager_MarkTaskAlive(WatchdogTaskId_t task_id);
void WatchdogManager_Panic(WatchdogPanicReason_t reason);
WatchdogPanicReason_t WatchdogManager_GetLastPanicReason(void);
uint8_t WatchdogManager_WasIwdgReset(void);

#ifdef __cplusplus
}
#endif

#endif /* __WATCHDOG_MANAGER_H__ */
