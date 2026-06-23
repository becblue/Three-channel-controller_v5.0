#ifndef __RESET_REASON_STORE_H__
#define __RESET_REASON_STORE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "watchdog_manager.h"

typedef enum
{
    RESET_SW_REASON_NONE = 0,
    RESET_SW_REASON_WATCHDOG_MAIN_LOOP_TIMEOUT = 1,
    RESET_SW_REASON_WATCHDOG_TASK_MISSED = 2,
    RESET_SW_REASON_WATCHDOG_TICK_ERROR = 3,
    RESET_SW_REASON_WATCHDOG_STATE_ERROR = 4,
    RESET_SW_REASON_HARDFAULT = 16,
    RESET_SW_REASON_MEMFAULT = 17,
    RESET_SW_REASON_BUSFAULT = 18,
    RESET_SW_REASON_USAGEFAULT = 19,
    RESET_SW_REASON_CSS_FAULT = 32,
    RESET_SW_REASON_PVD_LOW_VOLTAGE = 33
} ResetSoftwareReason_t;

#define RESET_HW_FLAG_PIN       (1UL << 0)
#define RESET_HW_FLAG_POR_PDR   (1UL << 1)
#define RESET_HW_FLAG_SOFTWARE  (1UL << 2)
#define RESET_HW_FLAG_IWDG      (1UL << 3)
#define RESET_HW_FLAG_WWDG      (1UL << 4)
#define RESET_HW_FLAG_LOW_POWER (1UL << 5)

typedef struct
{
    uint32_t hardware_flags;
    uint16_t software_reason;
    uint16_t software_detail;
    uint8_t backup_reason_valid;
} ResetReasonSnapshot_t;

void ResetReasonStore_Init(void);
ResetReasonSnapshot_t ResetReasonStore_GetSnapshot(void);
void ResetReasonStore_SaveWatchdogPanic(WatchdogPanicReason_t reason);
void ResetReasonStore_SaveSoftwareReason(ResetSoftwareReason_t reason, uint16_t detail);
uint8_t ResetReasonStore_WasIwdgReset(void);

#ifdef __cplusplus
}
#endif

#endif /* __RESET_REASON_STORE_H__ */
