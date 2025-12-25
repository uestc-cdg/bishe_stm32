#include "dsp_tester.h"

#include <math.h>

volatile uint8_t g_UseSimSource = 1;  /* 默认使用模拟源，安全模式 */
volatile uint8_t g_SendProcessed = 1;

int32_t Test_Sim_I[DSP_TEST_BLOCK_SIZE];
int32_t Test_Sim_Q[DSP_TEST_BLOCK_SIZE];

/* 模拟数据缓冲（打包的 uint16_t 格式，用于 DSP_Process_Block） */
uint16_t Test_Sim_Buffer[LIA_BLOCK_SIZE * 4u];

/* 简单伪随机（LCG）用于噪声 */
static uint32_t s_rng = 0x12345678u;
static int32_t rand_noise(int32_t amp)
{
    /* 32-bit LCG */
    s_rng = (1103515245u * s_rng + 12345u);
    /* 取高位作为随机 */
    {
        int32_t r = (int32_t)(s_rng >> 16);
        /* 映射到 [-amp, amp] */
        int32_t v = r % (amp + 1);
        if ((s_rng & 1u) != 0u) v = -v;
        return v;
    }
}

void DSP_Tester_Init(void)
{
    uint32_t n;
    /* 模拟信号：正弦 + 噪声
     * - I = A*sin(w*n) + noise
     * - Q = A*cos(w*n) + noise
     */
    const float w = 2.0f * 3.1415926f * (1.0f / 64.0f); /* 每 64 点一个周期 */
    const int32_t A = (int32_t)(0.6f * 2147483647.0f);
    const int32_t N = (int32_t)(0.02f * 2147483647.0f);

    for (n = 0; n < DSP_TEST_BLOCK_SIZE; n++)
    {
        float s = sinf(w * (float)n);
        float c = cosf(w * (float)n);
        int32_t i = (int32_t)(s * (float)A) + rand_noise(N);
        int32_t q = (int32_t)(c * (float)A) + rand_noise(N);
        Test_Sim_I[n] = i;
        Test_Sim_Q[n] = q;
    }
    
    /* 将 int32_t I/Q 打包成 uint16_t 格式（用于 DSP_Process_Block） */
    for (n = 0; n < LIA_BLOCK_SIZE; n++)
    {
        uint32_t base_idx = n * 4u;
        int32_t i_val = Test_Sim_I[n];
        int32_t q_val = Test_Sim_Q[n];
        
        /* 打包格式：[I_low, I_high, Q_low, Q_high] */
        Test_Sim_Buffer[base_idx + 0] = (uint16_t)(i_val & 0xFFFFu);
        Test_Sim_Buffer[base_idx + 1] = (uint16_t)((i_val >> 16) & 0xFFFFu);
        Test_Sim_Buffer[base_idx + 2] = (uint16_t)(q_val & 0xFFFFu);
        Test_Sim_Buffer[base_idx + 3] = (uint16_t)((q_val >> 16) & 0xFFFFu);
    }
}

/**
 * @brief 获取模拟数据缓冲区（打包格式）
 * @return 指向模拟数据缓冲区的指针
 */
uint16_t* DSP_Tester_Get_Sim_Buffer(void)
{
    return Test_Sim_Buffer;
}

void DSP_Tester_Calc(int32_t i, int32_t q, float *r, float *theta)
{
    int32_t inI[1];
    int32_t inQ[1];
    LIA_Packet_t out;

    if (r == 0 || theta == 0)
        return;

    /* 重置 DSP 状态，保证单点“即时”输出（第一个点不会走 EMA 递推） */
    DSP_Core_Init();

    inI[0] = i;
    inQ[0] = q;

    DSP_Process_Block_Legacy(inI, inQ, &out, 1u);

    *r = out.R;
    *theta = out.Theta;
}


