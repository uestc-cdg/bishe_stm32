#include "scpi_callbacks.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp_fpga.h"
#include "dsp_core.h"
#include "dsp_tester.h"  /* 用于 g_UseSimSource */

/* 外部全局变量（在 rtos_task.c 中定义） */
extern volatile uint8_t g_AcquisitionRunning;
extern volatile uint8_t g_DataOutputMode;

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

static void scpi_reply(const char *fmt, ...)
{
    char buf[128];
    va_list ap;

    if (s_send_line == 0)
        return;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    s_send_line(buf);
}

/* ------- 具体命令回调 ------- */
static void SCPI_Cmd_IDN(char *args)
{
    (void)args;
    scpi_reply("STM32F429IGT6,SCPI,1.0\r\n");
}

static void SCPI_Cmd_FPGA_ID(char *args)
{
    uint16_t id;
    (void)args;
    id = BSP_FPGA_Read(0);
    scpi_reply("FPGA_ID:0x%04X\r\n", id);
}

static void SCPI_Cmd_FPGA_LED_Set(char *args)
{
    int v = 0;
    if (args && *args)
    {
        v = atoi(args);
    }
    if (v < 0) v = 0;
    if (v > 7) v = 7;

    BSP_FPGA_Write(2, (uint16_t)v);
    scpi_reply("OK\r\n");
}

static void SCPI_Cmd_FPGA_LED_Query(char *args)
{
    uint16_t v;
    (void)args;
    v = BSP_FPGA_Read(2);
    scpi_reply("FPGA_LED:%d\r\n", v);
}

static void SCPI_Cmd_StartAcq(char *args)
{
    (void)args;
    /* 启动采集 */
    g_AcquisitionRunning = 1;
    BSP_FPGA_Start_Acquisition();
    scpi_reply("OK\r\n");
}

static void SCPI_Cmd_StopAcq(char *args)
{
    (void)args;
    /* 停止采集 */
    g_AcquisitionRunning = 0;
    /* TODO: 实现 BSP_FPGA_Stop() 函数 */
    scpi_reply("OK\r\n");
}

/* 示例：:FREQ <val> -> 写 FPGA 某寄存器（这里用寄存器 3 占位） */
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

static void SCPI_Cmd_FPGA_Freq(char *args)
{
    int v = 0;
    if (args && *args)
    {
        v = atoi(args);
    }
    /* 假设频率寄存器在索引 3 */
    BSP_FPGA_Write(3, (uint16_t)v);
    scpi_reply("OK\r\n");
}

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


