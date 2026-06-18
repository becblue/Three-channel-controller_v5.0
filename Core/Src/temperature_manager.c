#include "temperature_manager.h"
#include "app_config.h"

static TemperatureSnapshot_t g_temperature_snapshot;

void TemperatureManager_Init(void)
{
    g_temperature_snapshot.ntc1_c = 0;
    g_temperature_snapshot.ntc2_c = 0;
    g_temperature_snapshot.ntc3_c = 0;
    g_temperature_snapshot.fan_pwm_percent = APP_FAN_PWM_LOW_PERCENT;
    g_temperature_snapshot.fault_k_active = 0U;
    g_temperature_snapshot.fault_l_active = 0U;
    g_temperature_snapshot.fault_m_active = 0U;
    g_temperature_snapshot.ntc_sensor_abnormal = 0U;
    g_temperature_snapshot.fan_tach_abnormal = 0U;
}

void TemperatureManager_Task(uint32_t tick_ms)
{
    (void)tick_ms;
    /* TODO: 实现 NTC 换算、温度确认、风扇 PWM 目标和诊断提示。 */
}

TemperatureSnapshot_t TemperatureManager_GetSnapshot(void)
{
    return g_temperature_snapshot;
}
