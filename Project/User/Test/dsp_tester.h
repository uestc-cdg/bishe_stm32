#ifndef __DSP_TESTER_H
#define __DSP_TESTER_H

#include <stdint.h>

#include "dsp_core.h"
#include "bsp_fpga.h"  /* 用于 LIA_BLOCK_SIZE */

/* 与 DataStream 的 blockSize 保持一致（验证阶段固定） */
#define DSP_TEST_BLOCK_SIZE   (1024u)

/* 0=FPGA 源，1=模拟源 */
extern volatile uint8_t g_UseSimSource;
/* 0=输出 Raw I/Q，1=输出 Processed R/Theta */
extern volatile uint8_t g_SendProcessed;

/* 模拟数据缓冲（int32 I/Q） */
extern int32_t Test_Sim_I[DSP_TEST_BLOCK_SIZE];
extern int32_t Test_Sim_Q[DSP_TEST_BLOCK_SIZE];

/* 模拟数据缓冲（打包的 uint16_t 格式，用于 DSP_Process_Block） */
extern uint16_t Test_Sim_Buffer[LIA_BLOCK_SIZE * 4u];

void DSP_Tester_Init(void);

/**
 * @brief 获取模拟数据缓冲区（打包格式）
 * @return 指向模拟数据缓冲区的指针（uint16_t 数组，格式：[I_low, I_high, Q_low, Q_high] * N）
 */
uint16_t* DSP_Tester_Get_Sim_Buffer(void);

/**
 * @brief 单点计算（用于 :TEST:CALC <I> <Q>）
 * @note  内部会重置 DSP 状态，确保返回的是该点的即时 R/Theta（不受前序滤波状态影响）
 */
void DSP_Tester_Calc(int32_t i, int32_t q, float *r, float *theta);

#endif /* __DSP_TESTER_H */


