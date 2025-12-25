#include "scpi_callbacks.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp_fpga.h"
#include "dsp_core.h"
#include "dsp_tester.h"  /* 用于 g_UseSimSource */
#include "rtos_task.h"   /* 用于 USB_Send_Safe 声明 */

/* 外部全局变量（在 rtos_task.c 中定义） */
extern volatile uint8_t g_AcquisitionRunning;
extern volatile uint8_t g_DataOutputMode;
extern volatile uint8_t g_UseSimSource;

/* 仅用于通知数据任务：实现文件里再包含 FreeRTOS，避免污染解析层 */
#include "FreeRTOS.h"
#include "task.h"

/* ------- 注入/全局依赖 ------- */
static SCPI_SendLineFn s_send_line = 0;
static TaskHandle_t s_data_task = 0;

void SCPI_Callbacks_Init(SCPI_SendLineFn send_fn)
{
    s_send_line = send_fn;
}

void SCPI_SetDataTaskHandle(void *task_handle)
{
    s_data_task = (TaskHandle_t)task_handle;
}

/**
 * @brief  SCPI回复函数（线程安全）
 * @param  fmt: 格式化字符串
 * @retval None
 * 
 * 功能：格式化字符串并通过USB发送，使用USB_Send_Safe确保线程安全
 */
static void scpi_reply(const char *fmt, ...)
{
    char buf[128];
    va_list ap;

    if (fmt == NULL)
    {
        return;
    }

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    /* 使用线程安全的USB发送函数 */
    USB_Send_Safe((uint8_t *)buf, strlen(buf));
}

/* ------- 具体命令回调 ------- */

/**
 * @brief  *IDN? 命令 - 返回设备标识
 */
static void SCPI_Cmd_IDN(char *args)
{
    (void)args;
    scpi_reply("*IDN STM32-LIA,STM32F429IGT6,SCPI,1.0\r\n");
}

/**
 * @brief  :32ID? 命令 - 返回32位ID
 */
static void SCPI_Cmd_32ID(char *args)
{
    (void)args;
    /* 读取CPU ID（STM32F429的96位唯一ID，取低32位） */
    uint32_t id_low = *(volatile uint32_t *)0x1FFF7A10;
    uint32_t id_mid = *(volatile uint32_t *)0x1FFF7A14;
    uint32_t id_high = *(volatile uint32_t *)0x1FFF7A18;
    /* 组合成32位ID（取低32位） */
    uint32_t device_id = id_low;
    scpi_reply("32ID:0x%08lX\r\n", (unsigned long)device_id);
}

/**
 * @brief  :FPGA:ID? 命令 - 读取FPGA ID
 */
static void SCPI_Cmd_FPGA_ID(char *args)
{
    uint16_t id;
    (void)args;
    id = BSP_FPGA_Read(0);
    scpi_reply("FPGA_ID:0x%04X\r\n", id);
}

/**
 * @brief  :FPGA:LED 命令 - 设置FPGA LED
 */
static void SCPI_Cmd_FPGA_LED_Set(char *args)
{
    int v = 0;
    if (args && *args)
    {
        v = atoi(args);
    }
    if (v < 0) v = 0;
    if (v > 7) v = 7;

    BSP_FPGA_Write(FPGA_REG_LED_CTRL, (uint16_t)v);
    scpi_reply("OK\r\n");
}

/**
 * @brief  :FPGA:LED? 命令 - 查询FPGA LED状态
 */
static void SCPI_Cmd_FPGA_LED_Query(char *args)
{
    uint16_t v;
    (void)args;
    v = BSP_FPGA_Read(FPGA_REG_LED_CTRL);
    scpi_reply("FPGA_LED:%d\r\n", v);
}

/**
 * @brief  :ACQ:START 命令 - 启动采集
 */
static void SCPI_Cmd_StartAcq(char *args)
{
    (void)args;
    /* 启动采集 */
    g_AcquisitionRunning = 1;
    BSP_FPGA_Start_Acquisition();
    scpi_reply("ACQ STARTED\r\n");
}

/**
 * @brief  :ACQ:STOP 命令 - 停止采集
 */
static void SCPI_Cmd_StopAcq(char *args)
{
    (void)args;
    /* 停止采集 */
    g_AcquisitionRunning = 0;
    /* 注意：BSP_FPGA_Start_Acquisition 启动的DMA会持续运行，
       停止时只需要设置标志，Task_DSP会停止处理数据 */
    scpi_reply("ACQ STOPPED\r\n");
}

/**
 * @brief  :FREQ 命令 - 设置频率（已废弃，使用:FPGA:FREQ）
 */
static void SCPI_Cmd_SetFreq(char *args)
{
    int v = 0;
    if (args && *args)
    {
        v = atoi(args);
    }
    BSP_FPGA_Write(3, (uint16_t)v);
    scpi_reply("OK\r\n");
}

/**
 * @brief  :DATA:MODE 命令 - 设置数据输出模式
 * @param  args: 0=结果模式(R/Theta), 1=原始模式(I/Q)
 */
static void SCPI_Cmd_DataMode(char *args)
{
    int v = 0;
    if (args && *args)
    {
        v = atoi(args);
    }
    g_DataOutputMode = (v != 0) ? 1u : 0u;
    scpi_reply("OK\r\n");
}

/**
 * @brief  :DSP:TC 命令 - 设置DSP时间常数
 * @param  args: 时间常数alpha值 (0.001-0.999)
 */
static void SCPI_Cmd_DSP_TC(char *args)
{
    float alpha = 0.1f;
    if (args && *args)
    {
        alpha = (float)atof(args);
    }
    /* 限制范围 */
    if (alpha <= 0.0f) alpha = 0.001f;
    if (alpha >= 1.0f) alpha = 0.999f;
    
    DSP_Set_TimeConstant(alpha);
    scpi_reply("OK\r\n");
}

/**
 * @brief  :FPGA:FREQ 命令 - 设置FPGA频率寄存器
 * @param  args: 频率值（32位，拆分为低16位和高16位写入）
 */
static void SCPI_Cmd_FPGA_Freq(char *args)
{
    uint32_t freq = 0;
    if (args && *args)
    {
        freq = (uint32_t)atoi(args);
    }
    /* 写入低16位到寄存器3 */
    BSP_FPGA_Write(3, (uint16_t)(freq & 0xFFFF));
    /* 写入高16位到寄存器4 */
    BSP_FPGA_Write(4, (uint16_t)((freq >> 16) & 0xFFFF));
    scpi_reply("OK\r\n");
}

/**
 * @brief  :TEST:SOURCE 命令 - 设置测试数据源
 * @param  args: 0=硬件FPGA, 1=模拟数据源
 */
static void SCPI_Cmd_TestSource(char *args)
{
    int v = 0;
    if (args && *args)
    {
        v = atoi(args);
    }
    g_UseSimSource = (v != 0) ? 1u : 0u;
    scpi_reply("OK\r\n");
}

/* ------- 命令表 ------- */
const SCPI_Entry_t scpi_commands[] =
{
    { "*IDN?",       SCPI_Cmd_IDN },
    { ":32ID?",      SCPI_Cmd_32ID },
    { ":FPGA:ID?",   SCPI_Cmd_FPGA_ID },
    { ":FPGA:LED",   SCPI_Cmd_FPGA_LED_Set },
    { ":FPGA:LED?",  SCPI_Cmd_FPGA_LED_Query },
    { ":FREQ",       SCPI_Cmd_SetFreq },
    { ":ACQ:START",  SCPI_Cmd_StartAcq },
    { ":ACQ:STOP",   SCPI_Cmd_StopAcq },
    { ":DATA:MODE",  SCPI_Cmd_DataMode },
    { ":DSP:TC",     SCPI_Cmd_DSP_TC },
    { ":FPGA:FREQ",  SCPI_Cmd_FPGA_Freq },
    { ":TEST:SOURCE", SCPI_Cmd_TestSource },
};

const uint32_t scpi_commands_count = (uint32_t)(sizeof(scpi_commands) / sizeof(scpi_commands[0]));


