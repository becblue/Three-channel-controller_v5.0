#ifndef __TEMPERATURE_MANAGER_H__
#define __TEMPERATURE_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct
{
    int16_t ntc1_c;
    int16_t ntc2_c;
    int16_t ntc3_c;
    uint8_t fan_pwm_percent;
    uint16_t fan_rpm;
    uint8_t fault_k_active;
    uint8_t fault_l_active;
    uint8_t fault_m_active;
    uint8_t ntc_sensor_abnormal;
    uint8_t fan_tach_abnormal;
} TemperatureSnapshot_t;

void TemperatureManager_Init(void);
void TemperatureManager_Task(uint32_t tick_ms);
TemperatureSnapshot_t TemperatureManager_GetSnapshot(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEMPERATURE_MANAGER_H__ */
