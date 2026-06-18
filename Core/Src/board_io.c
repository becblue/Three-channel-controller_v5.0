#include "board_io.h"

#include "adc.h"
#include "app_config.h"
#include "main.h"
#include "tim.h"

#define BOARD_IO_ADC_CHANNEL_COUNT 3U

static uint16_t g_adc_dma_buffer[BOARD_IO_ADC_CHANNEL_COUNT];

static uint8_t BoardIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    return (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) ? 1U : 0U;
}

static void BoardIO_WriteActiveLow(GPIO_TypeDef *port, uint16_t pin, uint8_t active)
{
    HAL_GPIO_WritePin(port, pin, (active != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void BoardIO_WritePin(GPIO_TypeDef *port, uint16_t pin, uint8_t state)
{
    HAL_GPIO_WritePin(port, pin, (state != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void BoardIO_Init(void)
{
    (void)HAL_ADCEx_Calibration_Start(&hadc1);
    (void)HAL_ADC_Start_DMA(&hadc1, (uint32_t *)g_adc_dma_buffer, BOARD_IO_ADC_CHANNEL_COUNT);
    (void)HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    BoardIO_SetFanPwmPercent(APP_FAN_PWM_LOW_PERCENT);
}

uint8_t BoardIO_ReadK1EnRaw(void)
{
    return BoardIO_ReadPin(K1_EN_GPIO_Port, K1_EN_Pin);
}

uint8_t BoardIO_ReadK2EnRaw(void)
{
    return BoardIO_ReadPin(K2_EN_GPIO_Port, K2_EN_Pin);
}

uint8_t BoardIO_ReadK3EnRaw(void)
{
    return BoardIO_ReadPin(K3_EN_GPIO_Port, K3_EN_Pin);
}

uint8_t BoardIO_ReadK1_1StaRaw(void)
{
    return BoardIO_ReadPin(K1_1_STA_GPIO_Port, K1_1_STA_Pin);
}

uint8_t BoardIO_ReadK1_2StaRaw(void)
{
    return BoardIO_ReadPin(K1_2_STA_GPIO_Port, K1_2_STA_Pin);
}

uint8_t BoardIO_ReadK2_1StaRaw(void)
{
    return BoardIO_ReadPin(K2_1_STA_GPIO_Port, K2_1_STA_Pin);
}

uint8_t BoardIO_ReadK2_2StaRaw(void)
{
    return BoardIO_ReadPin(K2_2_STA_GPIO_Port, K2_2_STA_Pin);
}

uint8_t BoardIO_ReadK3_1StaRaw(void)
{
    return BoardIO_ReadPin(K3_1_STA_GPIO_Port, K3_1_STA_Pin);
}

uint8_t BoardIO_ReadK3_2StaRaw(void)
{
    return BoardIO_ReadPin(K3_2_STA_GPIO_Port, K3_2_STA_Pin);
}

uint8_t BoardIO_ReadSw1StaRaw(void)
{
    return BoardIO_ReadPin(SW1_STA_GPIO_Port, SW1_STA_Pin);
}

uint8_t BoardIO_ReadSw2StaRaw(void)
{
    return BoardIO_ReadPin(SW2_STA_GPIO_Port, SW2_STA_Pin);
}

uint8_t BoardIO_ReadSw3StaRaw(void)
{
    return BoardIO_ReadPin(SW3_STA_GPIO_Port, SW3_STA_Pin);
}

uint8_t BoardIO_ReadDcCtrlRaw(void)
{
    return BoardIO_ReadPin(DC_CTRL_GPIO_Port, DC_CTRL_Pin);
}

uint8_t BoardIO_ReadFanSenseRaw(void)
{
    return BoardIO_ReadPin(FAN_SEN_GPIO_Port, FAN_SEN_Pin);
}

uint16_t BoardIO_ReadNtc1AdcRaw(void)
{
    return g_adc_dma_buffer[0];
}

uint16_t BoardIO_ReadNtc2AdcRaw(void)
{
    return g_adc_dma_buffer[1];
}

uint16_t BoardIO_ReadNtc3AdcRaw(void)
{
    return g_adc_dma_buffer[2];
}

uint8_t BoardIO_ReadKey1Raw(void)
{
    return BoardIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin);
}

uint8_t BoardIO_ReadKey2Raw(void)
{
    return BoardIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin);
}

void BoardIO_SetK1_1On(uint8_t state)
{
    BoardIO_WriteActiveLow(K1_1_ON_GPIO_Port, K1_1_ON_Pin, state);
}

void BoardIO_SetK1_1Off(uint8_t state)
{
    BoardIO_WriteActiveLow(K1_1_OFF_GPIO_Port, K1_1_OFF_Pin, state);
}

void BoardIO_SetK1_2On(uint8_t state)
{
    BoardIO_WriteActiveLow(K1_2_ON_GPIO_Port, K1_2_ON_Pin, state);
}

void BoardIO_SetK1_2Off(uint8_t state)
{
    BoardIO_WriteActiveLow(K1_2_OFF_GPIO_Port, K1_2_OFF_Pin, state);
}

void BoardIO_SetK2_1On(uint8_t state)
{
    BoardIO_WriteActiveLow(K2_1_ON_GPIO_Port, K2_1_ON_Pin, state);
}

void BoardIO_SetK2_1Off(uint8_t state)
{
    BoardIO_WriteActiveLow(K2_1_OFF_GPIO_Port, K2_1_OFF_Pin, state);
}

void BoardIO_SetK2_2On(uint8_t state)
{
    BoardIO_WriteActiveLow(K2_2_ON_GPIO_Port, K2_2_ON_Pin, state);
}

void BoardIO_SetK2_2Off(uint8_t state)
{
    BoardIO_WriteActiveLow(K2_2_OFF_GPIO_Port, K2_2_OFF_Pin, state);
}

void BoardIO_SetK3_1On(uint8_t state)
{
    BoardIO_WriteActiveLow(K3_1_ON_GPIO_Port, K3_1_ON_Pin, state);
}

void BoardIO_SetK3_1Off(uint8_t state)
{
    BoardIO_WriteActiveLow(K3_1_OFF_GPIO_Port, K3_1_OFF_Pin, state);
}

void BoardIO_SetK3_2On(uint8_t state)
{
    BoardIO_WriteActiveLow(K3_2_ON_GPIO_Port, K3_2_ON_Pin, state);
}

void BoardIO_SetK3_2Off(uint8_t state)
{
    BoardIO_WriteActiveLow(K3_2_OFF_GPIO_Port, K3_2_OFF_Pin, state);
}

void BoardIO_SetAlarmOutput(uint8_t active)
{
    BoardIO_WriteActiveLow(ALARM_GPIO_Port, ALARM_Pin, active);
}

void BoardIO_SetBuzzerOutput(uint8_t active)
{
    BoardIO_WriteActiveLow(BEEP_GPIO_Port, BEEP_Pin, active);
}

void BoardIO_SetFanPwmPercent(uint8_t percent)
{
    uint32_t period = __HAL_TIM_GET_AUTORELOAD(&htim3) + 1U;
    uint32_t pulse;

    if (percent > 100U)
    {
        percent = 100U;
    }

    pulse = (period * percent) / 100U;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);
}

void BoardIO_SetRs485Enable(uint8_t state)
{
    BoardIO_WritePin(RS485DE_GPIO_Port, RS485DE_Pin, state);
}

void BoardIO_SetRunHeartbeatOutput(uint8_t state)
{
    BoardIO_WritePin(RUN_HB_GPIO_Port, RUN_HB_Pin, state);
}
