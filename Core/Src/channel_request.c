#include "channel_request.h"

static AppChannel_t ChannelRequest_GetInputChannel(InputFilterSnapshot_t input_snapshot, uint8_t *active_count)
{
    AppChannel_t channel = APP_CHANNEL_NONE;
    uint8_t count = 0U;

    if (input_snapshot.k1_en_active != 0U)
    {
        channel = APP_CHANNEL_1;
        count++;
    }

    if (input_snapshot.k2_en_active != 0U)
    {
        channel = APP_CHANNEL_2;
        count++;
    }

    if (input_snapshot.k3_en_active != 0U)
    {
        channel = APP_CHANNEL_3;
        count++;
    }

    *active_count = count;
    return (count == 1U) ? channel : APP_CHANNEL_NONE;
}

static AppChannel_t ChannelRequest_GetOpenChannel(FeedbackSnapshot_t feedback_snapshot)
{
    if (feedback_snapshot.channel_1 == APP_CHANNEL_STATE_OPEN)
    {
        return APP_CHANNEL_1;
    }

    if (feedback_snapshot.channel_2 == APP_CHANNEL_STATE_OPEN)
    {
        return APP_CHANNEL_2;
    }

    if (feedback_snapshot.channel_3 == APP_CHANNEL_STATE_OPEN)
    {
        return APP_CHANNEL_3;
    }

    return APP_CHANNEL_NONE;
}

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
    uint8_t active_count;
    AppChannel_t input_channel;
    AppChannel_t open_channel;

    request.channel = APP_CHANNEL_NONE;
    request.action = APP_ACTION_NONE;
    request.valid = 0U;

    if ((relay_busy != 0U) || (action_allowed == 0U) || (input_snapshot.plc_state != PLC_CONTROL_VALID))
    {
        return request;
    }

    input_channel = ChannelRequest_GetInputChannel(input_snapshot, &active_count);
    open_channel = ChannelRequest_GetOpenChannel(feedback_snapshot);

    if (active_count > 1U)
    {
        return request;
    }

    if (active_count == 0U)
    {
        if (open_channel != APP_CHANNEL_NONE)
        {
            request.channel = open_channel;
            request.action = APP_ACTION_CLOSE;
            request.valid = 1U;
        }

        return request;
    }

    if (open_channel == APP_CHANNEL_NONE)
    {
        request.channel = input_channel;
        request.action = APP_ACTION_OPEN;
        request.valid = 1U;
    }

    return request;
}
