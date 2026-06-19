#include "watchdog_manager.h"

#include "app_config.h"
#include "main.h"

#define WATCHDOG_ALL_TASK_MASK       ((1UL << (uint32_t)WATCHDOG_TASK_COUNT) - 1UL)
#define WATCHDOG_IWDG_KEY_ENABLE     0xCCCCU
#define WATCHDOG_IWDG_KEY_RELOAD     0xAAAAU
#define WATCHDOG_IWDG_KEY_WRITE      0x5555U
#define WATCHDOG_IWDG_PRESCALER_64   0x04U
#define WATCHDOG_IWDG_LSI_HZ         40000U

static WatchdogPanicReason_t g_panic_reason;
static uint32_t g_alive_task_bits;
static uint32_t g_last_task_tick_ms;
static uint32_t g_last_feed_tick_ms;
static uint8_t g_iwdg_reset_detected;
static uint8_t g_iwdg_started;

static void WatchdogManager_StartHardware(void)
{
#if (APP_WATCHDOG_ENABLE != 0)
    uint32_t reload_value = ((APP_WATCHDOG_TIMEOUT_MS * (WATCHDOG_IWDG_LSI_HZ / 64U)) / 1000U);

    if (reload_value > 0x0FFFU)
    {
        reload_value = 0x0FFFU;
    }

    if (reload_value == 0U)
    {
        reload_value = 1U;
    }

    IWDG->KR = WATCHDOG_IWDG_KEY_WRITE;
    IWDG->PR = WATCHDOG_IWDG_PRESCALER_64;
    IWDG->RLR = reload_value;
    IWDG->KR = WATCHDOG_IWDG_KEY_RELOAD;
    IWDG->KR = WATCHDOG_IWDG_KEY_ENABLE;
    g_iwdg_started = 1U;
#else
    g_iwdg_started = 0U;
#endif
}

static void WatchdogManager_FeedHardware(void)
{
#if (APP_WATCHDOG_ENABLE != 0)
    if (g_iwdg_started != 0U)
    {
        IWDG->KR = WATCHDOG_IWDG_KEY_RELOAD;
    }
#else
    (void)g_iwdg_started;
#endif
}

static void WatchdogManager_CheckResetReason(void)
{
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET)
    {
        g_iwdg_reset_detected = 1U;
    }
    else
    {
        g_iwdg_reset_detected = 0U;
    }

    __HAL_RCC_CLEAR_RESET_FLAGS();
}

void WatchdogManager_Init(void)
{
    g_panic_reason = WATCHDOG_PANIC_NONE;
    g_alive_task_bits = 0U;
    g_last_task_tick_ms = 0U;
    g_last_feed_tick_ms = 0U;
    g_iwdg_started = 0U;
    WatchdogManager_CheckResetReason();
    WatchdogManager_StartHardware();
}

void WatchdogManager_Task(uint32_t tick_ms)
{
    uint32_t loop_delta_ms;

    if (g_panic_reason != WATCHDOG_PANIC_NONE)
    {
        return;
    }

    if (g_last_task_tick_ms != 0U)
    {
        loop_delta_ms = tick_ms - g_last_task_tick_ms;

        if (loop_delta_ms > APP_MAIN_LOOP_MAX_MS)
        {
            WatchdogManager_Panic(WATCHDOG_PANIC_MAIN_LOOP_TIMEOUT);
            return;
        }
    }

    g_last_task_tick_ms = tick_ms;

    if ((g_alive_task_bits & WATCHDOG_ALL_TASK_MASK) != WATCHDOG_ALL_TASK_MASK)
    {
        WatchdogManager_Panic(WATCHDOG_PANIC_TASK_MISSED);
        return;
    }

    g_alive_task_bits = 0U;

    if ((tick_ms - g_last_feed_tick_ms) >= APP_WATCHDOG_FEED_PERIOD_MS)
    {
        g_last_feed_tick_ms = tick_ms;
        WatchdogManager_FeedHardware();
    }
}

void WatchdogManager_MarkTaskAlive(WatchdogTaskId_t task_id)
{
    if (task_id < WATCHDOG_TASK_COUNT)
    {
        g_alive_task_bits |= (1UL << (uint32_t)task_id);
    }
}

void WatchdogManager_Panic(WatchdogPanicReason_t reason)
{
    if (g_panic_reason == WATCHDOG_PANIC_NONE)
    {
        g_panic_reason = reason;
    }
}

WatchdogPanicReason_t WatchdogManager_GetLastPanicReason(void)
{
    return g_panic_reason;
}

uint8_t WatchdogManager_WasIwdgReset(void)
{
    return g_iwdg_reset_detected;
}
