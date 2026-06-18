#ifndef __APP_TYPES_H__
#define __APP_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
    APP_CHANNEL_NONE = 0,
    APP_CHANNEL_1 = 1,
    APP_CHANNEL_2 = 2,
    APP_CHANNEL_3 = 3
} AppChannel_t;

typedef enum
{
    APP_CHANNEL_STATE_CLOSED = 0,
    APP_CHANNEL_STATE_OPEN = 1,
    APP_CHANNEL_STATE_UNKNOWN = 2,
    APP_CHANNEL_STATE_FAULT = 3
} AppChannelState_t;

typedef enum
{
    APP_ACTION_NONE = 0,
    APP_ACTION_OPEN = 1,
    APP_ACTION_CLOSE = 2
} AppAction_t;

typedef enum
{
    APP_RESULT_OK = 0,
    APP_RESULT_BUSY = 1,
    APP_RESULT_REJECTED = 2,
    APP_RESULT_ERROR = 3
} AppResult_t;

typedef enum
{
    APP_FAULT_A = 0,
    APP_FAULT_B,
    APP_FAULT_C,
    APP_FAULT_D,
    APP_FAULT_E,
    APP_FAULT_F,
    APP_FAULT_G,
    APP_FAULT_H,
    APP_FAULT_I,
    APP_FAULT_J,
    APP_FAULT_K,
    APP_FAULT_L,
    APP_FAULT_M,
    APP_FAULT_N,
    APP_FAULT_O,
    APP_FAULT_COUNT
} AppFaultId_t;

typedef enum
{
    APP_FAULT_SEVERITY_NONE = 0,
    APP_FAULT_SEVERITY_GENERAL,
    APP_FAULT_SEVERITY_URGENT,
    APP_FAULT_SEVERITY_SERIOUS
} AppFaultSeverity_t;

#ifdef __cplusplus
}
#endif

#endif /* __APP_TYPES_H__ */
