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
    SafetySnapshot_t safety_snapshot;
    RelayDoneEvent_t relay_done_event;
    ChannelRequest_t channel_request;
    RelayCommand_t relay_command;

    InputFilter_Task(tick_ms);
    WatchdogManager_MarkTaskAlive(WATCHDOG_TASK_INPUT);

    MaintenanceManager_Task(tick_ms);
    TemperatureManager_Task(tick_ms);
    RelayDriver_Task(tick_ms);
    WatchdogManager_MarkTaskAlive(WATCHDOG_TASK_RELAY);

    if (RelayDriver_TakeDoneEvent(&relay_done_event) != 0U)
    {
        FeedbackMonitor_HandleRelayDone(relay_done_event);
    }

    FeedbackMonitor_Task(tick_ms);
    WatchdogManager_MarkTaskAlive(WATCHDOG_TASK_FEEDBACK);

    SafetyManager_Task(tick_ms);
    WatchdogManager_MarkTaskAlive(WATCHDOG_TASK_SAFETY);

    safety_snapshot = SafetyManager_GetSnapshot();

    channel_request = ChannelRequest_Evaluate(InputFilter_GetSnapshot(),
                                              FeedbackMonitor_GetSnapshot(),
                                              RelayDriver_IsBusy(),
                                              safety_snapshot.relay_action_allowed);
    if (channel_request.valid != 0U)
    {
        relay_command.channel = channel_request.channel;
        relay_command.action = channel_request.action;
        (void)RelayDriver_Start(relay_command, tick_ms);
    }

    g_app_state = (safety_snapshot.any_fault_active != 0U) ? APP_STATE_ALARM : APP_STATE_STANDBY;

    AlarmOutput_Task(tick_ms, safety_snapshot);
    DisplayManager_Task(tick_ms,
                        FeedbackMonitor_GetSnapshot(),
                        safety_snapshot,
                        TemperatureManager_GetSnapshot());
    WatchdogManager_MarkTaskAlive(WATCHDOG_TASK_OUTPUT);

    WatchdogManager_Task(tick_ms);
}

AppState_t AppController_GetState(void)
{
    return g_app_state;
}
