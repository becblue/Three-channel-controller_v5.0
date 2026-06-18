#include "relay_driver.h"

#include "app_config.h"
#include "board_io.h"

typedef enum
{
    RELAY_STATE_IDLE = 0,
    RELAY_STATE_PULSE,
    RELAY_STATE_FEEDBACK_WAIT
} RelayState_t;

static uint8_t g_relay_busy;
static RelayDoneEvent_t g_done_event;
static RelayState_t g_relay_state;
static RelayCommand_t g_active_command;
static uint32_t g_state_start_ms;

static uint8_t RelayDriver_IsCommandValid(RelayCommand_t command)
{
    if ((command.channel < APP_CHANNEL_1) || (command.channel > APP_CHANNEL_3))
    {
        return 0U;
    }

    if ((command.action != APP_ACTION_OPEN) && (command.action != APP_ACTION_CLOSE))
    {
        return 0U;
    }

    return 1U;
}

static void RelayDriver_SetChannelOutput(AppChannel_t channel, AppAction_t action, uint8_t active)
{
    if (channel == APP_CHANNEL_1)
    {
        if (action == APP_ACTION_OPEN)
        {
            BoardIO_SetK1_1On(active);
            BoardIO_SetK1_2On(active);
        }
        else
        {
            BoardIO_SetK1_1Off(active);
            BoardIO_SetK1_2Off(active);
        }
    }
    else if (channel == APP_CHANNEL_2)
    {
        if (action == APP_ACTION_OPEN)
        {
            BoardIO_SetK2_1On(active);
            BoardIO_SetK2_2On(active);
        }
        else
        {
            BoardIO_SetK2_1Off(active);
            BoardIO_SetK2_2Off(active);
        }
    }
    else if (channel == APP_CHANNEL_3)
    {
        if (action == APP_ACTION_OPEN)
        {
            BoardIO_SetK3_1On(active);
            BoardIO_SetK3_2On(active);
        }
        else
        {
            BoardIO_SetK3_1Off(active);
            BoardIO_SetK3_2Off(active);
        }
    }
}

void RelayDriver_Init(void)
{
    g_relay_busy = 0U;
    g_relay_state = RELAY_STATE_IDLE;
    g_active_command.channel = APP_CHANNEL_NONE;
    g_active_command.action = APP_ACTION_NONE;
    g_state_start_ms = 0U;
    g_done_event.ready_for_feedback = 0U;
    RelayDriver_ForceAllOutputsInactive();
}

void RelayDriver_Task(uint32_t tick_ms)
{
    if (g_relay_state == RELAY_STATE_PULSE)
    {
        if ((tick_ms - g_state_start_ms) >= APP_RELAY_PULSE_MS)
        {
            RelayDriver_ForceAllOutputsInactive();
            g_state_start_ms = tick_ms;
            g_relay_state = RELAY_STATE_FEEDBACK_WAIT;
        }
    }
    else if (g_relay_state == RELAY_STATE_FEEDBACK_WAIT)
    {
        if ((tick_ms - g_state_start_ms) >= APP_RELAY_FEEDBACK_WAIT_MS)
        {
            g_done_event.channel = g_active_command.channel;
            g_done_event.action = g_active_command.action;
            g_done_event.ready_for_feedback = 1U;

            g_active_command.channel = APP_CHANNEL_NONE;
            g_active_command.action = APP_ACTION_NONE;
            g_relay_busy = 0U;
            g_relay_state = RELAY_STATE_IDLE;
        }
    }
}

AppResult_t RelayDriver_Start(RelayCommand_t command, uint32_t tick_ms)
{
    if (g_relay_busy != 0U)
    {
        return APP_RESULT_BUSY;
    }

    if (RelayDriver_IsCommandValid(command) == 0U)
    {
        return APP_RESULT_REJECTED;
    }

    RelayDriver_ForceAllOutputsInactive();
    RelayDriver_SetChannelOutput(command.channel, command.action, 1U);

    g_active_command = command;
    g_state_start_ms = tick_ms;
    g_relay_busy = 1U;
    g_relay_state = RELAY_STATE_PULSE;
    g_done_event.ready_for_feedback = 0U;

    return APP_RESULT_OK;
}

uint8_t RelayDriver_IsBusy(void)
{
    return g_relay_busy;
}

uint8_t RelayDriver_TakeDoneEvent(RelayDoneEvent_t *event)
{
    if ((event == 0) || (g_done_event.ready_for_feedback == 0U))
    {
        return 0U;
    }

    *event = g_done_event;
    g_done_event.ready_for_feedback = 0U;
    return 1U;
}

void RelayDriver_ForceAllOutputsInactive(void)
{
    BoardIO_SetK1_1On(0U);
    BoardIO_SetK1_1Off(0U);
    BoardIO_SetK1_2On(0U);
    BoardIO_SetK1_2Off(0U);
    BoardIO_SetK2_1On(0U);
    BoardIO_SetK2_1Off(0U);
    BoardIO_SetK2_2On(0U);
    BoardIO_SetK2_2Off(0U);
    BoardIO_SetK3_1On(0U);
    BoardIO_SetK3_1Off(0U);
    BoardIO_SetK3_2On(0U);
    BoardIO_SetK3_2Off(0U);
}
