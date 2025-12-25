#ifndef BSP_FPGA_H
#define BSP_FPGA_H

#include <stdint.h>

/**
 * @brief FSMC/FMC Bank2 (NE2) 基地址
 * - CS: FMC_NE2 (PG9)
 * - Base: 0x64000000
 */
#define FPGA_BASE_ADDR      (0x64000000u)

/**
 * @brief 地址偏移（A16 “移位”）
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
 * @brief 16-bit 寄存器访问宏（异步 SRAM/FMC）
 */
#define BSP_FPGA_WRITE(Idx, Val) \
    (*((volatile uint16_t *)((uint32_t)FPGA_BASE_ADDR + ((uint32_t)(Idx) << (uint32_t)FPGA_ADDR_SHIFT))) = (uint16_t)(Val))

#define BSP_FPGA_READ(Idx) \
    (*((volatile uint16_t *)((uint32_t)FPGA_BASE_ADDR + ((uint32_t)(Idx) << (uint32_t)FPGA_ADDR_SHIFT))))

void BSP_FPGA_Init(void);
void BSP_FPGA_Write(uint16_t RegIndex, uint16_t Data);
uint16_t BSP_FPGA_Read(uint16_t RegIndex);
void BSP_FPGA_ReadBuffer(uint16_t RegIndex, uint16_t *pBuf, uint32_t Count);

#endif /* BSP_FPGA_H */


