#include "feedback_monitor.h"

static FeedbackSnapshot_t g_feedback_snapshot;

void FeedbackMonitor_Init(void)
{
    g_feedback_snapshot.channel_1 = APP_CHANNEL_STATE_UNKNOWN;
    g_feedback_snapshot.channel_2 = APP_CHANNEL_STATE_UNKNOWN;
    g_feedback_snapshot.channel_3 = APP_CHANNEL_STATE_UNKNOWN;
    g_feedback_snapshot.fault_mask_b_to_j = 0U;
}

void FeedbackMonitor_Task(uint32_t tick_ms)
{
    (void)tick_ms;
    /* TODO: 实现九路反馈快照、通道状态识别和 B~J 异常依据输出。 */
}

FeedbackSnapshot_t FeedbackMonitor_GetSnapshot(void)
{
    return g_feedback_snapshot;
}

AppChannel_t FeedbackMonitor_GetOpenChannel(void)
{
    /* TODO: 根据九路反馈一致性返回当前实际打开通道。 */
    return APP_CHANNEL_NONE;
}
