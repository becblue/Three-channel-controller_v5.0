#include "channel_request.h"

void ChannelRequest_Init(void)
{
    /* 当前层无硬件资源，初始化时只保留接口一致性。 */
}

ChannelRequest_t ChannelRequest_Evaluate(InputFilterSnapshot_t input_snapshot,
                                         FeedbackSnapshot_t feedback_snapshot,
                                         uint8_t relay_busy,
                                         uint8_t action_allowed)
{
    ChannelRequest_t request;

    (void)input_snapshot;
    (void)feedback_snapshot;
    (void)relay_busy;
    (void)action_allowed;

    request.channel = APP_CHANNEL_NONE;
    request.action = APP_ACTION_NONE;
    request.valid = 0U;

    /* TODO: 实现三路互锁、禁止自动切换和动作请求生成。 */
    return request;
}
