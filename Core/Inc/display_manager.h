#ifndef __DISPLAY_MANAGER_H__
#define __DISPLAY_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "feedback_monitor.h"
#include "safety_manager.h"
#include "temperature_manager.h"

typedef enum
{
    DISPLAY_PAGE_LOGO = 0,
    DISPLAY_PAGE_SELF_TEST,
    DISPLAY_PAGE_MAIN,
    DISPLAY_PAGE_ALARM
} DisplayPage_t;

void DisplayManager_Init(void);
void DisplayManager_Task(uint32_t tick_ms,
                         FeedbackSnapshot_t feedback_snapshot,
                         SafetySnapshot_t safety_snapshot,
                         TemperatureSnapshot_t temperature_snapshot);
void DisplayManager_SetPage(DisplayPage_t page);

#ifdef __cplusplus
}
#endif

#endif /* __DISPLAY_MANAGER_H__ */
