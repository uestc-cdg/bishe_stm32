#ifndef BSP_FPGA_H
#define BSP_FPGA_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/**
 * @brief FSMC/FMC Bank2 (NE2) 基地址
 * - CS: FMC_NE2 (PG9)
 * - Base: 0x64000000
 */
#define FPGA_BASE_ADDR      (0x64000000u)

/**
 * @brief 地址偏移（A16 "移位"）
 * STM32 FMC_A16(PD11) 物理连接到 FPGA ADDR[0]。
 * 16-bit 模式下，外部 A16 对应 HADDR[17]，因此寄存器索引需要 <<17。
 *
 * Physical_Addr = Base + (Reg_Index << 17)
 */
#define FPGA_ADDR_SHIFT     (17u)

/* 兼容/示例：若上层用到某些寄存器索引，可在此集中定义 */
#ifndef FPGA_REG_LED_CTRL
#define FPGA_REG_LED_CTRL   (0x02u)
#endif

/**
 * @brief FPGA FIFO 寄存器索引
 */
#define FPGA_REG_FIFO       (0x01u)

/**
 * @brief Lock-In Amplifier 块大小（每个块包含的 I/Q 样本数）
 */
#define LIA_BLOCK_SIZE      (1024u)

/**
 * @brief DMA 缓冲区大小（16-bit halfwords）
 * 每个 I/Q 样本 = 4 个 16-bit 字（I_low, I_high, Q_low, Q_high）
 */
#define DMA_BUFFER_SIZE     (LIA_BLOCK_SIZE * 4u)

/**
 * @brief 16-bit 寄存器访问宏（异步 SRAM/FMC）
 */
#define BSP_FPGA_WRITE(Idx, Val) \
    (*((volatile uint16_t *)((uint32_t)FPGA_BASE_ADDR + ((uint32_t)(Idx) << (uint32_t)FPGA_ADDR_SHIFT))) = (uint16_t)(Val))

#define BSP_FPGA_READ(Idx) \
    (*((volatile uint16_t *)((uint32_t)FPGA_BASE_ADDR + ((uint32_t)(Idx) << (uint32_t)FPGA_ADDR_SHIFT))))

/**
 * @brief 双缓冲 DMA 缓冲区（全局，供 RTOS 任务访问）
 * 每个缓冲区大小：DMA_BUFFER_SIZE (4096 个 16-bit halfwords)
 */
extern uint16_t FPGA_DMA_Buffer_A[DMA_BUFFER_SIZE];
extern uint16_t FPGA_DMA_Buffer_B[DMA_BUFFER_SIZE];

void BSP_FPGA_Init(void);
void BSP_FPGA_Write(uint16_t RegIndex, uint16_t Data);
uint16_t BSP_FPGA_Read(uint16_t RegIndex);
void BSP_FPGA_ReadBuffer(uint16_t RegIndex, uint16_t *pBuf, uint32_t Count);

/**
 * @brief 从 FPGA FIFO 读取一段数据（DMA，单次传输）
 * @param RegIndex FIFO 寄存器偏移（例如 FIFO=1）
 * @param pBuf     目标缓冲区（16-bit）
 * @param Len      16-bit halfword 数量
 */
HAL_StatusTypeDef BSP_FPGA_Read_DMA(uint16_t RegIndex, uint16_t *pBuf, uint32_t Len);

/**
 * @brief 启动双缓冲（Ping-Pong）DMA 采集
 * 启动后，DMA 会在 Buffer A 和 Buffer B 之间自动切换。
 * 每次完成一个缓冲区的传输后，会触发回调通知 RTOS 任务。
 */
void BSP_FPGA_Start_Acquisition(void);

/* DMA IRQ 入口（由中断服务函数调用） */
void BSP_FPGA_DMA_IRQHandler(void);

/* DMA 完成/错误回调钩子（默认弱实现，任务层可重写用于释放信号量） */
void BSP_FPGA_DMA_Complete_Callback(void);
void BSP_FPGA_DMA_Error_Callback(void);

#endif /* BSP_FPGA_H */


