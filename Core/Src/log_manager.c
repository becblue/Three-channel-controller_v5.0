#include "log_manager.h"

void LogManager_Init(void)
{
    /* TODO: 初始化 2000 条循环日志区域和复位原因记录。 */
}

void LogManager_Task(uint32_t tick_ms)
{
    (void)tick_ms;
    /* TODO: 推进非阻塞 Flash 写入和串口分页输出。 */
}

void LogManager_Record(LogEventId_t event_id, uint16_t param, uint32_t detail)
{
    (void)event_id;
    (void)param;
    (void)detail;
    /* TODO: 事件发生时写入固定长度日志记录，不做每秒定时写入。 */
}

void LogManager_Clear(void)
{
    /* TODO: 清空历史日志后立即写入 LOG_EVENT_LOG_CLEARED。 */
}
