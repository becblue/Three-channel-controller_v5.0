#include "maintenance_manager.h"

#include "app_config.h"
#include "board_io.h"
#include "log_manager.h"

typedef struct
{
    uint8_t stable_pressed;
    uint8_t last_raw_pressed;
    uint8_t long_triggered;
    uint32_t debounce_tick;
    uint32_t pressed_tick;
} MaintKeyState_t;

static MaintKeyState_t g_key1;
static MaintKeyState_t g_key2;
static uint8_t g_log_view_active;
static uint16_t g_log_page;

static uint32_t MaintenanceManager_ElapsedMs(uint32_t now, uint32_t last)
{
    return now - last;
}

static void MaintenanceManager_ResetKey(MaintKeyState_t *key, uint8_t raw_pressed, uint32_t tick_ms)
{
    key->stable_pressed = raw_pressed;
    key->last_raw_pressed = raw_pressed;
    key->long_triggered = 0U;
    key->debounce_tick = tick_ms;
    key->pressed_tick = tick_ms;
}

static uint8_t MaintenanceManager_UpdateKey(MaintKeyState_t *key, uint8_t raw_pressed, uint32_t tick_ms)
{
    uint8_t event = 0U;

    if (raw_pressed != key->last_raw_pressed)
    {
        key->last_raw_pressed = raw_pressed;
        key->debounce_tick = tick_ms;
    }

    if (MaintenanceManager_ElapsedMs(tick_ms, key->debounce_tick) < APP_MAINT_KEY_CONFIRM_MS)
    {
        return 0U;
    }

    if (raw_pressed != key->stable_pressed)
    {
        key->stable_pressed = raw_pressed;

        if (key->stable_pressed != 0U)
        {
            key->pressed_tick = tick_ms;
            key->long_triggered = 0U;
        }
        else
        {
            if ((key->long_triggered == 0U) &&
                (MaintenanceManager_ElapsedMs(tick_ms, key->pressed_tick) <= APP_MAINT_KEY_SHORT_MAX_MS))
            {
                event = 1U;
            }

            key->long_triggered = 0U;
        }
    }

    if ((key->stable_pressed != 0U) &&
        (key->long_triggered == 0U) &&
        (MaintenanceManager_ElapsedMs(tick_ms, key->pressed_tick) >= APP_MAINT_KEY_LONG_MS))
    {
        key->long_triggered = 1U;
        event = 2U;
    }

    return event;
}

static void MaintenanceManager_ShowCurrentPage(void)
{
    uint16_t page_count = LogManager_GetPageCount();

    if (g_log_page >= page_count)
    {
        g_log_page = (uint16_t)(page_count - 1U);
    }

    LogManager_PrintPage(g_log_page);
}

void MaintenanceManager_Init(void)
{
    LogManager_Init();
    MaintenanceManager_ResetKey(&g_key1, (BoardIO_ReadKey1Raw() == 0U) ? 1U : 0U, 0U);
    MaintenanceManager_ResetKey(&g_key2, (BoardIO_ReadKey2Raw() == 0U) ? 1U : 0U, 0U);
    g_log_view_active = 0U;
    g_log_page = 0U;
}

void MaintenanceManager_Task(uint32_t tick_ms)
{
    uint8_t key1_event;
    uint8_t key2_event;

    LogManager_Task(tick_ms);

    key1_event = MaintenanceManager_UpdateKey(&g_key1, (BoardIO_ReadKey1Raw() == 0U) ? 1U : 0U, tick_ms);
    key2_event = MaintenanceManager_UpdateKey(&g_key2, (BoardIO_ReadKey2Raw() == 0U) ? 1U : 0U, tick_ms);

    if (key1_event == 1U)
    {
        if (g_log_view_active == 0U)
        {
            g_log_view_active = 1U;
            g_log_page = 0U;
        }
        else if (g_log_page > 0U)
        {
            g_log_page--;
        }

        MaintenanceManager_ShowCurrentPage();
    }
    else if (key1_event == 2U)
    {
        g_log_view_active = 0U;
        LogManager_StopPrint();
    }

    if (key2_event == 1U)
    {
        if (g_log_view_active != 0U)
        {
            uint16_t page_count = LogManager_GetPageCount();

            if ((g_log_page + 1U) < page_count)
            {
                g_log_page++;
            }

            MaintenanceManager_ShowCurrentPage();
        }
    }
    else if (key2_event == 2U)
    {
        LogManager_Clear();
        g_log_view_active = 1U;
        g_log_page = 0U;
        MaintenanceManager_ShowCurrentPage();
    }
}
