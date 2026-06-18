#include "maintenance_manager.h"
#include "log_manager.h"

void MaintenanceManager_Init(void)
{
    LogManager_Init();
    /* TODO: 初始化 KEY1/KEY2 日志维护状态和串口分页状态。 */
}

void MaintenanceManager_Task(uint32_t tick_ms)
{
    LogManager_Task(tick_ms);
    /* TODO: 实现两键短按/长按日志维护，不影响安全链路。 */
}
