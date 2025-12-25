//-----------------------------------------------------------------
// 程序描述:
//     USB_VCP_HS(虚拟串口)实验
// 作    者: 凌智电子
// 开始日期: 2018-08-04
// 完成日期: 2018-08-04
// 修改日期: 
// 当前版本: V1.0
// 历史版本:
//  - V1.0: (2018-08-04)	 USB_VCP_HS(虚拟串口)
// 调试工具: 凌智STM32F429+CycloneIV电子系统设计开发板、LZE_ST_LINK2
// 说    明: 
//    
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// 头文件包含
//-----------------------------------------------------------------
#include "stm32f429_winner.h"
#include "delay.h" 
#include "led.h"  
#include "usart.h"   
#include "sdram.h"    
#include "w25qxx.h"    

#include "usbd_cdc_vcp.h"
#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usb_conf.h"
#include "usbd_desc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "rtos_task.h"

#include "dsp_core.h"
#include "dsp_tester.h"
#include "bsp_fpga.h"
#include "scpi_core.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* 外部全局变量声明 */
extern volatile uint8_t g_AcquisitionRunning;
extern volatile uint8_t g_UseSimSource;
extern volatile uint8_t g_DataOutputMode;
//-----------------------------------------------------------------

__ALIGN_BEGIN USB_OTG_CORE_HANDLE    USB_OTG_dev __ALIGN_END ;

//-----------------------------------------------------------------

//-----------------------------------------------------------------
// 主程序
//-----------------------------------------------------------------
int main(void)
{
	HAL_Init();          				 		// 初始化HAL库
	SystemClock_Config(360,25,2,8); // 设置时钟,192Mhz   
	uart_init(115200);              // 初始化USART
	LED_Init();											// 初始化与LED连接的硬件接口
	SDRAM_Init();										// 初始化SDRAM 
//	W25QXX_Init();									// 初始化W25Q128      
	// 初始化USB主机
	USBD_Init(&USB_OTG_dev,USB_OTG_HS_CORE_ID,&USR_desc,&USBD_CDC_cb,&USR_cb);  
	
	// // 创建任务
	// RTOS_Task_Init();
	
	// // 启动调度器
	// vTaskStartScheduler();
	
	u16 t;
	u16 len;
	static uint8_t dsp_initialized = 0;  /* 初始化标志 */
	uint16_t *pBuffer = NULL;
	char tx_buf[64];
	LIA_Result_t result;
	static uint32_t last_process_time = 0;

   while(1)
   {
		/* 初始化DSP和测试数据源（只初始化一次） */
		if (dsp_initialized == 0)
		{
			/* 初始化FPGA/FMC（如果使用硬件源） */
			BSP_FPGA_Init();
			
			/* 初始化DSP引擎 */
			DSP_Init();
			
			/* 初始化测试数据生成器 */
			DSP_Tester_Init();
			
			dsp_initialized = 1;
		}
		
		/* 检查USB接收 */
		if(USB_USART_RX_STA & 0x8000)
		{					   
			len = USB_USART_RX_STA & 0x3FFF;  /* 得到此次接收到的数据长度 */
			if (len > 0)
			{
				/* 确保字符串以NULL结尾 */
				if (len < sizeof(USB_USART_RX_BUF))
				{
					USB_USART_RX_BUF[len] = '\0';
				}
				/* 解析SCPI命令 */
				SCPI_Parse((char *)USB_USART_RX_BUF);
			}
			USB_USART_RX_STA = 0;
		}
		
		/* 检查采集运行状态并处理数据 */
		if (g_AcquisitionRunning)
		{
			/* 检查是否到了处理时间（模拟采样速率，约20ms一次） */
			uint32_t current_time = HAL_GetTick();
			if (current_time - last_process_time >= 20)
			{
				last_process_time = current_time;
				
				/* 根据数据源选择 */
				if (g_UseSimSource == 1)
				{
					/* 使用模拟数据源 */
					pBuffer = DSP_Tester_Get_Sim_Buffer();
				}
				else
				{
					/* 使用硬件FPGA源（需要DMA，这里简化处理） */
					/* 注意：硬件模式需要RTOS任务和DMA，裸机模式下建议使用模拟源 */
					pBuffer = NULL;
				}
				
				/* 处理数据并发送 */
				if (pBuffer != NULL)
				{
					/* 调用DSP处理 */
					DSP_Process_Block(pBuffer, LIA_BLOCK_SIZE, &result);
					
					/* 根据数据模式发送 */
					if (g_DataOutputMode == 0)
					{
						/* 结果模式：发送 R=...,P=... */
						snprintf(tx_buf, sizeof(tx_buf), "R=%.4f,P=%.4f\r\n", 
								 (double)result.R, (double)result.Theta);
						USB_Send_Safe((uint8_t *)tx_buf, strlen(tx_buf));
					}
					else
					{
						/* 原始模式：发送 I/Q 数据 */
						uint32_t i;
						for (i = 0; i < LIA_BLOCK_SIZE; i++)
						{
							uint32_t base_idx = i * 4u;
							int32_t I_raw = (int32_t)((uint32_t)pBuffer[base_idx + 1] << 16) | (uint32_t)pBuffer[base_idx + 0];
							int32_t Q_raw = (int32_t)((uint32_t)pBuffer[base_idx + 3] << 16) | (uint32_t)pBuffer[base_idx + 2];
							snprintf(tx_buf, sizeof(tx_buf), "%ld,%ld\r\n", (long)I_raw, (long)Q_raw);
							USB_Send_Safe((uint8_t *)tx_buf, strlen(tx_buf));
						}
					}
				}
			}
		}
		else
		{
			/* 采集停止，重置时间戳 */
			last_process_time = 0;
		}
		
		/* 短暂延时，避免CPU占用过高 */
		HAL_Delay(1);
   }
}

//-----------------------------------------------------------------
// End Of File
//----------------------------------------------------------------- 
