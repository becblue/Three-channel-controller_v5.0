#include "feedback_monitor.h"

#include "board_io.h"

#define FEEDBACK_FAULT_B_BIT 0U
#define FEEDBACK_FAULT_C_BIT 1U
#define FEEDBACK_FAULT_D_BIT 2U
#define FEEDBACK_FAULT_E_BIT 3U
#define FEEDBACK_FAULT_F_BIT 4U
#define FEEDBACK_FAULT_G_BIT 5U
#define FEEDBACK_FAULT_H_BIT 6U
#define FEEDBACK_FAULT_I_BIT 7U
#define FEEDBACK_FAULT_J_BIT 8U

typedef struct
{
    uint8_t k1_1;
    uint8_t k2_1;
    uint8_t k3_1;
    uint8_t k1_2;
    uint8_t k2_2;
    uint8_t k3_2;
    uint8_t sw1;
    uint8_t sw2;
    uint8_t sw3;
} FeedbackRaw_t;

static FeedbackSnapshot_t g_feedback_snapshot;
static uint16_t g_latched_fault_mask_b_to_j;

static FeedbackRaw_t FeedbackMonitor_ReadRaw(void)
{
    FeedbackRaw_t raw;

    raw.k1_1 = BoardIO_ReadK1_1StaRaw();
    raw.k2_1 = BoardIO_ReadK2_1StaRaw();
    raw.k3_1 = BoardIO_ReadK3_1StaRaw();
    raw.k1_2 = BoardIO_ReadK1_2StaRaw();
    raw.k2_2 = BoardIO_ReadK2_2StaRaw();
    raw.k3_2 = BoardIO_ReadK3_2StaRaw();
    raw.sw1 = BoardIO_ReadSw1StaRaw();
    raw.sw2 = BoardIO_ReadSw2StaRaw();
    raw.sw3 = BoardIO_ReadSw3StaRaw();

    return raw;
}

static uint8_t FeedbackMonitor_IsAllClosed(FeedbackRaw_t raw)
{
    return ((raw.k1_1 == 0U) && (raw.k2_1 == 0U) && (raw.k3_1 == 0U) &&
            (raw.k1_2 == 0U) && (raw.k2_2 == 0U) && (raw.k3_2 == 0U) &&
            (raw.sw1 == 0U) && (raw.sw2 == 0U) && (raw.sw3 == 0U)) ? 1U : 0U;
}

static uint8_t FeedbackMonitor_IsChannelOpen(FeedbackRaw_t raw, AppChannel_t channel)
{
    if (channel == APP_CHANNEL_1)
    {
        return ((raw.k1_1 != 0U) && (raw.k1_2 != 0U) && (raw.sw1 != 0U) &&
                (raw.k2_1 == 0U) && (raw.k3_1 == 0U) &&
                (raw.k2_2 == 0U) && (raw.k3_2 == 0U) &&
                (raw.sw2 == 0U) && (raw.sw3 == 0U)) ? 1U : 0U;
    }

    if (channel == APP_CHANNEL_2)
    {
        return ((raw.k2_1 != 0U) && (raw.k2_2 != 0U) && (raw.sw2 != 0U) &&
                (raw.k1_1 == 0U) && (raw.k3_1 == 0U) &&
                (raw.k1_2 == 0U) && (raw.k3_2 == 0U) &&
                (raw.sw1 == 0U) && (raw.sw3 == 0U)) ? 1U : 0U;
    }

    if (channel == APP_CHANNEL_3)
    {
        return ((raw.k3_1 != 0U) && (raw.k3_2 != 0U) && (raw.sw3 != 0U) &&
                (raw.k1_1 == 0U) && (raw.k2_1 == 0U) &&
                (raw.k1_2 == 0U) && (raw.k2_2 == 0U) &&
                (raw.sw1 == 0U) && (raw.sw2 == 0U)) ? 1U : 0U;
    }

    return 0U;
}

static uint8_t FeedbackMonitor_GetExpectedRaw(FeedbackRaw_t raw, AppChannel_t channel)
{
    if (channel == APP_CHANNEL_1)
    {
        return (raw.k1_1 != 0U) && (raw.k1_2 != 0U) && (raw.sw1 != 0U);
    }

    if (channel == APP_CHANNEL_2)
    {
        return (raw.k2_1 != 0U) && (raw.k2_2 != 0U) && (raw.sw2 != 0U);
    }

    if (channel == APP_CHANNEL_3)
    {
        return (raw.k3_1 != 0U) && (raw.k3_2 != 0U) && (raw.sw3 != 0U);
    }

    return 0U;
}

static void FeedbackMonitor_UpdateStates(FeedbackRaw_t raw)
{
    if (FeedbackMonitor_IsAllClosed(raw) != 0U)
    {
        g_feedback_snapshot.channel_1 = APP_CHANNEL_STATE_CLOSED;
        g_feedback_snapshot.channel_2 = APP_CHANNEL_STATE_CLOSED;
        g_feedback_snapshot.channel_3 = APP_CHANNEL_STATE_CLOSED;
        g_latched_fault_mask_b_to_j = 0U;
    }
    else if (FeedbackMonitor_IsChannelOpen(raw, APP_CHANNEL_1) != 0U)
    {
        g_feedback_snapshot.channel_1 = APP_CHANNEL_STATE_OPEN;
        g_feedback_snapshot.channel_2 = APP_CHANNEL_STATE_CLOSED;
        g_feedback_snapshot.channel_3 = APP_CHANNEL_STATE_CLOSED;
        g_latched_fault_mask_b_to_j = 0U;
    }
    else if (FeedbackMonitor_IsChannelOpen(raw, APP_CHANNEL_2) != 0U)
    {
        g_feedback_snapshot.channel_1 = APP_CHANNEL_STATE_CLOSED;
        g_feedback_snapshot.channel_2 = APP_CHANNEL_STATE_OPEN;
        g_feedback_snapshot.channel_3 = APP_CHANNEL_STATE_CLOSED;
        g_latched_fault_mask_b_to_j = 0U;
    }
    else if (FeedbackMonitor_IsChannelOpen(raw, APP_CHANNEL_3) != 0U)
    {
        g_feedback_snapshot.channel_1 = APP_CHANNEL_STATE_CLOSED;
        g_feedback_snapshot.channel_2 = APP_CHANNEL_STATE_CLOSED;
        g_feedback_snapshot.channel_3 = APP_CHANNEL_STATE_OPEN;
        g_latched_fault_mask_b_to_j = 0U;
    }
    else
    {
        g_feedback_snapshot.channel_1 = APP_CHANNEL_STATE_FAULT;
        g_feedback_snapshot.channel_2 = APP_CHANNEL_STATE_FAULT;
        g_feedback_snapshot.channel_3 = APP_CHANNEL_STATE_FAULT;
    }

    g_feedback_snapshot.fault_mask_b_to_j = g_latched_fault_mask_b_to_j;
}

static void FeedbackMonitor_SetFaultIfMismatch(uint8_t actual, uint8_t expected, uint8_t bit)
{
    if (actual != expected)
    {
        g_latched_fault_mask_b_to_j |= (uint16_t)(1U << bit);
    }
}

void FeedbackMonitor_Init(void)
{
    g_feedback_snapshot.channel_1 = APP_CHANNEL_STATE_UNKNOWN;
    g_feedback_snapshot.channel_2 = APP_CHANNEL_STATE_UNKNOWN;
    g_feedback_snapshot.channel_3 = APP_CHANNEL_STATE_UNKNOWN;
    g_feedback_snapshot.fault_mask_b_to_j = 0U;
    g_latched_fault_mask_b_to_j = 0U;
}

void FeedbackMonitor_Task(uint32_t tick_ms)
{
    FeedbackRaw_t raw = FeedbackMonitor_ReadRaw();

    (void)tick_ms;
    FeedbackMonitor_UpdateStates(raw);
}

void FeedbackMonitor_HandleRelayDone(RelayDoneEvent_t event)
{
    FeedbackRaw_t raw = FeedbackMonitor_ReadRaw();
    uint8_t expected_high = (event.action == APP_ACTION_OPEN) ? 1U : 0U;

    if ((event.channel == APP_CHANNEL_NONE) || (event.action == APP_ACTION_NONE))
    {
        return;
    }

    if (event.action == APP_ACTION_CLOSE)
    {
        expected_high = 0U;
    }

    if ((event.action == APP_ACTION_OPEN) && (FeedbackMonitor_GetExpectedRaw(raw, event.channel) == 0U))
    {
        /* 继续逐点记录，下面会给出对应 B~J 位。 */
    }

    FeedbackMonitor_SetFaultIfMismatch(raw.k1_1, (event.channel == APP_CHANNEL_1) ? expected_high : 0U, FEEDBACK_FAULT_B_BIT);
    FeedbackMonitor_SetFaultIfMismatch(raw.k2_1, (event.channel == APP_CHANNEL_2) ? expected_high : 0U, FEEDBACK_FAULT_C_BIT);
    FeedbackMonitor_SetFaultIfMismatch(raw.k3_1, (event.channel == APP_CHANNEL_3) ? expected_high : 0U, FEEDBACK_FAULT_D_BIT);
    FeedbackMonitor_SetFaultIfMismatch(raw.k1_2, (event.channel == APP_CHANNEL_1) ? expected_high : 0U, FEEDBACK_FAULT_E_BIT);
    FeedbackMonitor_SetFaultIfMismatch(raw.k2_2, (event.channel == APP_CHANNEL_2) ? expected_high : 0U, FEEDBACK_FAULT_F_BIT);
    FeedbackMonitor_SetFaultIfMismatch(raw.k3_2, (event.channel == APP_CHANNEL_3) ? expected_high : 0U, FEEDBACK_FAULT_G_BIT);
    FeedbackMonitor_SetFaultIfMismatch(raw.sw1, (event.channel == APP_CHANNEL_1) ? expected_high : 0U, FEEDBACK_FAULT_H_BIT);
    FeedbackMonitor_SetFaultIfMismatch(raw.sw2, (event.channel == APP_CHANNEL_2) ? expected_high : 0U, FEEDBACK_FAULT_I_BIT);
    FeedbackMonitor_SetFaultIfMismatch(raw.sw3, (event.channel == APP_CHANNEL_3) ? expected_high : 0U, FEEDBACK_FAULT_J_BIT);

    FeedbackMonitor_UpdateStates(raw);
}

FeedbackSnapshot_t FeedbackMonitor_GetSnapshot(void)
{
    return g_feedback_snapshot;
}

AppChannel_t FeedbackMonitor_GetOpenChannel(void)
{
    if (g_feedback_snapshot.channel_1 == APP_CHANNEL_STATE_OPEN)
    {
        return APP_CHANNEL_1;
    }

    if (g_feedback_snapshot.channel_2 == APP_CHANNEL_STATE_OPEN)
    {
        return APP_CHANNEL_2;
    }

    if (g_feedback_snapshot.channel_3 == APP_CHANNEL_STATE_OPEN)
    {
        return APP_CHANNEL_3;
    }

    return APP_CHANNEL_NONE;
}
