#include "relay_driver.h"

static uint8_t g_relay_busy;
static RelayDoneEvent_t g_done_event;

void RelayDriver_Init(void)
{
    g_relay_busy = 0U;
    g_done_event.ready_for_feedback = 0U;
    RelayDriver_ForceAllOutputsInactive();
}

void RelayDriver_Task(uint32_t tick_ms)
{
    (void)tick_ms;
    /* TODO: 实现 500ms 低电平脉冲和 500ms 反馈等待的非阻塞状态机。 */
}

AppResult_t RelayDriver_Start(RelayCommand_t command, uint32_t tick_ms)
{
    (void)command;
    (void)tick_ms;
    if (g_relay_busy != 0U)
    {
        return APP_RESULT_BUSY;
    }

    /* TODO: 校验命令并启动对应通道的双继电器动作。 */
    return APP_RESULT_REJECTED;
}

uint8_t RelayDriver_IsBusy(void)
{
    return g_relay_busy;
}

uint8_t RelayDriver_TakeDoneEvent(RelayDoneEvent_t *event)
{
    if ((event == 0) || (g_done_event.ready_for_feedback == 0U))
    {
        return 0U;
    }

    *event = g_done_event;
    g_done_event.ready_for_feedback = 0U;
    return 1U;
}

void RelayDriver_ForceAllOutputsInactive(void)
{
    /* TODO: 调用 board_io 将所有低电平有效继电器输出恢复为非动作状态。 */
}
