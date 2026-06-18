#ifndef __FEEDBACK_MONITOR_H__
#define __FEEDBACK_MONITOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "app_types.h"
#include "relay_driver.h"

typedef struct
{
    AppChannelState_t channel_1;
    AppChannelState_t channel_2;
    AppChannelState_t channel_3;
    uint16_t fault_mask_b_to_j;
} FeedbackSnapshot_t;

void FeedbackMonitor_Init(void);
void FeedbackMonitor_Task(uint32_t tick_ms);
void FeedbackMonitor_HandleRelayDone(RelayDoneEvent_t event);
FeedbackSnapshot_t FeedbackMonitor_GetSnapshot(void);
AppChannel_t FeedbackMonitor_GetOpenChannel(void);

#ifdef __cplusplus
}
#endif

#endif /* __FEEDBACK_MONITOR_H__ */
