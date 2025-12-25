#include "bsp_fpga.h"
#include "stm32f4xx_hal.h"

/* 外部钩子函数声明（在 rtos_task.c 中实现） */
extern void BSP_FPGA_OnBlockReady(uint8_t buffer_index);

/* 全局双缓冲 DMA 缓冲区（供 RTOS 任务访问） */
uint16_t FPGA_DMA_Buffer_A[DMA_BUFFER_SIZE];
uint16_t FPGA_DMA_Buffer_B[DMA_BUFFER_SIZE];

static SRAM_HandleTypeDef hsram_fpga;
static FMC_NORSRAM_TimingTypeDef fpga_timing;
static uint8_t s_fpga_inited = 0;

/* DMA for FMC SRAM read (FPGA FIFO) */
static DMA_HandleTypeDef hdma_fpga_rx;

/* Ping-Pong 状态：1=使用 Buffer A，0=使用 Buffer B */
static uint8_t s_using_buffer_a = 1;

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
    __HAL_RCC_DMA2_CLK_ENABLE();

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
    hsram_fpga.Init.ContinuousClock    = FMC_CONTINUOUS_CLOCK_SYNC_ONLY;

    /* Safe timings for FPGA read operations (optimized for FIFO streaming)
     * - AddressSetupTime = 5: Sufficient time for FPGA to detect address change
     * - DataSetupTime = 9: Safe value for stable data output before STM32 latches
     */
    fpga_timing.AddressSetupTime      = 5;
    fpga_timing.AddressHoldTime       = 1;
    fpga_timing.DataSetupTime         = 9;
    fpga_timing.BusTurnAroundDuration = 1;
    fpga_timing.CLKDivision           = 2;
    fpga_timing.DataLatency           = 2;
    fpga_timing.AccessMode            = FMC_ACCESS_MODE_A;

    (void)HAL_SRAM_Init(&hsram_fpga, &fpga_timing, &fpga_timing);

    /* -------- DMA config for HAL_SRAM_Read_DMA --------
     * NOTE:
     * - HAL_SRAM_Read_DMA uses hsram->hdma and calls HAL_DMA_Start_IT()
     * - We configure: Fixed source (peripheral) + incrementing destination (memory)
     * - Data width: Halfword (16-bit)
     */
    hsram_fpga.hdma = &hdma_fpga_rx;
    hdma_fpga_rx.Instance = DMA2_Stream0;
    hdma_fpga_rx.Init.Channel = DMA_CHANNEL_0;
    /* FMC/FPGA FIFO 是“存储器映射地址”，没有 ADC 那种外设 DMA 请求线。
     * 如果配置成 DMA_PERIPH_TO_MEMORY，DMA 会等待外设请求而不启动（表现为 DMA 不通）。
     * 因此这里必须使用 Memory-to-Memory：源地址写到 DMA 的 PAR，目的地址写到 M0AR。
     */
    hdma_fpga_rx.Init.Direction = DMA_MEMORY_TO_MEMORY;
    hdma_fpga_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_fpga_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_fpga_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_fpga_rx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_fpga_rx.Init.Mode = DMA_NORMAL;
    hdma_fpga_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    hdma_fpga_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    /* 关联 DMA <-> SRAM 句柄（HAL_SRAM_Read_DMA 会用到 hsram_fpga.hdma） */
    __HAL_LINKDMA(&hsram_fpga, hdma, hdma_fpga_rx);

    (void)HAL_DMA_DeInit(&hdma_fpga_rx);
    (void)HAL_DMA_Init(&hdma_fpga_rx);

    /* DMA IRQ */
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
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

HAL_StatusTypeDef BSP_FPGA_Read_DMA(uint16_t RegIndex, uint16_t *pBuf, uint32_t Len)
{
    uint32_t fifo_addr;

    if (pBuf == 0 || Len == 0)
    {
        return HAL_ERROR;
    }

    /* Fixed source address: FPGA FIFO register */
    fifo_addr = (uint32_t)FPGA_BASE_ADDR + ((uint32_t)RegIndex << (uint32_t)FPGA_ADDR_SHIFT);

    return HAL_SRAM_Read_DMA(&hsram_fpga, (uint32_t *)fifo_addr, (uint32_t *)pBuf, Len);
}

/**
 * @brief 启动双缓冲（Ping-Pong）DMA 采集
 * 从 Buffer A 开始，DMA 完成后自动切换到 Buffer B，然后循环。
 */
void BSP_FPGA_Start_Acquisition(void)
{
    uint32_t fifo_addr;
    HAL_StatusTypeDef status;

    /* 计算 FPGA FIFO 固定地址 */
    fifo_addr = (uint32_t)FPGA_BASE_ADDR + ((uint32_t)FPGA_REG_FIFO << (uint32_t)FPGA_ADDR_SHIFT);

    /* 重置状态：从 Buffer A 开始 */
    s_using_buffer_a = 1;

    /* 启动第一次 DMA 传输到 Buffer A */
    status = HAL_SRAM_Read_DMA(&hsram_fpga, 
                                (uint32_t *)fifo_addr, 
                                (uint32_t *)FPGA_DMA_Buffer_A, 
                                DMA_BUFFER_SIZE);
    
    if (status != HAL_OK)
    {
        /* 如果启动失败，触发错误回调 */
        BSP_FPGA_DMA_Error_Callback();
    }
}

void BSP_FPGA_DMA_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_fpga_rx);
}

__weak void BSP_FPGA_DMA_Complete_Callback(void)
{
}

__weak void BSP_FPGA_DMA_Error_Callback(void)
{
}

void HAL_SRAM_DMA_XferCpltCallback(DMA_HandleTypeDef *hdma)
{
    uint32_t fifo_addr;
    HAL_StatusTypeDef status;

    if (hdma == &hdma_fpga_rx)
    {
        /* 计算 FPGA FIFO 固定地址 */
        fifo_addr = (uint32_t)FPGA_BASE_ADDR + ((uint32_t)FPGA_REG_FIFO << (uint32_t)FPGA_ADDR_SHIFT);

        /* Ping-Pong 切换逻辑 */
        if (s_using_buffer_a)
        {
            /* Buffer A 已填满，通知 RTOS 任务处理 Buffer A */
            BSP_FPGA_OnBlockReady(0);  /* 0 = Buffer A */

            /* 立即启动 DMA 传输到 Buffer B */
            s_using_buffer_a = 0;
            status = HAL_SRAM_Read_DMA(&hsram_fpga, 
                                        (uint32_t *)fifo_addr, 
                                        (uint32_t *)FPGA_DMA_Buffer_B, 
                                        DMA_BUFFER_SIZE);
            
            if (status != HAL_OK)
            {
                BSP_FPGA_DMA_Error_Callback();
            }
        }
        else
        {
            /* Buffer B 已填满，通知 RTOS 任务处理 Buffer B */
            BSP_FPGA_OnBlockReady(1);  /* 1 = Buffer B */

            /* 立即启动 DMA 传输到 Buffer A */
            s_using_buffer_a = 1;
            status = HAL_SRAM_Read_DMA(&hsram_fpga, 
                                        (uint32_t *)fifo_addr, 
                                        (uint32_t *)FPGA_DMA_Buffer_A, 
                                        DMA_BUFFER_SIZE);
            
            if (status != HAL_OK)
            {
                BSP_FPGA_DMA_Error_Callback();
            }
        }
    }
}

void HAL_SRAM_DMA_XferErrorCallback(DMA_HandleTypeDef *hdma)
{
    if (hdma == &hdma_fpga_rx)
    {
        BSP_FPGA_DMA_Error_Callback();
    }
}


