#ifndef __CHANNEL_REQUEST_H__
#define __CHANNEL_REQUEST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "app_types.h"
#include "input_filter.h"
#include "feedback_monitor.h"

typedef struct
{
    AppChannel_t channel;
    AppAction_t action;
    uint8_t valid;
} ChannelRequest_t;

void ChannelRequest_Init(void);
ChannelRequest_t ChannelRequest_Evaluate(InputFilterSnapshot_t input_snapshot,
                                         FeedbackSnapshot_t feedback_snapshot,
                                         uint8_t relay_busy,
                                         uint8_t action_allowed);

#ifdef __cplusplus
}
#endif

#endif /* __CHANNEL_REQUEST_H__ */
