#include "reset_reason_store.h"

#include "main.h"

#define RESET_BKP_MAGIC        0x5A3CU
#define RESET_BKP_MAGIC_CLEAR  0x0000U

static ResetReasonSnapshot_t g_reset_snapshot;

static void ResetReasonStore_EnableBackupAccess(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
}

static uint16_t ResetReasonStore_CalcBackupCheck(uint16_t reason, uint16_t detail)
{
    return (uint16_t)(RESET_BKP_MAGIC ^ reason ^ detail ^ 0xA55AU);
}

static uint32_t ResetReasonStore_ReadHardwareFlags(void)
{
    uint32_t flags = 0U;

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != RESET)
    {
        flags |= RESET_HW_FLAG_PIN;
    }

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST) != RESET)
    {
        flags |= RESET_HW_FLAG_POR_PDR;
    }

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) != RESET)
    {
        flags |= RESET_HW_FLAG_SOFTWARE;
    }

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET)
    {
        flags |= RESET_HW_FLAG_IWDG;
    }

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != RESET)
    {
        flags |= RESET_HW_FLAG_WWDG;
    }

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST) != RESET)
    {
        flags |= RESET_HW_FLAG_LOW_POWER;
    }

    return flags;
}

static uint8_t ResetReasonStore_ShouldUseBackupReason(uint32_t hardware_flags)
{
    if ((hardware_flags & (RESET_HW_FLAG_IWDG | RESET_HW_FLAG_SOFTWARE | RESET_HW_FLAG_WWDG)) != 0U)
    {
        return 1U;
    }

    return 0U;
}

static void ResetReasonStore_ClearBackupReason(void)
{
    BKP->DR1 = RESET_BKP_MAGIC_CLEAR;
    BKP->DR2 = 0U;
    BKP->DR3 = 0U;
    BKP->DR4 = 0U;
}

void ResetReasonStore_Init(void)
{
    uint16_t backup_magic;
    uint16_t backup_reason;
    uint16_t backup_detail;
    uint16_t backup_check;

    ResetReasonStore_EnableBackupAccess();

    g_reset_snapshot.hardware_flags = ResetReasonStore_ReadHardwareFlags();
    g_reset_snapshot.software_reason = (uint16_t)RESET_SW_REASON_NONE;
    g_reset_snapshot.software_detail = 0U;
    g_reset_snapshot.backup_reason_valid = 0U;

    backup_magic = (uint16_t)BKP->DR1;
    backup_reason = (uint16_t)BKP->DR2;
    backup_detail = (uint16_t)BKP->DR3;
    backup_check = (uint16_t)BKP->DR4;

    if ((backup_magic == RESET_BKP_MAGIC) &&
        (backup_check == ResetReasonStore_CalcBackupCheck(backup_reason, backup_detail)) &&
        (ResetReasonStore_ShouldUseBackupReason(g_reset_snapshot.hardware_flags) != 0U))
    {
        g_reset_snapshot.software_reason = backup_reason;
        g_reset_snapshot.software_detail = backup_detail;
        g_reset_snapshot.backup_reason_valid = 1U;
    }

    ResetReasonStore_ClearBackupReason();
    __HAL_RCC_CLEAR_RESET_FLAGS();
}

ResetReasonSnapshot_t ResetReasonStore_GetSnapshot(void)
{
    return g_reset_snapshot;
}

void ResetReasonStore_SaveSoftwareReason(ResetSoftwareReason_t reason, uint16_t detail)
{
    ResetReasonStore_EnableBackupAccess();

    BKP->DR1 = RESET_BKP_MAGIC;
    BKP->DR2 = (uint16_t)reason;
    BKP->DR3 = detail;
    BKP->DR4 = ResetReasonStore_CalcBackupCheck((uint16_t)reason, detail);
}

void ResetReasonStore_SaveWatchdogPanic(WatchdogPanicReason_t reason)
{
    ResetSoftwareReason_t software_reason = RESET_SW_REASON_NONE;

    if (reason == WATCHDOG_PANIC_MAIN_LOOP_TIMEOUT)
    {
        software_reason = RESET_SW_REASON_WATCHDOG_MAIN_LOOP_TIMEOUT;
    }
    else if (reason == WATCHDOG_PANIC_TASK_MISSED)
    {
        software_reason = RESET_SW_REASON_WATCHDOG_TASK_MISSED;
    }
    else if (reason == WATCHDOG_PANIC_TICK_ERROR)
    {
        software_reason = RESET_SW_REASON_WATCHDOG_TICK_ERROR;
    }
    else if (reason == WATCHDOG_PANIC_STATE_ERROR)
    {
        software_reason = RESET_SW_REASON_WATCHDOG_STATE_ERROR;
    }

    ResetReasonStore_SaveSoftwareReason(software_reason, (uint16_t)reason);
}

uint8_t ResetReasonStore_WasIwdgReset(void)
{
    return ((g_reset_snapshot.hardware_flags & RESET_HW_FLAG_IWDG) != 0U) ? 1U : 0U;
}
