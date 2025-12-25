#ifndef __DSP_CORE_H
#define __DSP_CORE_H

#include <stdint.h>

/* Lock-In Amplifier 输出包（保留兼容性） */
typedef struct
{
    float I;
    float Q;
    float R;
    float Theta;
} LIA_Packet_t;

/* Lock-In Amplifier 块平均结果（用于完整 DSP 处理引擎） */
typedef struct
{
    float R;      /* 块平均幅度 */
    float Theta;  /* 块平均相位 */
} LIA_Result_t;

/* DSP 配置（可扩展：FIR/CIC 补偿参数等） */
typedef struct
{
    float alpha; /* 0<alpha<1，一阶 IIR 低通时间常数 */
} DSP_Config_t;

/**
 * @brief 初始化 DSP 引擎（FIR 滤波器、IIR 状态等）
 */
void DSP_Init(void);

/**
 * @brief 设置 IIR 时间常数
 * @param alpha 时间常数（0 < alpha < 1）
 */
void DSP_Set_TimeConstant(float alpha);

/**
 * @brief 处理一块打包的 I/Q 数据（完整 DSP 处理链）
 * @param pInput     输入：DMA 缓冲区，uint16_t 数组（打包的 32-bit I/Q）
 *                   格式：每个样本 = 4 个 16-bit 字 [I_low, I_high, Q_low, Q_high]
 * @param num_samples 样本数量（例如 1024）
 * @param pOutput    输出：块平均结果（R, Theta）
 */
void DSP_Process_Block(uint16_t *pInput, uint32_t num_samples, LIA_Result_t *pOutput);

/* 兼容旧 API（保留，已重命名避免函数重载冲突） */
void DSP_Core_Init(void);
void DSP_Process_Block_Legacy(int32_t *in_I, int32_t *in_Q, LIA_Packet_t *out_result, uint32_t blockSize);

#endif /* __DSP_CORE_H */


