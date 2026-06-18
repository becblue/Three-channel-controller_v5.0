#ifndef __RELAY_DRIVER_H__
#define __RELAY_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "app_types.h"

typedef struct
{
    AppChannel_t channel;
    AppAction_t action;
} RelayCommand_t;

typedef struct
{
    AppChannel_t channel;
    AppAction_t action;
    uint8_t ready_for_feedback;
} RelayDoneEvent_t;

void RelayDriver_Init(void);
void RelayDriver_Task(uint32_t tick_ms);
AppResult_t RelayDriver_Start(RelayCommand_t command, uint32_t tick_ms);
uint8_t RelayDriver_IsBusy(void);
uint8_t RelayDriver_TakeDoneEvent(RelayDoneEvent_t *event);
void RelayDriver_ForceAllOutputsInactive(void);

#ifdef __cplusplus
}
#endif

#endif /* __RELAY_DRIVER_H__ */
