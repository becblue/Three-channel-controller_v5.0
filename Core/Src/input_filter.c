#include "input_filter.h"

static InputFilterSnapshot_t g_input_snapshot;

void InputFilter_Init(void)
{
    g_input_snapshot.k1_en_active = 0U;
    g_input_snapshot.k2_en_active = 0U;
    g_input_snapshot.k3_en_active = 0U;
    g_input_snapshot.plc_state = PLC_CONTROL_RESTORE_CONFIRM;
}

void InputFilter_Task(uint32_t tick_ms)
{
    (void)tick_ms;
    /* TODO: 按已冻结规则实现 DC_CTRL 优先采样和 K_EN 稳定确认。 */
}

InputFilterSnapshot_t InputFilter_GetSnapshot(void)
{
    return g_input_snapshot;
}
