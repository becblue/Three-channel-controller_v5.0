#ifndef __OLED_DRIVER_H__
#define __OLED_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define OLED_WIDTH      128U
#define OLED_HEIGHT     64U
#define OLED_PAGE_COUNT 8U

void OledDriver_Init(void);
void OledDriver_Clear(void);
void OledDriver_ForceRefresh(void);
void OledDriver_RefreshDirty(void);
void OledDriver_ClearArea(uint8_t x, uint8_t page, uint8_t width, uint8_t page_count);
void OledDriver_DrawString(uint8_t x, uint8_t page, const char *text, uint8_t invert);
void OledDriver_DrawFrame(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void OledDriver_FillRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);

#ifdef __cplusplus
}
#endif

#endif /* __OLED_DRIVER_H__ */
