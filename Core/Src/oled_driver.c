#include "oled_driver.h"

#include "i2c.h"
#include <string.h>

#define OLED_I2C_ADDR              0x78U
#define OLED_CONTROL_CMD           0x00U
#define OLED_CONTROL_DATA          0x40U
#define OLED_DIRTY_NONE            255U

static uint8_t g_oled_gram[OLED_PAGE_COUNT][OLED_WIDTH];
static uint8_t g_dirty_start_page;
static uint8_t g_dirty_end_page;

static void OledDriver_WriteCommand(uint8_t command)
{
    (void)HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDR, OLED_CONTROL_CMD, I2C_MEMADD_SIZE_8BIT, &command, 1U, 20U);
}

static void OledDriver_WriteData(uint8_t *data, uint16_t size)
{
    (void)HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDR, OLED_CONTROL_DATA, I2C_MEMADD_SIZE_8BIT, data, size, 100U);
}

static void OledDriver_MarkDirtyPage(uint8_t page)
{
    if (page >= OLED_PAGE_COUNT)
    {
        return;
    }

    if (g_dirty_start_page == OLED_DIRTY_NONE)
    {
        g_dirty_start_page = page;
        g_dirty_end_page = page;
    }
    else
    {
        if (page < g_dirty_start_page)
        {
            g_dirty_start_page = page;
        }

        if (page > g_dirty_end_page)
        {
            g_dirty_end_page = page;
        }
    }
}

static void OledDriver_MarkDirtyPages(uint8_t start_page, uint8_t page_count)
{
    uint8_t page;

    for (page = 0U; page < page_count; page++)
    {
        OledDriver_MarkDirtyPage((uint8_t)(start_page + page));
    }
}

static const uint8_t *OledDriver_GetGlyph(char ch)
{
    static const uint8_t blank[5] = {0x00,0x00,0x00,0x00,0x00};
    static const uint8_t percent[5] = {0x63,0x13,0x08,0x64,0x63};
    static const uint8_t colon[5] = {0x00,0x36,0x36,0x00,0x00};
    static const uint8_t minus[5] = {0x08,0x08,0x08,0x08,0x08};
    static const uint8_t dot[5] = {0x00,0x60,0x60,0x00,0x00};
    static const uint8_t digits[10][5] = {
        {0x3E,0x51,0x49,0x45,0x3E},
        {0x00,0x42,0x7F,0x40,0x00},
        {0x42,0x61,0x51,0x49,0x46},
        {0x21,0x41,0x45,0x4B,0x31},
        {0x18,0x14,0x12,0x7F,0x10},
        {0x27,0x45,0x45,0x45,0x39},
        {0x3C,0x4A,0x49,0x49,0x30},
        {0x01,0x71,0x09,0x05,0x03},
        {0x36,0x49,0x49,0x49,0x36},
        {0x06,0x49,0x49,0x29,0x1E}
    };
    static const uint8_t letters[26][5] = {
        {0x7E,0x11,0x11,0x11,0x7E},
        {0x7F,0x49,0x49,0x49,0x36},
        {0x3E,0x41,0x41,0x41,0x22},
        {0x7F,0x41,0x41,0x22,0x1C},
        {0x7F,0x49,0x49,0x49,0x41},
        {0x7F,0x09,0x09,0x09,0x01},
        {0x3E,0x41,0x49,0x49,0x7A},
        {0x7F,0x08,0x08,0x08,0x7F},
        {0x00,0x41,0x7F,0x41,0x00},
        {0x20,0x40,0x41,0x3F,0x01},
        {0x7F,0x08,0x14,0x22,0x41},
        {0x7F,0x40,0x40,0x40,0x40},
        {0x7F,0x02,0x0C,0x02,0x7F},
        {0x7F,0x04,0x08,0x10,0x7F},
        {0x3E,0x41,0x41,0x41,0x3E},
        {0x7F,0x09,0x09,0x09,0x06},
        {0x3E,0x41,0x51,0x21,0x5E},
        {0x7F,0x09,0x19,0x29,0x46},
        {0x46,0x49,0x49,0x49,0x31},
        {0x01,0x01,0x7F,0x01,0x01},
        {0x3F,0x40,0x40,0x40,0x3F},
        {0x1F,0x20,0x40,0x20,0x1F},
        {0x3F,0x40,0x38,0x40,0x3F},
        {0x63,0x14,0x08,0x14,0x63},
        {0x07,0x08,0x70,0x08,0x07},
        {0x61,0x51,0x49,0x45,0x43}
    };

    if ((ch >= 'a') && (ch <= 'z'))
    {
        ch = (char)(ch - 32);
    }

    if ((ch >= '0') && (ch <= '9'))
    {
        return digits[ch - '0'];
    }

    if ((ch >= 'A') && (ch <= 'Z'))
    {
        return letters[ch - 'A'];
    }

    if (ch == '%')
    {
        return percent;
    }

    if (ch == ':')
    {
        return colon;
    }

    if (ch == '-')
    {
        return minus;
    }

    if (ch == '.')
    {
        return dot;
    }

    return blank;
}

uint8_t OledDriver_Init(void)
{
    if (HAL_I2C_IsDeviceReady(&hi2c1, OLED_I2C_ADDR, 2U, 20U) != HAL_OK)
    {
        return 0U;
    }

    g_dirty_start_page = OLED_DIRTY_NONE;
    g_dirty_end_page = OLED_DIRTY_NONE;

    OledDriver_WriteCommand(0xAEU);
    OledDriver_WriteCommand(0xD5U);
    OledDriver_WriteCommand(0x80U);
    OledDriver_WriteCommand(0xA8U);
    OledDriver_WriteCommand(0x3FU);
    OledDriver_WriteCommand(0xD3U);
    OledDriver_WriteCommand(0x00U);
    OledDriver_WriteCommand(0x40U);
    OledDriver_WriteCommand(0x8DU);
    OledDriver_WriteCommand(0x14U);
    OledDriver_WriteCommand(0x20U);
    OledDriver_WriteCommand(0x00U);
    OledDriver_WriteCommand(0xA1U);
    OledDriver_WriteCommand(0xC8U);
    OledDriver_WriteCommand(0xDAU);
    OledDriver_WriteCommand(0x12U);
    OledDriver_WriteCommand(0x81U);
    OledDriver_WriteCommand(0xCFU);
    OledDriver_WriteCommand(0xD9U);
    OledDriver_WriteCommand(0xF1U);
    OledDriver_WriteCommand(0xDBU);
    OledDriver_WriteCommand(0x40U);
    OledDriver_WriteCommand(0xA4U);
    OledDriver_WriteCommand(0xA6U);
    OledDriver_WriteCommand(0xAFU);

    OledDriver_Clear();
    return 1U;
}

void OledDriver_Clear(void)
{
    memset(g_oled_gram, 0, sizeof(g_oled_gram));
    OledDriver_MarkDirtyPages(0U, OLED_PAGE_COUNT);
    OledDriver_RefreshDirty();
}

void OledDriver_ForceRefresh(void)
{
    OledDriver_MarkDirtyPages(0U, OLED_PAGE_COUNT);
    OledDriver_RefreshDirty();
}

void OledDriver_RefreshDirty(void)
{
    uint8_t page;

    if (g_dirty_start_page == OLED_DIRTY_NONE)
    {
        return;
    }

    for (page = g_dirty_start_page; page <= g_dirty_end_page; page++)
    {
        OledDriver_WriteCommand((uint8_t)(0xB0U + page));
        OledDriver_WriteCommand(0x00U);
        OledDriver_WriteCommand(0x10U);
        OledDriver_WriteData(g_oled_gram[page], OLED_WIDTH);
    }

    g_dirty_start_page = OLED_DIRTY_NONE;
    g_dirty_end_page = OLED_DIRTY_NONE;
}

void OledDriver_ClearArea(uint8_t x, uint8_t page, uint8_t width, uint8_t page_count)
{
    uint8_t page_index;

    if (x >= OLED_WIDTH)
    {
        return;
    }

    if ((uint16_t)x + width > OLED_WIDTH)
    {
        width = (uint8_t)(OLED_WIDTH - x);
    }

    for (page_index = 0U; page_index < page_count; page_index++)
    {
        uint8_t target_page = (uint8_t)(page + page_index);

        if (target_page >= OLED_PAGE_COUNT)
        {
            break;
        }

        memset(&g_oled_gram[target_page][x], 0, width);
        OledDriver_MarkDirtyPage(target_page);
    }
}

void OledDriver_DrawString(uint8_t x, uint8_t page, const char *text, uint8_t invert)
{
    while ((*text != '\0') && (page < OLED_PAGE_COUNT) && (x < OLED_WIDTH))
    {
        uint8_t col;
        const uint8_t *glyph = OledDriver_GetGlyph(*text);

        for (col = 0U; col < 6U; col++)
        {
            uint8_t data = (col < 5U) ? glyph[col] : 0x00U;

            if (invert != 0U)
            {
                data = (uint8_t)~data;
            }

            if ((uint16_t)x + col < OLED_WIDTH)
            {
                g_oled_gram[page][x + col] = data;
            }
        }

        OledDriver_MarkDirtyPage(page);
        x = (uint8_t)(x + 6U);
        text++;
    }
}

void OledDriver_DrawBitmapRows(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *data)
{
    uint16_t bytes_per_row = (uint16_t)((width + 7U) / 8U);
    uint8_t row;

    for (row = 0U; row < height; row++)
    {
        uint16_t col_byte;

        for (col_byte = 0U; col_byte < bytes_per_row; col_byte++)
        {
            uint8_t bit;
            uint8_t data_byte = data[(uint16_t)row * bytes_per_row + col_byte];

            for (bit = 0U; bit < 8U; bit++)
            {
                uint8_t pixel_x = (uint8_t)(x + (uint8_t)(col_byte * 8U) + bit);
                uint8_t pixel_y = (uint8_t)(y + row);
                uint8_t color;

                if (((uint16_t)col_byte * 8U + bit) >= width)
                {
                    continue;
                }

                color = ((data_byte & (uint8_t)(0x80U >> bit)) != 0U) ? 1U : 0U;
                OledDriver_FillRect(pixel_x, pixel_y, 1U, 1U, color);
            }
        }
    }
}

void OledDriver_FillRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    uint8_t px;
    uint8_t py;

    for (py = y; (py < (uint8_t)(y + height)) && (py < OLED_HEIGHT); py++)
    {
        uint8_t page = (uint8_t)(py / 8U);
        uint8_t bit = (uint8_t)(1U << (py % 8U));

        for (px = x; (px < (uint8_t)(x + width)) && (px < OLED_WIDTH); px++)
        {
            if (color != 0U)
            {
                g_oled_gram[page][px] |= bit;
            }
            else
            {
                g_oled_gram[page][px] &= (uint8_t)~bit;
            }
        }

        OledDriver_MarkDirtyPage(page);
    }
}

void OledDriver_DrawFrame(uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    if ((width == 0U) || (height == 0U))
    {
        return;
    }

    OledDriver_FillRect(x, y, width, 1U, 1U);
    OledDriver_FillRect(x, (uint8_t)(y + height - 1U), width, 1U, 1U);
    OledDriver_FillRect(x, y, 1U, height, 1U);
    OledDriver_FillRect((uint8_t)(x + width - 1U), y, 1U, height, 1U);
}
