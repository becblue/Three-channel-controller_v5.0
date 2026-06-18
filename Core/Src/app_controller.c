#include "app_controller.h"

#include "alarm_output.h"
#include "channel_request.h"
#include "display_manager.h"
#include "feedback_monitor.h"
#include "input_filter.h"
#include "maintenance_manager.h"
#include "relay_driver.h"
#include "safety_manager.h"
#include "temperature_manager.h"
#include "watchdog_manager.h"

static AppState_t g_app_state;

void AppController_Init(uint32_t tick_ms)
{
    (void)tick_ms;

    g_app_state = APP_STATE_BOOT;

    InputFilter_Init();
    ChannelRequest_Init();
    RelayDriver_Init();
    FeedbackMonitor_Init();
    SafetyManager_Init();
    TemperatureManager_Init();
    AlarmOutput_Init();
    DisplayManager_Init();
    WatchdogManager_Init();
    MaintenanceManager_Init();
}

void AppController_RunOnce(uint32_t tick_ms)
{
    InputFilter_Task(tick_ms);
    MaintenanceManager_Task(tick_ms);
    TemperatureManager_Task(tick_ms);
    RelayDriver_Task(tick_ms);
    FeedbackMonitor_Task(tick_ms);
    SafetyManager_Task(tick_ms);

    /* TODO: 按 APP_STATE_BOOT/LOGO/SELF_TEST/STANDBY/RUNNING/ALARM/ERROR 推进总状态机。 */

    AlarmOutput_Task(tick_ms, SafetyManager_GetSnapshot());
    DisplayManager_Task(tick_ms,
                        FeedbackMonitor_GetSnapshot(),
                        SafetyManager_GetSnapshot(),
                        TemperatureManager_GetSnapshot());
    WatchdogManager_Task(tick_ms);
}

AppState_t AppController_GetState(void)
{
    return g_app_state;
}
