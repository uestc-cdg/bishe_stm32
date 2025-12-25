#include "dsp_core.h"
#include "bsp_fpga.h"  /* 用于 LIA_BLOCK_SIZE */

#include "arm_math.h"

#include <math.h>
#include <string.h>

/* ---------- 参数/资源限制（静态内存，适合裸机/RTOS） ---------- */
#define DSP_MAX_BLOCK_SIZE   (2048u)
#define FIR_TAP_NUM          (21u)  /* FIR 抽头数（占位，后续替换为 CIC 补偿系数） */
#define BLOCK_SIZE           (LIA_BLOCK_SIZE)  /* 1024 样本 */

/* FIR 占位系数：中心为 1.0，其余为 0（占位，后续替换为 CIC 补偿系数） */
static const float32_t firCoeffs[FIR_TAP_NUM] =
{
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f,  /* 中心抽头 */
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

/* 旧 API 的 FIR 系数（保留兼容性） */
#define FIR_TAP_NUM_OLD  (16u)
static const float s_fir_coeff[FIR_TAP_NUM_OLD] =
{
    1.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f,
    1.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f,
    1.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f,
    1.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f
};

/* FIR 滤波器实例 */
static arm_fir_instance_f32 S_FIR_I;
static arm_fir_instance_f32 S_FIR_Q;

/* FIR 状态缓冲区（长度 = numTaps + blockSize - 1） */
static float32_t pStateI[FIR_TAP_NUM + BLOCK_SIZE - 1u];
static float32_t pStateQ[FIR_TAP_NUM + BLOCK_SIZE - 1u];

/* Phase 1: Unpacking & Normalization 输出缓冲区 */
static float32_t I_Float_Buf[BLOCK_SIZE];
static float32_t Q_Float_Buf[BLOCK_SIZE];

/* Phase 2: FIR 补偿输出缓冲区 */
static float32_t I_Filt_Buf[BLOCK_SIZE];
static float32_t Q_Filt_Buf[BLOCK_SIZE];

/* 旧 API 的 FIR 实例和缓冲区（保留兼容性，使用较小缓冲区以节省 RAM） */
#define LEGACY_BLOCK_SIZE  (512u)  /* 旧 API 使用较小的块大小以节省内存 */
static arm_fir_instance_f32 s_fir_I;
static arm_fir_instance_f32 s_fir_Q;
static float s_fir_state_I[FIR_TAP_NUM_OLD + LEGACY_BLOCK_SIZE - 1u];
static float s_fir_state_Q[FIR_TAP_NUM_OLD + LEGACY_BLOCK_SIZE - 1u];
static float s_norm_I[LEGACY_BLOCK_SIZE];
static float s_norm_Q[LEGACY_BLOCK_SIZE];
static float s_fir_out_I[LEGACY_BLOCK_SIZE];
static float s_fir_out_Q[LEGACY_BLOCK_SIZE];

/* ---------- 配置/状态 ---------- */
static DSP_Config_t s_cfg;
static uint8_t s_inited = 0;

/* IIR(EMA) 状态（用于新 API） */
static float prev_R = 0.0f;
static float prev_Theta = 0.0f;
static float alpha = 0.1f;

/* 旧 API 的 IIR 状态（保留兼容性） */
static float s_R_filt = 0.0f;
static float s_Theta_filt = 0.0f;
static uint8_t s_has_state = 0;

/* 角度处理辅助 */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define TWO_PI (2.0f * (float)M_PI)

static float wrap_pi(float x)
{
    while (x > (float)M_PI)  x -= TWO_PI;
    while (x < -(float)M_PI) x += TWO_PI;
    return x;
}

/**
 * @brief 初始化 DSP 引擎（新 API）
 */
void DSP_Init(void)
{
    /* 默认时间常数 */
    alpha = 0.1f;
    prev_R = 0.0f;
    prev_Theta = 0.0f;

    /* 初始化 FIR 滤波器（注意：state buffer 长度 = numTaps + blockSize - 1） */
    (void)arm_fir_init_f32(&S_FIR_I, FIR_TAP_NUM, (float32_t *)firCoeffs, pStateI, BLOCK_SIZE);
    (void)arm_fir_init_f32(&S_FIR_Q, FIR_TAP_NUM, (float32_t *)firCoeffs, pStateQ, BLOCK_SIZE);

    s_inited = 1;
}

/**
 * @brief 初始化 DSP 引擎（旧 API，保留兼容性）
 */
void DSP_Core_Init(void)
{
    /* 默认时间常数 */
    s_cfg.alpha = 0.05f;

    /* 初始化 FIR（注意：state buffer 长度 = numTaps + blockSize - 1） */
    (void)arm_fir_init_f32(&s_fir_I, FIR_TAP_NUM_OLD, (float *)s_fir_coeff, s_fir_state_I, LEGACY_BLOCK_SIZE);
    (void)arm_fir_init_f32(&s_fir_Q, FIR_TAP_NUM_OLD, (float *)s_fir_coeff, s_fir_state_Q, LEGACY_BLOCK_SIZE);

    s_R_filt = 0.0f;
    s_Theta_filt = 0.0f;
    s_has_state = 0;

    s_inited = 1;
}

void DSP_Set_TimeConstant(float alpha_val)
{
    if (alpha_val > 0.0f && alpha_val < 1.0f)
    {
        alpha = alpha_val;
        s_cfg.alpha = alpha_val;  /* 同时更新旧 API 的配置 */
    }
}

/**
 * @brief 处理一块打包的 I/Q 数据（完整 DSP 处理链 - 新 API）
 * @param pInput     输入：DMA 缓冲区，uint16_t 数组（打包的 32-bit I/Q）
 * @param num_samples 样本数量
 * @param pOutput    输出：块平均结果（R, Theta）
 */
void DSP_Process_Block(uint16_t *pInput, uint32_t num_samples, LIA_Result_t *pOutput)
{
    uint32_t i;
    int32_t I_raw, Q_raw;
    float32_t R_inst, Theta_inst;
    float32_t sum_R = 0.0f;
    float32_t sum_Theta = 0.0f;
    float32_t one_minus_alpha;

    if (!s_inited)
    {
        DSP_Init();
    }
    if (pInput == 0 || pOutput == 0 || num_samples == 0)
    {
        return;
    }
    if (num_samples > BLOCK_SIZE)
    {
        /* 超出静态缓冲区能力，直接返回 */
        return;
    }

    one_minus_alpha = 1.0f - alpha;

    /* ========== Phase 1: Unpacking & Normalization ========== */
    for (i = 0; i < num_samples; i++)
    {
        uint32_t base_idx = i * 4u;  /* 每个样本 = 4 个 16-bit 字 */

        /* 解包 32-bit I: [I_low, I_high] */
        I_raw = (int32_t)((uint32_t)pInput[base_idx + 1] << 16) | (uint32_t)pInput[base_idx + 0];

        /* 解包 32-bit Q: [Q_low, Q_high] */
        Q_raw = (int32_t)((uint32_t)pInput[base_idx + 3] << 16) | (uint32_t)pInput[base_idx + 2];

        /* 转换为 float（归一化，假设 fullscale = 2^31） */
        I_Float_Buf[i] = (float32_t)I_raw / 2147483648.0f;
        Q_Float_Buf[i] = (float32_t)Q_raw / 2147483648.0f;
    }

    /* ========== Phase 2: FIR Compensation (The Correction) ========== */
    arm_fir_f32(&S_FIR_I, I_Float_Buf, I_Filt_Buf, num_samples);
    arm_fir_f32(&S_FIR_Q, Q_Float_Buf, Q_Filt_Buf, num_samples);

    /* ========== Phase 3: Vector & IIR (The Measurement) ========== */
    for (i = 0; i < num_samples; i++)
    {
        float32_t I_val = I_Filt_Buf[i];
        float32_t Q_val = Q_Filt_Buf[i];
        float32_t mag2;

        /* Vector: R = sqrt(I^2 + Q^2) */
        mag2 = I_val * I_val + Q_val * Q_val;
        (void)arm_sqrt_f32(mag2, &R_inst);

        /* Vector: Theta = atan2(Q, I) */
        Theta_inst = atan2f(Q_val, I_val);

        /* IIR: 一阶低通滤波 */
        prev_R = alpha * R_inst + one_minus_alpha * prev_R;

        /* Theta 相位展开（避免跨 ±π 时 EMA 抖动） */
        {
            float32_t diff = Theta_inst - prev_Theta;
            if (diff > 3.14159265358979323846f)
            {
                Theta_inst -= 6.28318530717958647692f;  /* 2*PI */
            }
            else if (diff < -3.14159265358979323846f)
            {
                Theta_inst += 6.28318530717958647692f;  /* 2*PI */
            }
        }
        prev_Theta = alpha * Theta_inst + one_minus_alpha * prev_Theta;

        /* 累加用于计算块平均值 */
        sum_R += prev_R;
        sum_Theta += prev_Theta;
    }

    /* ========== Phase 4: Output Update (Block Mean) ========== */
    pOutput->R = sum_R / (float32_t)num_samples;
    pOutput->Theta = sum_Theta / (float32_t)num_samples;
}

/**
 * @brief 处理一块 I/Q 数据（旧 API，保留兼容性，已重命名避免函数重载冲突）
 */
void DSP_Process_Block_Legacy(int32_t *in_I, int32_t *in_Q, LIA_Packet_t *out_result, uint32_t blockSize)
{
    uint32_t i;
    float meanI = 0.0f;
    float meanQ = 0.0f;
    float alpha;
    float one_minus_alpha;
    float last_I = 0.0f;
    float last_Q = 0.0f;
    float last_R = 0.0f;
    float last_Theta = 0.0f;

    if (!s_inited)
    {
        DSP_Core_Init();
    }
    if (in_I == 0 || in_Q == 0 || out_result == 0 || blockSize == 0)
    {
        return;
    }
    if (blockSize > LEGACY_BLOCK_SIZE)
    {
        /* 超出静态缓冲区能力，直接返回（可按需改为分块处理） */
        return;
    }

    alpha = s_cfg.alpha;
    one_minus_alpha = 1.0f - alpha;

    /* -------- Stage 1: 归一化 + 简单 DC 去除（按块求均值） -------- */
    for (i = 0; i < blockSize; i++)
    {
        meanI += (float)in_I[i];
        meanQ += (float)in_Q[i];
    }
    meanI /= (float)blockSize;
    meanQ /= (float)blockSize;

    /* int32 -> float 归一化（占位：按 fullscale=2^31） */
    {
        const float scale = 1.0f / 2147483648.0f;
        for (i = 0; i < blockSize; i++)
        {
            s_norm_I[i] = ((float)in_I[i] - meanI) * scale;
            s_norm_Q[i] = ((float)in_Q[i] - meanQ) * scale;
        }
    }

    /* -------- Stage 2: CIC Compensation Placeholder (FIR) -------- */
    arm_fir_f32(&s_fir_I, s_norm_I, s_fir_out_I, blockSize);
    arm_fir_f32(&s_fir_Q, s_norm_Q, s_fir_out_Q, blockSize);

    /* -------- Stage 3/4: R/Theta + 一阶 IIR 低通（EMA） -------- */
    for (i = 0; i < blockSize; i++)
    {
        float I = s_fir_out_I[i];
        float Q = s_fir_out_Q[i];
        float mag2 = I * I + Q * Q;
        float R;
        float Theta;

        /* R = sqrt(I^2 + Q^2) */
        (void)arm_sqrt_f32(mag2, &R);

        /* Theta = atan2(Q, I) */
        Theta = atan2f(Q, I);

        if (!s_has_state)
        {
            s_R_filt = R;
            s_Theta_filt = Theta;
            s_has_state = 1;
        }
        else
        {
            /* R 直接 EMA */
            s_R_filt = alpha * R + one_minus_alpha * s_R_filt;

            /* Theta 做简单 unwrap，避免跨 ±pi 时 EMA 抖动 */
            {
                float x = Theta;
                float diff = x - s_Theta_filt;
                if (diff > (float)M_PI)  x -= TWO_PI;
                if (diff < -(float)M_PI) x += TWO_PI;
                s_Theta_filt = alpha * x + one_minus_alpha * s_Theta_filt;
                s_Theta_filt = wrap_pi(s_Theta_filt);
            }
        }

        last_I = I;
        last_Q = Q;
        last_R = s_R_filt;
        last_Theta = s_Theta_filt;
    }

    out_result->I = last_I;
    out_result->Q = last_Q;
    out_result->R = last_R;
    out_result->Theta = last_Theta;
}


