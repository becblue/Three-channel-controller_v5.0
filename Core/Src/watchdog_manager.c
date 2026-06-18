#include "watchdog_manager.h"

static WatchdogPanicReason_t g_panic_reason;

void WatchdogManager_Init(void)
{
    g_panic_reason = WATCHDOG_PANIC_NONE;
}

void WatchdogManager_Task(uint32_t tick_ms)
{
    (void)tick_ms;
    /* TODO: 实现集中喂狗、主循环健康检查和复位原因诊断。 */
}

void WatchdogManager_MarkTaskAlive(uint32_t task_id)
{
    (void)task_id;
    /* TODO: 记录关键任务心跳，供喂狗前统一检查。 */
}

void WatchdogManager_Panic(WatchdogPanicReason_t reason)
{
    g_panic_reason = reason;
    /* TODO: 写入复位诊断记录，并停止后续喂狗。 */
}

WatchdogPanicReason_t WatchdogManager_GetLastPanicReason(void)
{
    return g_panic_reason;
}
