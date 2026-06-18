#include "display_manager.h"

static DisplayPage_t g_display_page;

void DisplayManager_Init(void)
{
    g_display_page = DISPLAY_PAGE_LOGO;
}

void DisplayManager_Task(uint32_t tick_ms,
                         FeedbackSnapshot_t feedback_snapshot,
                         SafetySnapshot_t safety_snapshot,
                         TemperatureSnapshot_t temperature_snapshot)
{
    (void)tick_ms;
    (void)feedback_snapshot;
    (void)safety_snapshot;
    (void)temperature_snapshot;

    switch (g_display_page)
    {
        case DISPLAY_PAGE_LOGO:
        case DISPLAY_PAGE_SELF_TEST:
        case DISPLAY_PAGE_MAIN:
        case DISPLAY_PAGE_ALARM:
        default:
            /* TODO: 实现 128x64 OLED 页面和脏区刷新。 */
            break;
    }
}

void DisplayManager_SetPage(DisplayPage_t page)
{
    g_display_page = page;
}
