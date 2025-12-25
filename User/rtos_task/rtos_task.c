#include "rtos_task.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "scpi_core.h"
#include "scpi_callbacks.h"

#include "bsp_fpga.h"
#include "usbd_cdc_vcp.h"

#include "dsp_core.h"
#include "dsp_tester.h"  /* 用于模拟数据源 */

/* 引入 main.c 中使用到的头文件（保持工程兼容） */
#include "stm32f429_winner.h"
#include "led.h"

#include <string.h>
#include <stdio.h>

/* 外部变量声明（在 usbd_cdc_vcp.c 中定义） */
extern u8 USB_USART_RX_BUF[];
extern u16 USB_USART_RX_STA;
extern u8 USART_PRINTF_Buffer[];

/* ---------------- 任务句柄 ---------------- */
static TaskHandle_t s_task_scpi = 0;
static TaskHandle_t s_task_data = 0;
static TaskHandle_t s_task_app  = 0;

/* ---------------- 同步对象 ---------------- */
static SemaphoreHandle_t s_scpi_rx_sem = 0;     /* USB 收到一行命令后释放 */
SemaphoreHandle_t Sem_DMA_Complete = 0;          /* FPGA DMA 完成后释放（全局，供 bsp_fpga.c 使用） */

static volatile uint16_t s_last_scpi_len = 0;

/* ---------------- 全局状态变量 ---------------- */
volatile uint8_t g_ActiveBufferIdx = 0;          /* 0=Buffer A, 1=Buffer B */
volatile uint8_t g_AcquisitionRunning = 0;      /* 0=Stop, 1=Run */
volatile uint8_t g_DataOutputMode = 0;          /* 0=Result R/Theta, 1=Raw I/Q */

/* ---------------- 数据缓存 ---------------- */
/* 使用全局双缓冲（在 bsp_fpga.c 中定义）：FPGA_DMA_Buffer_A 和 FPGA_DMA_Buffer_B */
/* DSP 处理结果 */
static LIA_Result_t Global_Result;

/* ---------------- 本地函数声明 ---------------- */
static void Task_SCPI(void *pvParameters);
static void Task_DSP(void *pvParameters);        /* 高优先级 DSP 处理任务 */
static void AppTask(void *pvParameters);
static void scpi_send_line(const char *line);

/* ---------------- DMA 驱动钩子函数 ---------------- */
/**
 * @brief DMA 完成回调钩子（由 bsp_fpga.c 的 ISR 调用）
 * @param buffer_index 0=Buffer A, 1=Buffer B
 * 【已屏蔽】DSP逻辑已屏蔽，避免干扰
 */
void BSP_FPGA_OnBlockReady(uint8_t buffer_index)
{
    /* 【已屏蔽】DSP逻辑已屏蔽，用于后续恢复 */
    // BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // /* 更新活动缓冲区索引 */
    // g_ActiveBufferIdx = buffer_index;
    
    // /* 释放信号量通知 DSP 任务 */
    // if (Sem_DMA_Complete)
    // {
    //     xSemaphoreGiveFromISR(Sem_DMA_Complete, &xHigherPriorityTaskWoken);
    // }
    
    // portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    
    (void)buffer_index; /* 避免未使用参数警告 */
}

/**
 * @brief  注册并创建所有任务
 * @param  None
 * @retval None
 */
void RTOS_Task_Init(void)
{
    /* 创建同步对象 */
    s_scpi_rx_sem = xSemaphoreCreateBinary();
    Sem_DMA_Complete = xSemaphoreCreateBinary();  /* 全局 DMA 完成信号量 */

    /* 创建 AppTask（心跳和USB回显） */
    /* 栈大小增加到 1024 words (4KB) 以支持 usb_printf/vsprintf 的栈使用 */
    xTaskCreate((TaskFunction_t)AppTask,
                (const char *)"AppTask",
                (uint16_t)1024,  /* 增加到 1024 words (4KB) 防止栈溢出 */
                (void *)NULL,
                (UBaseType_t)(tskIDLE_PRIORITY + 1),
                (TaskHandle_t *)&s_task_app);

    /* 创建 DSP 处理任务：高优先级，处理 DMA 数据 */
    xTaskCreate((TaskFunction_t)Task_DSP,
                (const char *)"Task_DSP",
                (uint16_t)1024,
                (void *)NULL,
                (UBaseType_t)(tskIDLE_PRIORITY + 4),  /* 高优先级 */
                (TaskHandle_t *)&s_task_data);

    // /* 创建 SCPI 任务：命令处理（已屏蔽，使用AppTask处理回显和32ID查询） */
    // xTaskCreate((TaskFunction_t)Task_SCPI,
    //             (const char *)"Task_SCPI",
    //             (uint16_t)1024,
    //             (void *)NULL,
    //             (UBaseType_t)(tskIDLE_PRIORITY + 3),
    //             (TaskHandle_t *)&s_task_scpi);
}

/**
 * @brief  主应用任务（USB回显和心跳任务）
 * @param  pvParameters: 传入参数
 * @retval None
 */
/**
 * @brief  主应用任务（USB回显和简单指令处理）
 * @param  pvParameters: 传入参数
 * @retval None
 * 
 * 支持的简单指令（用于快速测试）：
 * - "START" 或 ":ACQ:START" - 启动采集
 * - "STOP" 或 ":ACQ:STOP" - 停止采集
 * 
 * 注意：完整SCPI指令由Task_SCPI任务处理
 */
/**
 * @brief  主应用任务（USB回显和32ID查询）
 * @param  pvParameters: 传入参数
 * @retval None
 */
static void AppTask(void *pvParameters)
{
    u16 t = 0;
    u16 len = 0;
    char cmd_buf[128];
    const char *id_response = "stm32f429\r\n";
    
    (void)pvParameters;

    while(1)
    {
        /* USB回显和32ID查询逻辑 */
        if(USB_USART_RX_STA & 0x8000)
        {
            len = USB_USART_RX_STA & 0x3FFF;  /* 得到此次接收到的数据长度 */
            
            /* 复制命令到本地缓冲区 */
            if (len < sizeof(cmd_buf) && len > 0)
            {
                memcpy(cmd_buf, USB_USART_RX_BUF, len);
                cmd_buf[len] = '\0';
                
                /* 检查是否是32ID查询命令 */
                if (strstr(cmd_buf, ":32ID?") != NULL)
                {
                    /* 返回32ID */
                    for (t = 0; id_response[t] != '\0'; t++)
                    {
                        VCP_DataTx((uint8_t)id_response[t]);
                    }
                }
                else
                {
                    /* 普通回显：回显接收到的数据 */
                    for(t = 0; t < len; t++)
                    {
                        VCP_DataTx(USB_USART_RX_BUF[t]);
                    }
                    VCP_DataTx('\r');
                    VCP_DataTx('\n');
                }
            }
            
            USB_USART_RX_STA = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ---------------- USB -> Task_SCPI 通知回调（从 USB ISR/回调上下文调用） ---------------- */
/**
 * 【已屏蔽】SCPI任务已屏蔽，避免干扰
 */
void SCPI_Receive_Callback(uint16_t len)
{
    /* 【已屏蔽】SCPI任务已屏蔽，用于后续恢复 */
    // BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // s_last_scpi_len = len;
    // if (s_scpi_rx_sem)
    // {
    //     xSemaphoreGiveFromISR(s_scpi_rx_sem, &xHigherPriorityTaskWoken);
    // }
    // portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    
    (void)len; /* 避免未使用参数警告 */
}

/* ---------------- FPGA DMA 完成通知（从 DMA ISR 调用） ---------------- */
/**
 * @brief DMA传输完成回调（由bsp_fpga.c的HAL回调调用）
 */
void BSP_FPGA_DMA_Complete_Callback(void)
{
    /* 实际逻辑已移至 BSP_FPGA_OnBlockReady，这里保留兼容性 */
    /* 如果需要，可以在这里添加额外的处理逻辑 */
}

void BSP_FPGA_DMA_Error_Callback(void)
{
    /* 【已屏蔽】DSP逻辑已屏蔽，避免干扰 */
    // /* DMA错误时也释放信号量，避免数据任务永久阻塞 */
    // /* 上层可进一步打印错误信息 */
    // BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // if (Sem_DMA_Complete)
    // {
    //     xSemaphoreGiveFromISR(Sem_DMA_Complete, &xHigherPriorityTaskWoken);
    // }
    // portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void scpi_send_line(const char *line)
{
    const char *p;
    if (line == 0)
        return;
    p = line;
    while (*p)
    {
        VCP_DataTx((uint8_t)(*p));
        p++;
    }
}

static void Task_SCPI(void *pvParameters)
{
    char rx_line[256];
    uint16_t len;

    (void)pvParameters;

    /* 注入输出与数据任务句柄 */
    SCPI_Callbacks_Init(scpi_send_line);
    SCPI_SetDataTaskHandle((void *)s_task_data);

    while (1)
    {
        if (s_scpi_rx_sem)
        {
            xSemaphoreTake(s_scpi_rx_sem, portMAX_DELAY);
        }
        else
        {
            vTaskDelay(10);
        }

        if ((USB_USART_RX_STA & 0x8000) == 0)
        {
            continue;
        }

        len = (uint16_t)(USB_USART_RX_STA & 0x3FFF);
        if (len >= (sizeof(rx_line) - 1))
        {
            len = (uint16_t)(sizeof(rx_line) - 1);
        }

        memcpy(rx_line, USB_USART_RX_BUF, len);
        rx_line[len] = '\0';
        USB_USART_RX_STA = 0;

        /* 保留原有回显行为 */
        usb_printf("Echo: %s\r\n", rx_line);

        /* 纯解析层调用 -> 回调 -> 驱动 */
        SCPI_Parse(rx_line);
    }
}

/**
 * @brief 流水灯任务（用于验证RTOS基本功能）
 * 【DSP逻辑已屏蔽】恢复为流水灯任务
 */
static void Task_DSP(void *pvParameters)
{
    (void)pvParameters;
    
    /* 初始化LED */
    LED_Init();
    
    /* 流水灯循环 - 用于验证RTOS任务调度是否正常 */
    while(1)
    {
        /* 绿色LED亮 */
        LED_G = 1;
        LED_R = 0;
        LED_B = 0;
        vTaskDelay(pdMS_TO_TICKS(500));
        
        /* 红色LED亮 */
        LED_G = 0;
        LED_R = 1;
        LED_B = 0;
        vTaskDelay(pdMS_TO_TICKS(500));
        
        /* 蓝色LED亮 */
        LED_G = 0;
        LED_R = 0;
        LED_B = 1;
        vTaskDelay(pdMS_TO_TICKS(500));
        
        /* 全部熄灭 */
        LED_G = 0;
        LED_R = 0;
        LED_B = 0;
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    /* ========== 以下为原始DSP处理逻辑（已屏蔽，用于后续恢复） ========== */
    // uint16_t *pBuffer = NULL;
    // char tx_buf[64];
    
    // /* 初始化 FPGA/FMC（也会初始化 DMA） */
    // BSP_FPGA_Init();
    
    // /* 初始化 DSP 引擎 */
    // DSP_Init();
    
    // /* 初始化测试数据生成器 */
    // DSP_Tester_Init();
    
    // while (1)
    // {
    //     /* 只有在采集启动时才运行 */
    //     if (g_AcquisitionRunning)
    //     {
    //         /* --- MODE A: Standalone Simulation (No FPGA needed) --- */
    //         if (g_UseSimSource == 1)
    //         {
    //             /* 1. 获取模拟数据（正弦波） */
    //             pBuffer = DSP_Tester_Get_Sim_Buffer();
                
    //             /* 2. 模拟采样速率延迟（50kHz，1024 点约 20ms） */
    //             /* 使用 vTaskDelay 防止 CPU 过载 */
    //             vTaskDelay(pdMS_TO_TICKS(20));
    //         }
    //         /* --- MODE B: Full Hardware Chain (Waits for FPGA DMA) --- */
    //         else
    //         {
    //             /* 1. 等待 DMA 完成信号量（带超时，允许在 FPGA 故障时退出） */
    //             if (Sem_DMA_Complete)
    //             {
    //                 if (xSemaphoreTake(Sem_DMA_Complete, pdMS_TO_TICKS(100)) == pdTRUE)
    //                 {
    //                     /* 2. 选择 DMA 刚完成的缓冲区（Ping-Pong） */
    //                     if (g_ActiveBufferIdx == 0)
    //                     {
    //                         pBuffer = FPGA_DMA_Buffer_A;
    //                     }
    //                     else
    //                     {
    //                         pBuffer = FPGA_DMA_Buffer_B;
    //                     }
    //                 }
    //                 else
    //                 {
    //                     /* DMA 超时，跳过本次循环 */
    //                     pBuffer = NULL;
    //                 }
    //             }
    //             else
    //             {
    //                 pBuffer = NULL;
    //             }
    //         }
            
    //         /* --- Common Processing & Sending --- */
    //         if (pBuffer != NULL)
    //         {
    //             /* 调用 DSP 引擎（FIR -> R/Theta -> IIR） */
    //             DSP_Process_Block(pBuffer, LIA_BLOCK_SIZE, &Global_Result);
                
    //             /* 根据数据模式发送到 PC */
    //             if (g_DataOutputMode == 0)
    //             {
    //                 /* 结果模式：发送结果字符串 "R=...,P=..." */
    //                 snprintf(tx_buf, sizeof(tx_buf), "R=%.4f,P=%.4f\r\n", 
    //                          (double)Global_Result.R, (double)Global_Result.Theta);
    //                 scpi_send_line(tx_buf);
    //             }
    //             else
    //             {
    //                 /* 原始模式：发送原始 I/Q 数据（CSV 格式） */
    //                 uint32_t i;
    //                 for (i = 0; i < LIA_BLOCK_SIZE; i++)
    //                 {
    //                     uint32_t base_idx = i * 4u;
    //                     int32_t I_raw = (int32_t)((uint32_t)pBuffer[base_idx + 1] << 16) | (uint32_t)pBuffer[base_idx + 0];
    //                     int32_t Q_raw = (int32_t)((uint32_t)pBuffer[base_idx + 3] << 16) | (uint32_t)pBuffer[base_idx + 2];
    //                     snprintf(tx_buf, sizeof(tx_buf), "%ld,%ld\r\n", (long)I_raw, (long)Q_raw);
    //                     scpi_send_line(tx_buf);
    //                 }
    //             }
    //         }
    //     }
    //     else
    //     {
    //         /* 空闲状态：等待采集启动 */
    //         vTaskDelay(pdMS_TO_TICKS(100));
    //     }
    // }
}
