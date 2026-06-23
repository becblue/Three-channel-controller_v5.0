#ifndef __FLASH_W25Q128_H__
#define __FLASH_W25Q128_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define W25Q128_PAGE_SIZE      256U
#define W25Q128_SECTOR_SIZE    4096U
#define W25Q128_TOTAL_SIZE     (16UL * 1024UL * 1024UL)

typedef enum
{
    W25Q_RESULT_OK = 0,
    W25Q_RESULT_NOT_READY,
    W25Q_RESULT_TIMEOUT,
    W25Q_RESULT_PARAM_ERROR,
    W25Q_RESULT_HAL_ERROR
} W25QResult_t;

void W25Q128_Init(void);
uint8_t W25Q128_IsReady(void);
uint32_t W25Q128_GetJedecId(void);
W25QResult_t W25Q128_Read(uint32_t address, uint8_t *data, uint32_t length);
W25QResult_t W25Q128_PageProgram(uint32_t address, const uint8_t *data, uint16_t length);
W25QResult_t W25Q128_SectorErase(uint32_t address);
W25QResult_t W25Q128_WaitReady(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* __FLASH_W25Q128_H__ */
