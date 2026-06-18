#include "display_manager.h"

#include "oled_driver.h"
#include <stdio.h>
#include <string.h>

#define DISPLAY_LOGO_MS          2000U
#define DISPLAY_SELF_TEST_MS     3000U
#define DISPLAY_REFRESH_MS       250U
#define DISPLAY_ALARM_ROTATE_MS  1000U

static DisplayPage_t g_display_page;
static uint32_t g_display_start_tick_ms;
static uint32_t g_last_refresh_tick_ms;
static uint32_t g_last_alarm_rotate_tick_ms;
static uint8_t g_alarm_rotate_index;
static uint8_t g_last_render_key;

static AppChannelState_t DisplayManager_GetChannelState(FeedbackSnapshot_t feedback_snapshot, AppChannel_t channel)
{
    if (channel == APP_CHANNEL_1)
    {
        return feedback_snapshot.channel_1;
    }

    if (channel == APP_CHANNEL_2)
    {
        return feedback_snapshot.channel_2;
    }

    if (channel == APP_CHANNEL_3)
    {
        return feedback_snapshot.channel_3;
    }

    return APP_CHANNEL_STATE_UNKNOWN;
}

static const char *DisplayManager_GetChannelShortText(AppChannelState_t state)
{
    if (state == APP_CHANNEL_STATE_OPEN)
    {
        return "ON";
    }

    if (state == APP_CHANNEL_STATE_CLOSED)
    {
        return "OFF";
    }

    if (state == APP_CHANNEL_STATE_FAULT)
    {
        return "FLT";
    }

    return "UNK";
}

static uint8_t DisplayManager_GetFaultList(SafetySnapshot_t safety_snapshot, AppFaultId_t fault_list[APP_FAULT_COUNT])
{
    uint8_t count = 0U;
    uint8_t index;

    for (index = 0U; index < (uint8_t)APP_FAULT_COUNT; index++)
    {
        if ((safety_snapshot.active_fault_bits & (1UL << index)) != 0U)
        {
            fault_list[count] = (AppFaultId_t)index;
            count++;
        }
    }

    return count;
}

static char DisplayManager_GetFaultChar(AppFaultId_t fault_id)
{
    return (char)('A' + (uint8_t)fault_id);
}

static AppFaultSeverity_t DisplayManager_GetFaultSeverity(AppFaultId_t fault_id)
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

static AppFaultId_t DisplayManager_SelectFault(uint32_t tick_ms, SafetySnapshot_t safety_snapshot)
{
    AppFaultId_t fault_list[APP_FAULT_COUNT];
    AppFaultSeverity_t target_severity = safety_snapshot.highest_severity;
    uint8_t same_level_count = 0U;
    uint8_t count;
    uint8_t index;

    count = DisplayManager_GetFaultList(safety_snapshot, fault_list);

    if (count == 0U)
    {
        g_alarm_rotate_index = 0U;
        return APP_FAULT_COUNT;
    }

    if ((tick_ms - g_last_alarm_rotate_tick_ms) >= DISPLAY_ALARM_ROTATE_MS)
    {
        g_last_alarm_rotate_tick_ms = tick_ms;
        g_alarm_rotate_index++;
    }

    for (index = 0U; index < count; index++)
    {
        if (DisplayManager_GetFaultSeverity(fault_list[index]) == target_severity)
        {
            if (same_level_count == (uint8_t)(g_alarm_rotate_index % count))
            {
                return fault_list[index];
            }

            same_level_count++;
        }
    }

    for (index = 0U; index < count; index++)
    {
        if (DisplayManager_GetFaultSeverity(fault_list[index]) == target_severity)
        {
            return fault_list[index];
        }
    }

    return fault_list[0];
}

static void DisplayManager_DrawLogo(void)
{
    OledDriver_ClearArea(0U, 0U, OLED_WIDTH, OLED_PAGE_COUNT);
    OledDriver_DrawString(46U, 2U, "MINYER", 0U);
    OledDriver_DrawString(25U, 4U, "3CH CTRL V5", 0U);
    OledDriver_RefreshDirty();
}

static void DisplayManager_DrawSelfTest(uint32_t tick_ms)
{
    uint32_t elapsed_ms = tick_ms - (g_display_start_tick_ms + DISPLAY_LOGO_MS);
    uint8_t percent;
    uint8_t fill_width;
    char line[20];

    if (elapsed_ms > DISPLAY_SELF_TEST_MS)
    {
        elapsed_ms = DISPLAY_SELF_TEST_MS;
    }

    percent = (uint8_t)((elapsed_ms * 100U) / DISPLAY_SELF_TEST_MS);
    fill_width = (uint8_t)((percent * 116U) / 100U);

    OledDriver_ClearArea(0U, 0U, OLED_WIDTH, OLED_PAGE_COUNT);
    OledDriver_DrawString(46U, 0U, "MINYER", 0U);
    OledDriver_DrawString(30U, 2U, "SELF TEST", 0U);
    OledDriver_DrawFrame(5U, 38U, 118U, 10U);
    OledDriver_FillRect(6U, 39U, fill_width, 8U, 1U);
    (void)snprintf(line, sizeof(line), "STEP %03u", percent);
    OledDriver_DrawString(37U, 6U, line, 0U);
    OledDriver_RefreshDirty();
}

static void DisplayManager_DrawChannelBox(uint8_t index, AppChannelState_t state)
{
    uint8_t x = (uint8_t)(index * 42U);
    char title[5];
    const char *state_text = DisplayManager_GetChannelShortText(state);
    uint8_t invert = (state == APP_CHANNEL_STATE_OPEN) ? 1U : 0U;

    OledDriver_DrawFrame(x, 16U, 42U, 18U);
    (void)snprintf(title, sizeof(title), "CH%u", (uint8_t)(index + 1U));
    OledDriver_DrawString((uint8_t)(x + 10U), 2U, title, invert);
    OledDriver_DrawString((uint8_t)(x + 8U), 3U, state_text, invert);
}

static void DisplayManager_DrawTemperatureLine(TemperatureSnapshot_t temperature_snapshot)
{
    char line[24];

    if (temperature_snapshot.ntc_sensor_abnormal != 0U)
    {
        (void)snprintf(line, sizeof(line), "T1:%03d T2:%03d", temperature_snapshot.ntc1_c, temperature_snapshot.ntc2_c);
        OledDriver_DrawString(0U, 5U, line, 0U);
        (void)snprintf(line, sizeof(line), "T3:%03d FAN:%02u%%", temperature_snapshot.ntc3_c, temperature_snapshot.fan_pwm_percent);
        OledDriver_DrawString(0U, 6U, line, 0U);
        OledDriver_DrawString(0U, 7U, "NTC ERR", 0U);
    }
    else
    {
        (void)snprintf(line, sizeof(line), "T1:%02d T2:%02d T3:%02d", temperature_snapshot.ntc1_c, temperature_snapshot.ntc2_c, temperature_snapshot.ntc3_c);
        OledDriver_DrawString(0U, 5U, line, 0U);
        (void)snprintf(line, sizeof(line), "FAN:%02u%%", temperature_snapshot.fan_pwm_percent);
        OledDriver_DrawString(0U, 7U, line, 0U);
    }
}

static void DisplayManager_DrawMain(uint32_t tick_ms,
                                    FeedbackSnapshot_t feedback_snapshot,
                                    SafetySnapshot_t safety_snapshot,
                                    TemperatureSnapshot_t temperature_snapshot)
{
    AppFaultId_t shown_fault;
    char alarm_line[20];

    OledDriver_ClearArea(0U, 0U, OLED_WIDTH, OLED_PAGE_COUNT);

    if (safety_snapshot.any_fault_active != 0U)
    {
        shown_fault = DisplayManager_SelectFault(tick_ms, safety_snapshot);
        (void)snprintf(alarm_line, sizeof(alarm_line), "FLT:%c  LV:%u", DisplayManager_GetFaultChar(shown_fault), (uint8_t)safety_snapshot.highest_severity);
        OledDriver_DrawString(0U, 0U, alarm_line, 1U);
    }
    else
    {
        OledDriver_DrawString(0U, 0U, "SYS OK", 0U);
    }

    DisplayManager_DrawChannelBox(0U, DisplayManager_GetChannelState(feedback_snapshot, APP_CHANNEL_1));
    DisplayManager_DrawChannelBox(1U, DisplayManager_GetChannelState(feedback_snapshot, APP_CHANNEL_2));
    DisplayManager_DrawChannelBox(2U, DisplayManager_GetChannelState(feedback_snapshot, APP_CHANNEL_3));
    DisplayManager_DrawTemperatureLine(temperature_snapshot);
    OledDriver_RefreshDirty();
}

void DisplayManager_Init(void)
{
    g_display_page = DISPLAY_PAGE_LOGO;
    g_display_start_tick_ms = 0U;
    g_last_refresh_tick_ms = 0U;
    g_last_alarm_rotate_tick_ms = 0U;
    g_alarm_rotate_index = 0U;
    g_last_render_key = 255U;
    OledDriver_Init();
}

void DisplayManager_Task(uint32_t tick_ms,
                         FeedbackSnapshot_t feedback_snapshot,
                         SafetySnapshot_t safety_snapshot,
                         TemperatureSnapshot_t temperature_snapshot)
{
    uint32_t elapsed_ms;
    uint8_t render_key;

    if (g_display_start_tick_ms == 0U)
    {
        g_display_start_tick_ms = tick_ms;
    }

    elapsed_ms = tick_ms - g_display_start_tick_ms;

    if (elapsed_ms < DISPLAY_LOGO_MS)
    {
        g_display_page = DISPLAY_PAGE_LOGO;
    }
    else if (elapsed_ms < (DISPLAY_LOGO_MS + DISPLAY_SELF_TEST_MS))
    {
        g_display_page = DISPLAY_PAGE_SELF_TEST;
    }
    else if (safety_snapshot.any_fault_active != 0U)
    {
        g_display_page = DISPLAY_PAGE_ALARM;
    }
    else
    {
        g_display_page = DISPLAY_PAGE_MAIN;
    }

    render_key = (uint8_t)g_display_page;

    if (((tick_ms - g_last_refresh_tick_ms) < DISPLAY_REFRESH_MS) && (render_key == g_last_render_key))
    {
        return;
    }

    g_last_refresh_tick_ms = tick_ms;
    g_last_render_key = render_key;

    switch (g_display_page)
    {
        case DISPLAY_PAGE_LOGO:
            DisplayManager_DrawLogo();
            break;

        case DISPLAY_PAGE_SELF_TEST:
            DisplayManager_DrawSelfTest(tick_ms);
            break;

        case DISPLAY_PAGE_MAIN:
        case DISPLAY_PAGE_ALARM:
            DisplayManager_DrawMain(tick_ms, feedback_snapshot, safety_snapshot, temperature_snapshot);
            break;

        default:
            break;
    }
}

void DisplayManager_SetPage(DisplayPage_t page)
{
    g_display_page = page;
}
