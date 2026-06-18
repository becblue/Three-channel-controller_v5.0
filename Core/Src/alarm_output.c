#include "alarm_output.h"

#include "app_config.h"
#include "board_io.h"

static AppFaultSeverity_t g_last_severity;
static uint32_t g_beep_phase_start_ms;
static uint8_t g_beep_active;

static void AlarmOutput_SetBuzzer(uint8_t active)
{
    g_beep_active = active;
    BoardIO_SetBuzzerOutput(active);
}

void AlarmOutput_Init(void)
{
    g_last_severity = APP_FAULT_SEVERITY_NONE;
    g_beep_phase_start_ms = 0U;
    g_beep_active = 0U;
    BoardIO_SetAlarmOutput(0U);
    BoardIO_SetBuzzerOutput(0U);
}

void AlarmOutput_Task(uint32_t tick_ms, SafetySnapshot_t safety_snapshot)
{
    uint32_t on_ms = 0U;
    uint32_t off_ms = 0U;
    uint32_t elapsed_ms;

    BoardIO_SetAlarmOutput(safety_snapshot.any_fault_active);

    if (safety_snapshot.highest_severity != g_last_severity)
    {
        g_last_severity = safety_snapshot.highest_severity;
        g_beep_phase_start_ms = tick_ms;
        AlarmOutput_SetBuzzer((g_last_severity == APP_FAULT_SEVERITY_NONE) ? 0U : 1U);
    }

    if (safety_snapshot.highest_severity == APP_FAULT_SEVERITY_NONE)
    {
        AlarmOutput_SetBuzzer(0U);
        return;
    }

    if (safety_snapshot.highest_severity == APP_FAULT_SEVERITY_SERIOUS)
    {
        AlarmOutput_SetBuzzer(1U);
        return;
    }

    if (safety_snapshot.highest_severity == APP_FAULT_SEVERITY_URGENT)
    {
        on_ms = APP_BEEP_URGENT_ON_MS;
        off_ms = APP_BEEP_URGENT_OFF_MS;
    }
    else
    {
        on_ms = APP_BEEP_GENERAL_ON_MS;
        off_ms = APP_BEEP_GENERAL_OFF_MS;
    }

    elapsed_ms = tick_ms - g_beep_phase_start_ms;

    if ((g_beep_active != 0U) && (elapsed_ms >= on_ms))
    {
        g_beep_phase_start_ms = tick_ms;
        AlarmOutput_SetBuzzer(0U);
    }
    else if ((g_beep_active == 0U) && (elapsed_ms >= off_ms))
    {
        g_beep_phase_start_ms = tick_ms;
        AlarmOutput_SetBuzzer(1U);
    }
}
