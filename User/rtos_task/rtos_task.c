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
SemaphoreHandle_t Mutex_USB_TX = 0;              /* USB发送互斥锁，保护USB传输不被打断（全局，供其他模块使用） */

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
/**
 * @brief  DMA完成回调钩子（由 bsp_fpga.c 的 ISR 调用）
 * @param  buffer_index 0=Buffer A, 1=Buffer B
 */
void BSP_FPGA_OnBlockReady(uint8_t buffer_index)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    /* 更新活动缓冲区索引 */
    g_ActiveBufferIdx = buffer_index;
    
    /* 释放信号量通知 DSP 任务 */
    if (Sem_DMA_Complete)
    {
        xSemaphoreGiveFromISR(Sem_DMA_Complete, &xHigherPriorityTaskWoken);
    }
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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
    
    /* 创建USB发送互斥锁 */
    Mutex_USB_TX = xSemaphoreCreateMutex();

    /* 创建 Task_SCPI（高优先级）- 唯一允许检查USB接收的任务 */
    xTaskCreate((TaskFunction_t)Task_SCPI,
                (const char *)"Task_SCPI",
                (uint16_t)1024,  /* 1024 words = 4KB stack for printf/FPU */
                (void *)NULL,
                (UBaseType_t)(tskIDLE_PRIORITY + 5),  /* 最高优先级 */
                (TaskHandle_t *)&s_task_scpi);

    /* 创建 Task_DSP（中优先级）- 数据处理任务 */
    // xTaskCreate((TaskFunction_t)Task_DSP,
    //             (const char *)"Task_DSP",
    //             (uint16_t)1024,  /* 1024 words = 4KB stack for printf/FPU */
    //             (void *)NULL,
    //             (UBaseType_t)(tskIDLE_PRIORITY + 3),  /* 中优先级 */
    //             (TaskHandle_t *)&s_task_data);

    /* 创建 AppTask（低优先级）- LED心跳任务 */
    // xTaskCreate((TaskFunction_t)AppTask,
    //             (const char *)"AppTask",
    //             (uint16_t)256,  /* 256 words = 1KB stack (minimal but safe) */
    //             (void *)NULL,
    //             (UBaseType_t)(tskIDLE_PRIORITY + 1),  /* 低优先级 */
    //             (TaskHandle_t *)&s_task_app);
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
/**
 * @brief  AppTask - LED心跳任务（低优先级）
 * @param  pvParameters: 传入参数
 * @retval None
 * 
 * 功能：LED心跳，每500ms切换一次
 * 注意：不处理任何USB相关代码
 */
static void AppTask(void *pvParameters)
{
    (void)pvParameters;
    
    /* 初始化LED */
    LED_Init();
    
    while(1)
    {
        /* LED心跳：每500ms切换一次 */
        LED_G = 1;
        LED_R = 0;
        LED_B = 0;
        vTaskDelay(pdMS_TO_TICKS(500));
        
        LED_G = 0;
        LED_R = 0;
        LED_B = 0;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ---------------- USB -> Task_SCPI 通知回调（从 USB ISR/回调上下文调用） ---------------- */
/* 注意：此函数在中断上下文中调用，不能使用printf等非中断安全函数 */
void SCPI_Receive_Callback(uint16_t len)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE; 
    BaseType_t result;
    
    /* 在中断上下文中，只做必要的操作，调试信息移到任务中 */
    s_last_scpi_len = len;
    
    if (s_scpi_rx_sem)
    {
        result = xSemaphoreGiveFromISR(s_scpi_rx_sem, &xHigherPriorityTaskWoken);
        /* 如果信号量已满，说明任务还没处理完上次的数据 */
        if (result != pdTRUE)
        {

        }
    }
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* ---------------- FPGA DMA 完成通知（从 DMA ISR 调用） ---------------- */
/**
 * @brief DMA传输完成回调（由bsp_fpga.c的HAL回调调用）
 */
/**
 * @brief  DMA传输完成回调（由bsp_fpga.c的HAL回调调用）
 */
void BSP_FPGA_DMA_Complete_Callback(void)
{
    /* 实际逻辑已移至 BSP_FPGA_OnBlockReady，这里保留兼容性 */
}

/**
 * @brief  DMA错误回调
 */
void BSP_FPGA_DMA_Error_Callback(void)
{
    /* DMA错误时也释放信号量，避免数据任务永久阻塞 */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (Sem_DMA_Complete)
    {
        xSemaphoreGiveFromISR(Sem_DMA_Complete, &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief  线程安全的USB发送函数
 * @param  buf: 要发送的数据缓冲区
 * @param  len: 数据长度
 * @retval None
 * 
 * 功能：使用互斥锁保护USB发送，防止多个任务同时发送导致数据交错
 */
void USB_Send_Safe(uint8_t *buf, uint32_t len)
{
    if (buf == NULL || len == 0)
    {
        return;
    }
    
    /* 获取USB发送互斥锁 */
    if (Mutex_USB_TX != NULL)
    {
        if (xSemaphoreTake(Mutex_USB_TX, portMAX_DELAY) == pdTRUE)
        {
            /* 发送数据 */
            for (uint32_t i = 0; i < len; i++)
            {
                VCP_DataTx(buf[i]);
            }
            
            /* 释放互斥锁 */
            xSemaphoreGive(Mutex_USB_TX);
        }
    }
    else
    {
        /* 如果没有互斥锁，直接发送（不安全，但至少能工作） */
        for (uint32_t i = 0; i < len; i++)
        {
            VCP_DataTx(buf[i]);
        }
    }
}

/**
 * @brief  SCPI发送函数（带互斥锁保护）
 * @param  line: 要发送的字符串
 * @retval None
 * 
 * 功能：通过USB发送数据，使用互斥锁保护，防止与DSP任务的数据流冲突
 * 注意：此函数由SCPI回调调用，使用USB_Send_Safe确保线程安全
 */
static void scpi_send_line(const char *line)
{
    if (line == NULL)
    {
        return;
    }
    
    /* 使用线程安全的发送函数 */
    USB_Send_Safe((uint8_t *)line, strlen(line));
}

/**
 * @brief  Task_SCPI - SCPI命令接收与解析任务（高优先级）
 * @param  pvParameters: 传入参数
 * @retval None
 * 
 * 功能：
 * - 唯一允许检查 USB_USART_RX_STA 的任务
 * - 轮询USB接收，解析SCPI命令并执行
 * - 高优先级，确保能及时停止数据流
 */
static void Task_SCPI(void *pvParameters)
{   
    u16 t;
	u16 len;
    char idn[] = ":32ID?\r\n";
    while(1)
   {
       if(USB_USART_RX_STA&0x8000)
		{					   
			len=USB_USART_RX_STA&0x3FFF;// 得到此次接收到的数据长度
			// usb_printf("您发送的消息为:\r\n");
            if(strcmp(USB_USART_RX_BUF, idn) == 0)
            {
                // SCPI_Parse(USB_USART_RX_BUF);
                USB_Send_Safe((uint8_t *)idn, strlen(idn));
            }
			// for(t=0;t<len;t++)
			// {
			// 	VCP_DataTx(USB_USART_RX_BUF[t]);// 以字节方式,发送给USB 
                
			// }
			usb_printf("\r\n");
			USB_USART_RX_STA=0;
		}
   }
    // char rx_line[256];
    // uint16_t len;

    // (void)pvParameters;

    // /* 注入输出与数据任务句柄 */
    // SCPI_Callbacks_Init(scpi_send_line);
    // SCPI_SetDataTaskHandle((void *)s_task_data);

    // while (1)
    // {
    //     /* 等待USB接收信号量（由USB ISR触发）或轮询检查 */
    //     if (s_scpi_rx_sem != NULL)
    //     {
    //         /* 使用信号量等待（更高效） */
    //         if (xSemaphoreTake(s_scpi_rx_sem, pdMS_TO_TICKS(100)) == pdTRUE)
    //         {
    //             /* 信号量触发，检查USB接收状态 */
    //             uint16_t poll_sta = USB_USART_RX_STA;
    //             if ((poll_sta & 0x8000) != 0)
    //             {
    //                 len = (uint16_t)(poll_sta & 0x3FFF);
    //                 if (len > 0 && len < sizeof(rx_line))
    //                 {
    //                     memcpy(rx_line, USB_USART_RX_BUF, len);
    //                     rx_line[len] = '\0';
    //                     USB_USART_RX_STA = 0;
                        
    //                     /* 调用SCPI解析器（回调会使用USB_Send_Safe，自动处理互斥锁） */
    //                     SCPI_Parse(rx_line);
    //                 }
    //                 else
    //                 {
    //                     USB_USART_RX_STA = 0;
    //                 }
    //             }
    //         }
    //         else
    //         {
    //             /* 信号量超时，轮询检查（后备机制） */
    //             uint16_t poll_sta = USB_USART_RX_STA;
    //             if ((poll_sta & 0x8000) != 0)
    //             {
    //                 len = (uint16_t)(poll_sta & 0x3FFF);
    //                 if (len > 0 && len < sizeof(rx_line))
    //                 {
    //                     memcpy(rx_line, USB_USART_RX_BUF, len);
    //                     rx_line[len] = '\0';
    //                     USB_USART_RX_STA = 0;
                        
    //                     SCPI_Parse(rx_line);
    //                 }
    //                 else
    //                 {
    //                     USB_USART_RX_STA = 0;
    //                 }
    //             }
    //         }
    //     }
    //     else
    //     {
    //         /* 没有信号量，使用轮询模式 */
    //         uint16_t poll_sta = USB_USART_RX_STA;
    //         if ((poll_sta & 0x8000) != 0)
    //         {
    //             len = (uint16_t)(poll_sta & 0x3FFF);
    //             if (len > 0 && len < sizeof(rx_line))
    //             {
    //                 memcpy(rx_line, USB_USART_RX_BUF, len);
    //                 rx_line[len] = '\0';
    //                 USB_USART_RX_STA = 0;
                    
    //                 SCPI_Parse(rx_line);
    //             }
    //             else
    //             {
    //                 USB_USART_RX_STA = 0;
    //             }
    //         }
    //         vTaskDelay(pdMS_TO_TICKS(10));
    //     }
    // }
}

/**
 * @brief  Task_DSP - 数据处理任务（中优先级）
 * @param  pvParameters: 传入参数
 * @retval None
 * 
 * 功能：
 * - 检查全局标志 g_AcquisitionRunning（由SCPI设置）
 * - 如果运行：等待DMA信号量，处理数据，通过USB发送（使用互斥锁保护）
 * - 如果停止：延时等待
 */
static void Task_DSP(void *pvParameters)
{
    uint16_t *pBuffer = NULL;
    char tx_buf[64];
    
    (void)pvParameters;
    
    /* 初始化 FPGA/FMC（也会初始化 DMA） */
    BSP_FPGA_Init();
    
    /* 初始化 DSP 引擎 */
    DSP_Init();
    
    /* 初始化测试数据生成器 */
    DSP_Tester_Init();
    
    while (1)
    {
        /* 检查采集运行标志（由SCPI命令控制） */
        if (g_AcquisitionRunning)
        {
            /* --- MODE A: Standalone Simulation (No FPGA needed) --- */
            if (g_UseSimSource == 1)
            {
                /* 1. 获取模拟数据（正弦波） */
                pBuffer = DSP_Tester_Get_Sim_Buffer();
                
                /* 2. 模拟采样速率延迟（50kHz，1024 点约 20ms） */
                vTaskDelay(pdMS_TO_TICKS(20));
            }
            /* --- MODE B: Full Hardware Chain (Waits for FPGA DMA) --- */
            else
            {
                /* 1. 等待 DMA 完成信号量（带超时，允许在 FPGA 故障时退出） */
                if (Sem_DMA_Complete)
                {
                    if (xSemaphoreTake(Sem_DMA_Complete, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        /* 2. 选择 DMA 刚完成的缓冲区（Ping-Pong） */
                        if (g_ActiveBufferIdx == 0)
                        {
                            pBuffer = FPGA_DMA_Buffer_A;
                        }
                        else
                        {
                            pBuffer = FPGA_DMA_Buffer_B;
                        }
                    }
                    else
                    {
                        /* DMA 超时，跳过本次循环 */
                        pBuffer = NULL;
                    }
                }
                else
                {
                    pBuffer = NULL;
                }
            }
            
            /* --- Common Processing & Sending --- */
            if (pBuffer != NULL)
            {
                /* 调用 DSP 引擎（FIR -> R/Theta -> IIR） */
                DSP_Process_Block(pBuffer, LIA_BLOCK_SIZE, &Global_Result);
                
                /* 根据数据模式发送到 PC（使用线程安全的发送函数） */
                if (g_DataOutputMode == 0)
                {
                    /* 结果模式：发送结果字符串 "R=...,P=..." */
                    snprintf(tx_buf, sizeof(tx_buf), "R=%.4f,P=%.4f\r\n", 
                             (double)Global_Result.R, (double)Global_Result.Theta);
                    /* 使用线程安全的发送函数（自动处理互斥锁） */
                    USB_Send_Safe((uint8_t *)tx_buf, strlen(tx_buf));
                }
                else
                {
                    /* 原始模式：发送原始 I/Q 数据（CSV 格式） */
                    uint32_t i;
                    for (i = 0; i < LIA_BLOCK_SIZE; i++)
                    {
                        uint32_t base_idx = i * 4u;
                        int32_t I_raw = (int32_t)((uint32_t)pBuffer[base_idx + 1] << 16) | (uint32_t)pBuffer[base_idx + 0];
                        int32_t Q_raw = (int32_t)((uint32_t)pBuffer[base_idx + 3] << 16) | (uint32_t)pBuffer[base_idx + 2];
                        snprintf(tx_buf, sizeof(tx_buf), "%ld,%ld\r\n", (long)I_raw, (long)Q_raw);
                        /* 使用线程安全的发送函数（自动处理互斥锁） */
                        USB_Send_Safe((uint8_t *)tx_buf, strlen(tx_buf));
                    }
                }
            }
        }
        else
        {
            /* 空闲状态：等待采集启动 */
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
