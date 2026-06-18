#include "alarm_output.h"

void AlarmOutput_Init(void)
{
    /* TODO: 初始化 ALARM 和 BEEP 为非报警状态。 */
}

void AlarmOutput_Task(uint32_t tick_ms, SafetySnapshot_t safety_snapshot)
{
    (void)tick_ms;
    (void)safety_snapshot;
    /* TODO: 实现 ALARM 持续低电平和蜂鸣器等级时序。 */
}
