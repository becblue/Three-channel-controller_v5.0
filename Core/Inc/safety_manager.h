#ifndef __SAFETY_MANAGER_H__
#define __SAFETY_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "app_types.h"

typedef struct
{
    uint32_t active_fault_bits;
    AppFaultSeverity_t highest_severity;
    uint8_t any_fault_active;
    uint8_t relay_action_allowed;
} SafetySnapshot_t;

void SafetyManager_Init(void);
void SafetyManager_Task(uint32_t tick_ms);
void SafetyManager_SetFault(AppFaultId_t fault_id);
void SafetyManager_ClearFault(AppFaultId_t fault_id);
uint8_t SafetyManager_IsFaultActive(AppFaultId_t fault_id);
SafetySnapshot_t SafetyManager_GetSnapshot(void);

#ifdef __cplusplus
}
#endif

#endif /* __SAFETY_MANAGER_H__ */
