#include "flash_w25q128.h"

#include "main.h"
#include "spi.h"

#define W25Q_CMD_WRITE_ENABLE   0x06U
#define W25Q_CMD_READ_STATUS_1  0x05U
#define W25Q_CMD_PAGE_PROGRAM   0x02U
#define W25Q_CMD_READ_DATA      0x03U
#define W25Q_CMD_SECTOR_ERASE   0x20U
#define W25Q_CMD_JEDEC_ID       0x9FU

#define W25Q_STATUS_BUSY        0x01U
#define W25Q_EXPECTED_ID        0xEF4018UL
#define W25Q_SPI_TIMEOUT_MS     20U
#define W25Q_WRITE_TIMEOUT_MS   20U
#define W25Q_ERASE_TIMEOUT_MS   500U

static uint8_t g_w25q_ready;
static uint32_t g_w25q_jedec_id;

static void W25Q128_Select(void)
{
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
}

static void W25Q128_Unselect(void)
{
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
}

static W25QResult_t W25Q128_Transmit(const uint8_t *data, uint16_t length)
{
    if (HAL_SPI_Transmit(&hspi2, (uint8_t *)data, length, W25Q_SPI_TIMEOUT_MS) != HAL_OK)
    {
        return W25Q_RESULT_HAL_ERROR;
    }

    return W25Q_RESULT_OK;
}

static W25QResult_t W25Q128_Receive(uint8_t *data, uint16_t length)
{
    if (HAL_SPI_Receive(&hspi2, data, length, W25Q_SPI_TIMEOUT_MS) != HAL_OK)
    {
        return W25Q_RESULT_HAL_ERROR;
    }

    return W25Q_RESULT_OK;
}

static void W25Q128_FillAddress(uint8_t *buffer, uint32_t address)
{
    buffer[0] = (uint8_t)((address >> 16) & 0xFFU);
    buffer[1] = (uint8_t)((address >> 8) & 0xFFU);
    buffer[2] = (uint8_t)(address & 0xFFU);
}

static W25QResult_t W25Q128_ReadStatus(uint8_t *status)
{
    uint8_t command = W25Q_CMD_READ_STATUS_1;
    W25QResult_t result;

    W25Q128_Select();
    result = W25Q128_Transmit(&command, 1U);

    if (result == W25Q_RESULT_OK)
    {
        result = W25Q128_Receive(status, 1U);
    }

    W25Q128_Unselect();
    return result;
}

static W25QResult_t W25Q128_WriteEnable(void)
{
    uint8_t command = W25Q_CMD_WRITE_ENABLE;
    W25QResult_t result;

    W25Q128_Select();
    result = W25Q128_Transmit(&command, 1U);
    W25Q128_Unselect();

    return result;
}

void W25Q128_Init(void)
{
    uint8_t command = W25Q_CMD_JEDEC_ID;
    uint8_t id_buffer[3] = {0U, 0U, 0U};

    g_w25q_ready = 0U;
    g_w25q_jedec_id = 0U;
    W25Q128_Unselect();

    W25Q128_Select();

    if ((W25Q128_Transmit(&command, 1U) == W25Q_RESULT_OK) &&
        (W25Q128_Receive(id_buffer, 3U) == W25Q_RESULT_OK))
    {
        g_w25q_jedec_id = ((uint32_t)id_buffer[0] << 16) |
                          ((uint32_t)id_buffer[1] << 8) |
                          (uint32_t)id_buffer[2];
    }

    W25Q128_Unselect();

    if (g_w25q_jedec_id == W25Q_EXPECTED_ID)
    {
        g_w25q_ready = 1U;
    }
}

uint8_t W25Q128_IsReady(void)
{
    return g_w25q_ready;
}

uint32_t W25Q128_GetJedecId(void)
{
    return g_w25q_jedec_id;
}

W25QResult_t W25Q128_WaitReady(uint32_t timeout_ms)
{
    uint32_t start_tick = HAL_GetTick();
    uint8_t status = 0U;
    W25QResult_t result;

    if (g_w25q_ready == 0U)
    {
        return W25Q_RESULT_NOT_READY;
    }

    do
    {
        result = W25Q128_ReadStatus(&status);

        if (result != W25Q_RESULT_OK)
        {
            return result;
        }

        if ((status & W25Q_STATUS_BUSY) == 0U)
        {
            return W25Q_RESULT_OK;
        }
    } while ((HAL_GetTick() - start_tick) < timeout_ms);

    return W25Q_RESULT_TIMEOUT;
}

W25QResult_t W25Q128_Read(uint32_t address, uint8_t *data, uint32_t length)
{
    uint8_t command[4];
    W25QResult_t result;

    if ((data == 0) || ((address + length) > W25Q128_TOTAL_SIZE))
    {
        return W25Q_RESULT_PARAM_ERROR;
    }

    if (g_w25q_ready == 0U)
    {
        return W25Q_RESULT_NOT_READY;
    }

    command[0] = W25Q_CMD_READ_DATA;
    W25Q128_FillAddress(&command[1], address);

    W25Q128_Select();
    result = W25Q128_Transmit(command, sizeof(command));

    if (result == W25Q_RESULT_OK)
    {
        while ((length > 0U) && (result == W25Q_RESULT_OK))
        {
            uint16_t chunk = (length > 256U) ? 256U : (uint16_t)length;
            result = W25Q128_Receive(data, chunk);
            data += chunk;
            length -= chunk;
        }
    }

    W25Q128_Unselect();
    return result;
}

W25QResult_t W25Q128_PageProgram(uint32_t address, const uint8_t *data, uint16_t length)
{
    uint8_t command[4];
    W25QResult_t result;

    if ((data == 0) || (length == 0U) || (length > W25Q128_PAGE_SIZE) ||
        (((address % W25Q128_PAGE_SIZE) + length) > W25Q128_PAGE_SIZE) ||
        ((address + length) > W25Q128_TOTAL_SIZE))
    {
        return W25Q_RESULT_PARAM_ERROR;
    }

    if (g_w25q_ready == 0U)
    {
        return W25Q_RESULT_NOT_READY;
    }

    result = W25Q128_WaitReady(W25Q_WRITE_TIMEOUT_MS);

    if (result != W25Q_RESULT_OK)
    {
        return result;
    }

    result = W25Q128_WriteEnable();

    if (result != W25Q_RESULT_OK)
    {
        return result;
    }

    command[0] = W25Q_CMD_PAGE_PROGRAM;
    W25Q128_FillAddress(&command[1], address);

    W25Q128_Select();
    result = W25Q128_Transmit(command, sizeof(command));

    if (result == W25Q_RESULT_OK)
    {
        result = W25Q128_Transmit(data, length);
    }

    W25Q128_Unselect();

    if (result != W25Q_RESULT_OK)
    {
        return result;
    }

    return W25Q128_WaitReady(W25Q_WRITE_TIMEOUT_MS);
}

W25QResult_t W25Q128_SectorErase(uint32_t address)
{
    uint8_t command[4];
    W25QResult_t result;

    if ((address >= W25Q128_TOTAL_SIZE) || ((address % W25Q128_SECTOR_SIZE) != 0U))
    {
        return W25Q_RESULT_PARAM_ERROR;
    }

    if (g_w25q_ready == 0U)
    {
        return W25Q_RESULT_NOT_READY;
    }

    result = W25Q128_WaitReady(W25Q_ERASE_TIMEOUT_MS);

    if (result != W25Q_RESULT_OK)
    {
        return result;
    }

    result = W25Q128_WriteEnable();

    if (result != W25Q_RESULT_OK)
    {
        return result;
    }

    command[0] = W25Q_CMD_SECTOR_ERASE;
    W25Q128_FillAddress(&command[1], address);

    W25Q128_Select();
    result = W25Q128_Transmit(command, sizeof(command));
    W25Q128_Unselect();

    if (result != W25Q_RESULT_OK)
    {
        return result;
    }

    return W25Q128_WaitReady(W25Q_ERASE_TIMEOUT_MS);
}
