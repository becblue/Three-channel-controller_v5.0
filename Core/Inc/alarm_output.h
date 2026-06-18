#ifndef __ALARM_OUTPUT_H__
#define __ALARM_OUTPUT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "safety_manager.h"

void AlarmOutput_Init(void);
void AlarmOutput_Task(uint32_t tick_ms, SafetySnapshot_t safety_snapshot);

#ifdef __cplusplus
}
#endif

#endif /* __ALARM_OUTPUT_H__ */
