#include "temp_ds18b20.h"
#include "main.h"

#define DS18B20_PORT TEMPE_GPIO_Port
#define DS18B20_PIN  TEMPE_Pin

static void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

static void DS18B20_WriteBit(uint8_t bit)
{
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    delay_us(bit ? 6 : 60);
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
    delay_us(bit ? 64 : 10);
}

static uint8_t DS18B20_ReadBit(void)
{
    uint8_t bit = 0;

    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    delay_us(3);
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
    delay_us(10);

    bit = HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN);
    delay_us(50);

    return bit;
}

static void DS18B20_WriteByte(uint8_t byte)
{
    for (int i = 0; i < 8; i++)
        DS18B20_WriteBit(byte & (1 << i));
}

static uint8_t DS18B20_ReadByte(void)
{
    uint8_t byte = 0;

    for (int i = 0; i < 8; i++)
        byte |= (DS18B20_ReadBit() << i);

    return byte;
}

static uint8_t DS18B20_Reset(void)
{
    uint8_t presence = 0;

    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    delay_us(480);

    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
    delay_us(70);

    presence = HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN);
    delay_us(410);

    return presence == 0;
}

void TEMPE_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

float TEMPE_Read(void)
{
    uint8_t temp_lsb, temp_msb;
    int16_t temp_raw;

    if (!DS18B20_Reset())
        return -100.0f;

    DS18B20_WriteByte(0xCC); // SKIP ROM
    DS18B20_WriteByte(0x44); // CONVERT T

    HAL_Delay(750); // Tiempo máximo de conversión

    if (!DS18B20_Reset())
        return -100.0f;

    DS18B20_WriteByte(0xCC); // SKIP ROM
    DS18B20_WriteByte(0xBE); // READ SCRATCHPAD

    temp_lsb = DS18B20_ReadByte();
    temp_msb = DS18B20_ReadByte();

    temp_raw = (temp_msb << 8) | temp_lsb;

    return temp_raw / 16.0f;
}
