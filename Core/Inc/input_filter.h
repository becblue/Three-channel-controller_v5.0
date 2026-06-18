#ifndef __INPUT_FILTER_H__
#define __INPUT_FILTER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "app_types.h"

typedef enum
{
    PLC_CONTROL_VALID = 0,
    PLC_CONTROL_SUSPECT_LOST,
    PLC_CONTROL_LOST,
    PLC_CONTROL_RESTORE_CONFIRM
} PlcControlState_t;

typedef struct
{
    uint8_t k1_en_active;
    uint8_t k2_en_active;
    uint8_t k3_en_active;
    PlcControlState_t plc_state;
} InputFilterSnapshot_t;

void InputFilter_Init(void);
void InputFilter_Task(uint32_t tick_ms);
InputFilterSnapshot_t InputFilter_GetSnapshot(void);

#ifdef __cplusplus
}
#endif

#endif /* __INPUT_FILTER_H__ */
