#include "bsp_fpga.h"
#include "stm32f4xx_hal.h"

static SRAM_HandleTypeDef hsram_fpga;
static FMC_NORSRAM_TimingTypeDef fpga_timing;
static uint8_t s_fpga_inited = 0;

void BSP_FPGA_Init(void)
{
    GPIO_InitTypeDef GPIO_Init_Structure;

    if (s_fpga_inited)
    {
        return;
    }
    s_fpga_inited = 1;

    /* Enable GPIO Clocks */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE(); /* NE2 on PG9 */

    /* Enable FMC Clock */
    __HAL_RCC_FMC_CLK_ENABLE();

    /* Common GPIO Configuration */
    GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
    GPIO_Init_Structure.Pull      = GPIO_PULLUP;
    GPIO_Init_Structure.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_Init_Structure.Alternate = GPIO_AF12_FMC;

    /* GPIOD: 0,1,4,5,8,9,10,11(A16),12(A17),14,15 */
    GPIO_Init_Structure.Pin = GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_4  | GPIO_PIN_5  |
                              GPIO_PIN_8  | GPIO_PIN_9  | GPIO_PIN_10 | GPIO_PIN_11 |
                              GPIO_PIN_12 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOD, &GPIO_Init_Structure);

    /* GPIOE: 7..15 (D4..D12) */
    GPIO_Init_Structure.Pin = GPIO_PIN_7  | GPIO_PIN_8  | GPIO_PIN_9  | GPIO_PIN_10 |
                              GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOE, &GPIO_Init_Structure);

    /* GPIOG: PG9 (NE2) */
    GPIO_Init_Structure.Pin = GPIO_PIN_9;
    HAL_GPIO_Init(GPIOG, &GPIO_Init_Structure);

    /* FMC SRAM (Bank2 / NE2) */
    hsram_fpga.Instance = FMC_NORSRAM_DEVICE;
    hsram_fpga.Extended = FMC_NORSRAM_EXTENDED_DEVICE;

    hsram_fpga.Init.NSBank             = FMC_NORSRAM_BANK2;
    hsram_fpga.Init.DataAddressMux     = FMC_DATA_ADDRESS_MUX_DISABLE;
    hsram_fpga.Init.MemoryType         = FMC_MEMORY_TYPE_SRAM;
    hsram_fpga.Init.MemoryDataWidth    = FMC_NORSRAM_MEM_BUS_WIDTH_16;
    hsram_fpga.Init.BurstAccessMode    = FMC_BURST_ACCESS_MODE_DISABLE;
    hsram_fpga.Init.WaitSignalPolarity = FMC_WAIT_SIGNAL_POLARITY_LOW;
    hsram_fpga.Init.WrapMode           = FMC_WRAP_MODE_DISABLE;
    hsram_fpga.Init.WaitSignalActive   = FMC_WAIT_TIMING_BEFORE_WS;
    hsram_fpga.Init.WriteOperation     = FMC_WRITE_OPERATION_ENABLE;
    hsram_fpga.Init.WaitSignal         = FMC_WAIT_SIGNAL_DISABLE;
    hsram_fpga.Init.ExtendedMode       = FMC_EXTENDED_MODE_DISABLE;
    hsram_fpga.Init.AsynchronousWait   = FMC_ASYNCHRONOUS_WAIT_DISABLE;
    hsram_fpga.Init.WriteBurst         = FMC_WRITE_BURST_DISABLE;
    hsram_fpga.Init.ContinuousClock    = FMC_CONTINUOUS_CLOCK_DISABLE;

    /* Conservative timings (FPGA sampling stability) */
    fpga_timing.AddressSetupTime      = 4;
    fpga_timing.AddressHoldTime       = 1;
    fpga_timing.DataSetupTime         = 9;
    fpga_timing.BusTurnAroundDuration = 1;
    fpga_timing.CLKDivision           = 2;
    fpga_timing.DataLatency           = 2;
    fpga_timing.AccessMode            = FMC_ACCESS_MODE_A;

    (void)HAL_SRAM_Init(&hsram_fpga, &fpga_timing, &fpga_timing);
}

void BSP_FPGA_Write(uint16_t RegIndex, uint16_t Data)
{
    BSP_FPGA_WRITE(RegIndex, Data);
}

uint16_t BSP_FPGA_Read(uint16_t RegIndex)
{
    return BSP_FPGA_READ(RegIndex);
}

void BSP_FPGA_ReadBuffer(uint16_t RegIndex, uint16_t *pBuf, uint32_t Count)
{
    volatile uint16_t *pReg =
        (volatile uint16_t *)((uint32_t)FPGA_BASE_ADDR + ((uint32_t)RegIndex << (uint32_t)FPGA_ADDR_SHIFT));

    while (Count--)
    {
        *pBuf++ = *pReg;
    }
}


