#include "temperature_manager.h"

#include "app_config.h"
#include "board_io.h"

#define TEMP_CHANNEL_COUNT              3U
#define NTC_ADC_MAX                     4095U
#define NTC_ADC_OPEN_THRESHOLD          4080U
#define NTC_ADC_SHORT_THRESHOLD         15U
#define NTC_TEMP_INVALID_C              999
#define NTC_TABLE_SIZE                  ((uint8_t)(sizeof(g_ntc_table) / sizeof(g_ntc_table[0])))

typedef struct
{
    int16_t temp_c;
    uint16_t resistance_x100_kohm;
} NtcTablePoint_t;

typedef struct
{
    uint8_t set_count;
    uint8_t clear_count;
    uint8_t active;
} TempConfirmState_t;

/* RT 表来自 DOC/NTC热敏电阻RT值.csv，阻值单位为 kΩ*100。 */
static const NtcTablePoint_t g_ntc_table[] = {
    {-40,19739},{-39,18654},{-38,17635},{-37,16680},{-36,15782},{-35,14939},{-34,14151},{-33,13409},{-32,12711},{-31,12053},
    {-30,11434},{-29,10853},{-28,10304},{-27,9787},{-26,9299},{-25,8838},{-24,8404},{-23,7993},{-22,7605},{-21,7238},
    {-20,6892},{-19,6563},{-18,6253},{-17,5959},{-16,5680},{-15,5417},{-14,5167},{-13,4929},{-12,4705},{-11,4491},
    {-10,4289},{-9,4097},{-8,3914},{-7,3741},{-6,3576},{-5,3420},{-4,3271},{-3,3129},{-2,2995},{-1,2866},
    {0,2745},{1,2628},{2,2518},{3,2412},{4,2312},{5,2217},{6,2125},{7,2038},{8,1956},{9,1876},
    {10,1801},{11,1729},{12,1660},{13,1595},{14,1532},{15,1472},{16,1415},{17,1360},{18,1308},{19,1258},
    {20,1210},{21,1164},{22,1120},{23,1079},{24,1038},{25,1000},{26,963},{27,928},{28,894},{29,862},
    {30,831},{31,801},{32,773},{33,745},{34,719},{35,694},{36,670},{37,647},{38,624},{39,603},
    {40,582},{41,563},{42,544},{43,526},{44,508},{45,491},{46,475},{47,459},{48,444},{49,430},
    {50,416},{51,403},{52,390},{53,377},{54,365},{55,354},{56,343},{57,332},{58,322},{59,312},
    {60,302},{61,293},{62,284},{63,276},{64,267},{65,259},{66,252},{67,244},{68,237},{69,230},
    {70,223},{71,217},{72,211},{73,204},{74,199},{75,193},{76,187},{77,182},{78,177},{79,172},
    {80,167},{81,163},{82,158},{83,154},{84,150},{85,146},{86,142},{87,138},{88,134},{89,130},
    {90,127},{91,124},{92,120},{93,117},{94,114},{95,111},{96,108},{97,106},{98,103},{99,100},
    {100,98},{101,95},{102,93},{103,90},{104,88},{105,86},{106,84},{107,82},{108,80},{109,78},
    {110,76},{111,74},{112,72},{113,71},{114,69},{115,67},{116,66},{117,64},{118,63},{119,61},
    {120,60},{121,58},{122,57},{123,56},{124,55},{125,53}
};

static TemperatureSnapshot_t g_temperature_snapshot;
static TempConfirmState_t g_temp_confirm[TEMP_CHANNEL_COUNT];
static uint32_t g_last_sample_tick_ms;
static uint8_t g_fan_high_latched;

static uint16_t TemperatureManager_ReadAdcRaw(uint8_t channel_index)
{
    if (channel_index == 0U)
    {
        return BoardIO_ReadNtc1AdcRaw();
    }

    if (channel_index == 1U)
    {
        return BoardIO_ReadNtc2AdcRaw();
    }

    return BoardIO_ReadNtc3AdcRaw();
}

static int16_t TemperatureManager_AdcToTempC(uint16_t adc_raw, uint8_t *sensor_abnormal)
{
    uint32_t resistance_x100_kohm;
    uint8_t index;

    *sensor_abnormal = 0U;

    if ((adc_raw <= NTC_ADC_SHORT_THRESHOLD) || (adc_raw >= NTC_ADC_OPEN_THRESHOLD))
    {
        *sensor_abnormal = 1U;
        return NTC_TEMP_INVALID_C;
    }

    resistance_x100_kohm = ((uint32_t)1000U * (uint32_t)adc_raw) / ((uint32_t)NTC_ADC_MAX - (uint32_t)adc_raw);

    if (resistance_x100_kohm >= g_ntc_table[0].resistance_x100_kohm)
    {
        return g_ntc_table[0].temp_c;
    }

    for (index = 1U; index < NTC_TABLE_SIZE; index++)
    {
        if (resistance_x100_kohm >= g_ntc_table[index].resistance_x100_kohm)
        {
            int32_t temp_high = g_ntc_table[index - 1U].temp_c;
            int32_t temp_low = g_ntc_table[index].temp_c;
            int32_t res_high = g_ntc_table[index - 1U].resistance_x100_kohm;
            int32_t res_low = g_ntc_table[index].resistance_x100_kohm;
            int32_t temp_c = temp_high + (((int32_t)resistance_x100_kohm - res_high) * (temp_low - temp_high)) / (res_low - res_high);

            return (int16_t)temp_c;
        }
    }

    return g_ntc_table[NTC_TABLE_SIZE - 1U].temp_c;
}

static void TemperatureManager_UpdateConfirm(uint8_t channel_index, uint8_t set_condition, uint8_t clear_condition)
{
    TempConfirmState_t *confirm = &g_temp_confirm[channel_index];

    if (set_condition != 0U)
    {
        if (confirm->set_count < APP_TEMP_CONFIRM_COUNT)
        {
            confirm->set_count++;
        }
        confirm->clear_count = 0U;
    }
    else if (clear_condition != 0U)
    {
        if (confirm->clear_count < APP_TEMP_CONFIRM_COUNT)
        {
            confirm->clear_count++;
        }
        confirm->set_count = 0U;
    }
    else
    {
        confirm->set_count = 0U;
        confirm->clear_count = 0U;
    }

    if (confirm->set_count >= APP_TEMP_CONFIRM_COUNT)
    {
        confirm->active = 1U;
    }

    if (confirm->clear_count >= APP_TEMP_CONFIRM_COUNT)
    {
        confirm->active = 0U;
    }
}

static void TemperatureManager_SetFaultByChannel(uint8_t channel_index, uint8_t active)
{
    if (channel_index == 0U)
    {
        g_temperature_snapshot.fault_k_active = active;
    }
    else if (channel_index == 1U)
    {
        g_temperature_snapshot.fault_l_active = active;
    }
    else
    {
        g_temperature_snapshot.fault_m_active = active;
    }
}

static void TemperatureManager_SetTempByChannel(uint8_t channel_index, int16_t temp_c)
{
    if (channel_index == 0U)
    {
        g_temperature_snapshot.ntc1_c = temp_c;
    }
    else if (channel_index == 1U)
    {
        g_temperature_snapshot.ntc2_c = temp_c;
    }
    else
    {
        g_temperature_snapshot.ntc3_c = temp_c;
    }
}

void TemperatureManager_Init(void)
{
    uint8_t index;

    g_temperature_snapshot.ntc1_c = 0;
    g_temperature_snapshot.ntc2_c = 0;
    g_temperature_snapshot.ntc3_c = 0;
    g_temperature_snapshot.fan_pwm_percent = APP_FAN_PWM_LOW_PERCENT;
    g_temperature_snapshot.fault_k_active = 0U;
    g_temperature_snapshot.fault_l_active = 0U;
    g_temperature_snapshot.fault_m_active = 0U;
    g_temperature_snapshot.ntc_sensor_abnormal = 0U;
    g_temperature_snapshot.fan_tach_abnormal = 0U;
    g_last_sample_tick_ms = 0U;
    g_fan_high_latched = 0U;

    for (index = 0U; index < TEMP_CHANNEL_COUNT; index++)
    {
        g_temp_confirm[index].set_count = 0U;
        g_temp_confirm[index].clear_count = 0U;
        g_temp_confirm[index].active = 0U;
    }
}

void TemperatureManager_Task(uint32_t tick_ms)
{
    uint8_t index;
    uint8_t any_sensor_abnormal = 0U;
    uint8_t max_temp_valid = 0U;
    int16_t max_temp_c = -100;

    if ((tick_ms - g_last_sample_tick_ms) < APP_TEMP_CONFIRM_PERIOD_MS)
    {
        return;
    }

    g_last_sample_tick_ms = tick_ms;

    for (index = 0U; index < TEMP_CHANNEL_COUNT; index++)
    {
        uint8_t sensor_abnormal;
        int16_t temp_c = TemperatureManager_AdcToTempC(TemperatureManager_ReadAdcRaw(index), &sensor_abnormal);
        uint8_t set_condition;
        uint8_t clear_condition;

        TemperatureManager_SetTempByChannel(index, temp_c);

        if (sensor_abnormal != 0U)
        {
            any_sensor_abnormal = 1U;
        }
        else
        {
            if ((max_temp_valid == 0U) || (temp_c > max_temp_c))
            {
                max_temp_c = temp_c;
                max_temp_valid = 1U;
            }
        }

        set_condition = ((sensor_abnormal != 0U) || (temp_c >= APP_TEMP_ALARM_SET_C)) ? 1U : 0U;
        clear_condition = ((sensor_abnormal == 0U) && (temp_c <= APP_TEMP_ALARM_CLEAR_C)) ? 1U : 0U;
        TemperatureManager_UpdateConfirm(index, set_condition, clear_condition);
        TemperatureManager_SetFaultByChannel(index, g_temp_confirm[index].active);
    }

    g_temperature_snapshot.ntc_sensor_abnormal = any_sensor_abnormal;

    if (any_sensor_abnormal != 0U)
    {
        g_fan_high_latched = 1U;
    }
    else if ((max_temp_valid != 0U) && (max_temp_c >= APP_TEMP_FAN_HIGH_ON_C))
    {
        g_fan_high_latched = 1U;
    }
    else if ((max_temp_valid != 0U) && (max_temp_c <= APP_TEMP_FAN_LOW_ON_C))
    {
        g_fan_high_latched = 0U;
    }

    g_temperature_snapshot.fan_pwm_percent = (g_fan_high_latched != 0U) ? APP_FAN_PWM_HIGH_PERCENT : APP_FAN_PWM_LOW_PERCENT;
    BoardIO_SetFanPwmPercent(g_temperature_snapshot.fan_pwm_percent);
}

TemperatureSnapshot_t TemperatureManager_GetSnapshot(void)
{
    return g_temperature_snapshot;
}
