#ifndef __APP_CONTROLLER_H__
#define __APP_CONTROLLER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
    APP_STATE_BOOT = 0,
    APP_STATE_LOGO,
    APP_STATE_SELF_TEST,
    APP_STATE_STANDBY,
    APP_STATE_RUNNING,
    APP_STATE_ALARM,
    APP_STATE_ERROR
} AppState_t;

void AppController_Init(uint32_t tick_ms);
void AppController_RunOnce(uint32_t tick_ms);
AppState_t AppController_GetState(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_CONTROLLER_H__ */
