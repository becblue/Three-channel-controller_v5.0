#include "safety_manager.h"

#include "input_filter.h"

static SafetySnapshot_t g_safety_snapshot;

static AppFaultSeverity_t SafetyManager_GetFaultSeverity(AppFaultId_t fault_id)
{
    if ((fault_id == APP_FAULT_A) || (fault_id == APP_FAULT_N) || (fault_id == APP_FAULT_O))
    {
        return APP_FAULT_SEVERITY_SERIOUS;
    }

    if ((fault_id >= APP_FAULT_B) && (fault_id <= APP_FAULT_J))
    {
        return APP_FAULT_SEVERITY_URGENT;
    }

    if ((fault_id >= APP_FAULT_K) && (fault_id <= APP_FAULT_M))
    {
        return APP_FAULT_SEVERITY_GENERAL;
    }

    return APP_FAULT_SEVERITY_NONE;
}

static void SafetyManager_RecalculateSnapshot(void)
{
    uint32_t fault_index;
    AppFaultSeverity_t highest = APP_FAULT_SEVERITY_NONE;
    uint8_t relay_allowed = 1U;

    for (fault_index = 0U; fault_index < (uint32_t)APP_FAULT_COUNT; fault_index++)
    {
        if ((g_safety_snapshot.active_fault_bits & (1UL << fault_index)) != 0U)
        {
            AppFaultSeverity_t severity = SafetyManager_GetFaultSeverity((AppFaultId_t)fault_index);

            if (severity > highest)
            {
                highest = severity;
            }

            if ((severity == APP_FAULT_SEVERITY_SERIOUS) || (severity == APP_FAULT_SEVERITY_URGENT))
            {
                relay_allowed = 0U;
            }
        }
    }

    g_safety_snapshot.highest_severity = highest;
    g_safety_snapshot.any_fault_active = (g_safety_snapshot.active_fault_bits != 0U) ? 1U : 0U;
    g_safety_snapshot.relay_action_allowed = relay_allowed;
}

void SafetyManager_Init(void)
{
    g_safety_snapshot.active_fault_bits = 0U;
    g_safety_snapshot.highest_severity = APP_FAULT_SEVERITY_NONE;
    g_safety_snapshot.any_fault_active = 0U;
    g_safety_snapshot.relay_action_allowed = 1U;
}

void SafetyManager_Task(uint32_t tick_ms)
{
    InputFilterSnapshot_t input_snapshot = InputFilter_GetSnapshot();

    (void)tick_ms;

    if (input_snapshot.plc_state == PLC_CONTROL_LOST)
    {
        SafetyManager_SetFault(APP_FAULT_O);
    }
    else if (input_snapshot.plc_state == PLC_CONTROL_VALID)
    {
        SafetyManager_ClearFault(APP_FAULT_O);
    }

    SafetyManager_RecalculateSnapshot();
}

void SafetyManager_SetFault(AppFaultId_t fault_id)
{
    if (fault_id < APP_FAULT_COUNT)
    {
        g_safety_snapshot.active_fault_bits |= (1UL << (uint32_t)fault_id);
        SafetyManager_RecalculateSnapshot();
    }
}

void SafetyManager_ClearFault(AppFaultId_t fault_id)
{
    if (fault_id < APP_FAULT_COUNT)
    {
        g_safety_snapshot.active_fault_bits &= ~(1UL << (uint32_t)fault_id);
        SafetyManager_RecalculateSnapshot();
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
