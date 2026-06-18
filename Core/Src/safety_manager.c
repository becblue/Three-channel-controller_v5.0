#include "safety_manager.h"

static SafetySnapshot_t g_safety_snapshot;

void SafetyManager_Init(void)
{
    g_safety_snapshot.active_fault_bits = 0U;
    g_safety_snapshot.highest_severity = APP_FAULT_SEVERITY_NONE;
    g_safety_snapshot.any_fault_active = 0U;
    g_safety_snapshot.relay_action_allowed = 1U;
}

void SafetyManager_Task(uint32_t tick_ms)
{
    (void)tick_ms;
    /* TODO: 汇总各模块异常依据，计算最高等级和继电器动作许可。 */
}

void SafetyManager_SetFault(AppFaultId_t fault_id)
{
    if (fault_id < APP_FAULT_COUNT)
    {
        g_safety_snapshot.active_fault_bits |= (1UL << (uint32_t)fault_id);
        g_safety_snapshot.any_fault_active = 1U;
    }
}

void SafetyManager_ClearFault(AppFaultId_t fault_id)
{
    if (fault_id < APP_FAULT_COUNT)
    {
        g_safety_snapshot.active_fault_bits &= ~(1UL << (uint32_t)fault_id);
        g_safety_snapshot.any_fault_active = (g_safety_snapshot.active_fault_bits != 0U) ? 1U : 0U;
    }
}

uint8_t SafetyManager_IsFaultActive(AppFaultId_t fault_id)
{
    if (fault_id >= APP_FAULT_COUNT)
    {
        return 0U;
    }

    return ((g_safety_snapshot.active_fault_bits & (1UL << (uint32_t)fault_id)) != 0U) ? 1U : 0U;
}

SafetySnapshot_t SafetyManager_GetSnapshot(void)
{
    return g_safety_snapshot;
}
