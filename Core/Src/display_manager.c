#include "display_manager.h"

#include "oled_driver.h"
#include <stdio.h>
#include <string.h>

#define DISPLAY_LOGO_MS          2000U
#define DISPLAY_SELF_TEST_MS     3000U
#define DISPLAY_REFRESH_MS       250U
#define DISPLAY_ALARM_ROTATE_MS  1000U

static const uint8_t g_logo_large[27][16] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x7F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFC},
    {0x01,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFC},
    {0x07,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFC},
    {0x0F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFC},
    {0x1F,0xFF,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x3F,0xFF,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x3F,0xFF,0x80,0x01,0xFF,0x01,0xE7,0x3C,0x3C,0x0F,0x0F,0x00,0x7F,0xF8,0x7F,0xC0},
    {0x7F,0xFF,0x00,0x03,0xFF,0xC1,0xE7,0x7C,0x7C,0x0F,0x0F,0x00,0x7F,0xF8,0x7F,0xF8},
    {0x7F,0xFF,0x00,0x03,0xFF,0xE1,0xE6,0x7C,0x7C,0x0F,0x0F,0x00,0x7F,0xF8,0x7F,0xF8},
    {0x7F,0xFE,0x00,0x03,0xFF,0xE1,0xE6,0x78,0x78,0x1F,0x1F,0x00,0x7F,0xF8,0x7F,0xFC},
    {0x7F,0xFE,0x00,0x03,0xC3,0xE3,0xE6,0x78,0x78,0x1F,0x1F,0x00,0x78,0x00,0xF8,0x7C},
    {0xFF,0xFE,0x00,0x03,0xC1,0xE3,0xEE,0x78,0x78,0x1F,0x1F,0x00,0x78,0x00,0xF8,0x3C},
    {0x7F,0xFE,0x00,0x03,0xC3,0xC3,0xE0,0x78,0x78,0x1E,0x1E,0x00,0xF8,0x00,0xF8,0x3C},
    {0x7F,0xFE,0x00,0x03,0xFF,0xC3,0xC0,0xF8,0xFF,0xFE,0x1E,0x00,0xFF,0xF0,0xF0,0x7C},
    {0x7F,0xFE,0x00,0x07,0xFF,0x03,0xC0,0xF8,0xFF,0xFE,0x1E,0x00,0xFF,0xF0,0xFF,0xF8},
    {0x7F,0xFE,0x00,0x07,0xFF,0x83,0xC0,0xF8,0xFF,0xFE,0x1E,0x00,0xFF,0xE0,0xFF,0xF0},
    {0x3F,0xFF,0x00,0x07,0xC7,0xC3,0xC0,0xF0,0xFF,0xFE,0x3E,0x00,0xFB,0xE0,0xFF,0xE0},
    {0x3F,0xFF,0x80,0x07,0x83,0xC3,0xC0,0xF0,0xF0,0x3E,0x3E,0x00,0xF0,0x01,0xFF,0xC0},
    {0x1F,0xFF,0x80,0x07,0x83,0xC3,0xC1,0xF0,0xF0,0x3E,0x3E,0x00,0xF0,0x01,0xF3,0xE0},
    {0x0F,0xFF,0xE0,0x07,0x87,0xC3,0xF3,0xE0,0xF0,0x3C,0x3F,0xF9,0xFF,0xE1,0xF3,0xE0},
    {0x07,0xFF,0xF8,0x07,0xFF,0xC3,0xFF,0xE1,0xF0,0x3C,0x3F,0xF9,0xFF,0xE1,0xE1,0xF0},
    {0x01,0xFF,0xF8,0x0F,0xFF,0x81,0xFF,0xC1,0xF0,0x3C,0x3F,0xF9,0xFF,0xE1,0xE1,0xF0},
    {0x00,0x7F,0xF8,0x0F,0xFF,0x00,0xFF,0x81,0xF0,0x7C,0x3F,0xF9,0xFF,0xE1,0xE0,0xF8},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};

static const uint8_t g_logo_small[] = {
0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X1C,
0X00,0X0C,0X08,0X30,0X00,0X63,0X00,0X00,0XC0,0X00,0X00,0X00,0X00,0X1F,0X3E,0X00,
0X1F,0X0C,0X78,0X00,0X63,0X80,0X01,0XCF,0XFF,0XF1,0XFF,0XFC,0X00,0X36,0X00,0X3F,
0X0C,0X7C,0X00,0X61,0X80,0X01,0X8F,0XFF,0XF1,0XFF,0XFE,0X00,0X33,0X00,0X33,0X0C,
0X6C,0X00,0X60,0XC0,0X03,0X8C,0X00,0X01,0X80,0X07,0X1F,0X33,0X00,0X33,0X0C,0X6E,
0X00,0X60,0XC0,0X07,0X0C,0X00,0X01,0X80,0X03,0X00,0X33,0X00,0X63,0X0C,0X66,0X00,
0X60,0X60,0X06,0X0C,0X00,0X01,0X80,0X03,0X80,0X33,0X80,0X63,0X0C,0X67,0X00,0X60,
0X20,0X0E,0X0C,0X00,0X01,0X80,0X01,0X80,0X31,0X80,0X63,0X0C,0X63,0X00,0X60,0X30,
0X1C,0X0C,0X00,0X01,0X80,0X01,0X9F,0X31,0X80,0XE3,0X0C,0X63,0X80,0X60,0X38,0X18,
0X0C,0X00,0X01,0X80,0X01,0X80,0X31,0XC0,0XC3,0X0C,0X61,0X80,0X60,0X18,0X38,0X0C,
0X00,0X01,0X80,0X03,0X8E,0X30,0XC0,0XC3,0X0C,0X61,0XC0,0X60,0X1C,0X70,0X0C,0X00,
0X01,0X80,0X03,0X00,0X30,0XC0,0XC3,0X0C,0X60,0XC0,0X60,0X0E,0X60,0X0C,0X00,0X01,
0X80,0X0F,0X1F,0X30,0XC1,0X83,0X0C,0X60,0XE0,0X60,0X06,0XE0,0X0C,0XFF,0XE1,0XBF,
0XFE,0X00,0X30,0X61,0X83,0X0C,0X60,0X70,0X60,0X03,0X80,0X0C,0X00,0X01,0X80,0X00,
0X18,0X30,0X63,0X83,0X0C,0X60,0X30,0X60,0X00,0X00,0X0C,0X00,0X01,0X80,0X30,0X00,
0X30,0X73,0X03,0X0C,0X60,0X38,0X60,0X01,0X00,0X0C,0X00,0X01,0X80,0X38,0X01,0X30,
0X33,0X03,0X0C,0X60,0X18,0X60,0X01,0X80,0X0C,0X00,0X01,0X80,0X18,0X04,0X30,0X37,
0X03,0X0C,0X60,0X1C,0X60,0X01,0X80,0X0C,0X00,0X01,0X80,0X0C,0X10,0X30,0X36,0X03,
0X0C,0X60,0X0C,0X60,0X01,0X80,0X0C,0X00,0X01,0X80,0X0E,0X00,0X30,0X16,0X03,0X0C,
0X60,0X0E,0X60,0X01,0X80,0X0C,0X00,0X01,0X80,0X06,0X00,0X30,0X1E,0X03,0X0C,0X60,
0X06,0X60,0X01,0X80,0X0C,0X00,0X01,0X80,0X07,0X04,0X30,0X1C,0X03,0X0C,0X60,0X07,
0X60,0X01,0X80,0X0C,0X00,0X01,0X80,0X03,0X10,0X30,0X1C,0X03,0X0C,0X60,0X03,0XE0,
0X01,0X80,0X0F,0XFF,0XF9,0X80,0X01,0X80,0X30,0X00,0X03,0X0C,0X60,0X03,0XE0,0X01,
0X80,0X07,0XFF,0XF9,0X80,0X01,0X9C,0X20,0X00,0X02,0X08,0X20,0X00,0X00,0X01,0X00,
0X00,0X00,0X01,0X00,0X00,0X80,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
0X00,0X00,0X00,0X00,0X00
};

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
    OledDriver_DrawBitmapRows(0U, 18U, 128U, 27U, &g_logo_large[0][0]);
    OledDriver_RefreshDirty();
}

static const char *DisplayManager_GetSelfTestStepText(uint8_t percent)
{
    if (percent < 25U)
    {
        return "IO CHECK";
    }

    if (percent < 50U)
    {
        return "FB CHECK";
    }

    if (percent < 75U)
    {
        return "TEMP CHECK";
    }

    return "READY";
}

static void DisplayManager_DrawSelfTest(uint32_t tick_ms)
{
    uint32_t elapsed_ms = tick_ms - (g_display_start_tick_ms + DISPLAY_LOGO_MS);
    uint8_t percent;
    uint8_t fill_width;
    const char *step_text;

    if (elapsed_ms > DISPLAY_SELF_TEST_MS)
    {
        elapsed_ms = DISPLAY_SELF_TEST_MS;
    }

    percent = (uint8_t)((elapsed_ms * 100U) / DISPLAY_SELF_TEST_MS);
    fill_width = (uint8_t)((percent * 116U) / 100U);
    step_text = DisplayManager_GetSelfTestStepText(percent);

    OledDriver_ClearArea(0U, 0U, OLED_WIDTH, OLED_PAGE_COUNT);
    OledDriver_DrawBitmapRows(6U, 1U, 115U, 27U, g_logo_small);
    OledDriver_DrawFrame(5U, 36U, 118U, 9U);
    OledDriver_FillRect(6U, 37U, fill_width, 7U, 1U);
    OledDriver_DrawString(37U, 6U, step_text, 0U);
    OledDriver_RefreshDirty();
}

static void DisplayManager_DrawChannelBox(uint8_t index, AppChannelState_t state)
{
    static const uint8_t box_x[3] = {0U, 44U, 88U};
    uint8_t x = box_x[index];
    char title[5];
    const char *state_text = DisplayManager_GetChannelShortText(state);
    uint8_t open = (state == APP_CHANNEL_STATE_OPEN) ? 1U : 0U;
    uint8_t state_text_x = (state == APP_CHANNEL_STATE_OPEN) ? 14U : 11U;

    if (open != 0U)
    {
        OledDriver_FillRect(x, 16U, 40U, 23U, 1U);
    }
    else
    {
        OledDriver_DrawFrame(x, 16U, 40U, 23U);
    }

    (void)snprintf(title, sizeof(title), "CH%u", (uint8_t)(index + 1U));
    OledDriver_DrawString((uint8_t)(x + 11U), 2U, title, open);
    OledDriver_DrawString((uint8_t)(x + state_text_x), 3U, state_text, open);
}

static void DisplayManager_DrawTemperatureLine(TemperatureSnapshot_t temperature_snapshot)
{
    char line[24];

    (void)snprintf(line, sizeof(line), "T1:%03d T2:%03d", temperature_snapshot.ntc1_c, temperature_snapshot.ntc2_c);
    OledDriver_DrawString(0U, 5U, line, 0U);
    (void)snprintf(line, sizeof(line), "T3:%03d", temperature_snapshot.ntc3_c);
    OledDriver_DrawString(0U, 6U, line, 0U);
    (void)snprintf(line, sizeof(line), "FAN:%02u%% %04uRPM", temperature_snapshot.fan_pwm_percent, temperature_snapshot.fan_rpm);
    OledDriver_DrawString(0U, 7U, line, 0U);
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

    if (temperature_snapshot.ntc_sensor_abnormal != 0U)
    {
        OledDriver_DrawString(86U, 0U, "NTC ERR", 0U);
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
