#include "input_filter.h"

#include "app_config.h"
#include "board_io.h"

typedef struct
{
    uint8_t stable_active;
    uint8_t candidate_active;
    uint8_t confirm_count;
} KenFilter_t;

static InputFilterSnapshot_t g_input_snapshot;
static KenFilter_t g_ken_filters[3];
static uint32_t g_dc_last_sample_ms;
static uint32_t g_ken_last_sample_ms;
static uint8_t g_dc_candidate_lost;
static uint8_t g_dc_confirm_count;

static uint8_t InputFilter_ReadKenActive(uint8_t index)
{
    uint8_t raw_level = 1U;

    if (index == 0U)
    {
        raw_level = BoardIO_ReadK1EnRaw();
    }
    else if (index == 1U)
    {
        raw_level = BoardIO_ReadK2EnRaw();
    }
    else
    {
        raw_level = BoardIO_ReadK3EnRaw();
    }

    return (raw_level == 0U) ? 1U : 0U;
}

static void InputFilter_UpdateSnapshotFromKenFilters(void)
{
    g_input_snapshot.k1_en_active = g_ken_filters[0].stable_active;
    g_input_snapshot.k2_en_active = g_ken_filters[1].stable_active;
    g_input_snapshot.k3_en_active = g_ken_filters[2].stable_active;

    if ((g_ken_filters[0].confirm_count >= APP_KEN_CONFIRM_COUNT) &&
        (g_ken_filters[1].confirm_count >= APP_KEN_CONFIRM_COUNT) &&
        (g_ken_filters[2].confirm_count >= APP_KEN_CONFIRM_COUNT))
    {
        g_input_snapshot.initial_sample_ready = 1U;
    }
}

static void InputFilter_UpdateKenOne(KenFilter_t *filter, uint8_t raw_active)
{
    if (raw_active != filter->candidate_active)
    {
        filter->candidate_active = raw_active;
        filter->confirm_count = 1U;
        return;
    }

    if (filter->confirm_count < APP_KEN_CONFIRM_COUNT)
    {
        filter->confirm_count++;
    }

    if (filter->confirm_count >= APP_KEN_CONFIRM_COUNT)
    {
        filter->stable_active = filter->candidate_active;
    }
}

void InputFilter_Init(void)
{
    uint8_t index;

    g_input_snapshot.k1_en_active = 0U;
    g_input_snapshot.k2_en_active = 0U;
    g_input_snapshot.k3_en_active = 0U;
    g_input_snapshot.initial_sample_ready = 0U;
    g_input_snapshot.plc_state = (BoardIO_ReadDcCtrlRaw() != 0U) ? PLC_CONTROL_SUSPECT_LOST : PLC_CONTROL_VALID;

    for (index = 0U; index < 3U; index++)
    {
        g_ken_filters[index].stable_active = 0U;
        g_ken_filters[index].candidate_active = InputFilter_ReadKenActive(index);
        g_ken_filters[index].confirm_count = 0U;
    }

    g_dc_last_sample_ms = 0U;
    g_ken_last_sample_ms = 0U;
    g_dc_candidate_lost = (BoardIO_ReadDcCtrlRaw() != 0U) ? 1U : 0U;
    g_dc_confirm_count = 0U;
}

void InputFilter_Task(uint32_t tick_ms)
{
    uint8_t dc_lost_raw = (BoardIO_ReadDcCtrlRaw() != 0U) ? 1U : 0U;

    if ((dc_lost_raw != 0U) && (g_input_snapshot.plc_state == PLC_CONTROL_VALID))
    {
        g_input_snapshot.plc_state = PLC_CONTROL_SUSPECT_LOST;
        g_dc_candidate_lost = 1U;
        g_dc_confirm_count = 1U;
        g_dc_last_sample_ms = tick_ms;
    }

    if ((tick_ms - g_dc_last_sample_ms) >= APP_DC_CONFIRM_PERIOD_MS)
    {
        g_dc_last_sample_ms = tick_ms;

        if (dc_lost_raw != g_dc_candidate_lost)
        {
            g_dc_candidate_lost = dc_lost_raw;
            g_dc_confirm_count = 1U;
        }
        else if (g_dc_confirm_count < APP_DC_CONFIRM_COUNT)
        {
            g_dc_confirm_count++;
        }

        if (g_dc_confirm_count >= APP_DC_CONFIRM_COUNT)
        {
            if (g_dc_candidate_lost != 0U)
            {
                g_input_snapshot.plc_state = PLC_CONTROL_LOST;
            }
            else
            {
                g_input_snapshot.plc_state = PLC_CONTROL_VALID;
            }
        }
        else if (g_input_snapshot.plc_state == PLC_CONTROL_LOST)
        {
            g_input_snapshot.plc_state = PLC_CONTROL_RESTORE_CONFIRM;
        }
    }

    if (g_input_snapshot.plc_state != PLC_CONTROL_VALID)
    {
        return;
    }

    if ((tick_ms - g_ken_last_sample_ms) >= APP_KEN_CONFIRM_PERIOD_MS)
    {
        g_ken_last_sample_ms = tick_ms;
        InputFilter_UpdateKenOne(&g_ken_filters[0], InputFilter_ReadKenActive(0U));
        InputFilter_UpdateKenOne(&g_ken_filters[1], InputFilter_ReadKenActive(1U));
        InputFilter_UpdateKenOne(&g_ken_filters[2], InputFilter_ReadKenActive(2U));
        InputFilter_UpdateSnapshotFromKenFilters();
    }
}

InputFilterSnapshot_t InputFilter_GetSnapshot(void)
{
    return g_input_snapshot;
}
