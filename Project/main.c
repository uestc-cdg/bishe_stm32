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


#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
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
	
	// 创建任务
	RTOS_Task_Init();
	
	// 启动调度器
	vTaskStartScheduler();
//	u16 t;
//	u16 len;

//    while(1)
//    {
//        if(USB_USART_RX_STA&0x8000)
//		{					   
//			len=USB_USART_RX_STA&0x3FFF;// 得到此次接收到的数据长度
//			usb_printf("您发送的消息为:\r\n");
//			for(t=0;t<len;t++)
//			{
//				VCP_DataTx(USB_USART_RX_BUF[t]);// 以字节方式,发送给USB 
//                printf("%c", USB_USART_RX_BUF[t]);
//			}
//			usb_printf("\r\n");
//			USB_USART_RX_STA=0;
//		}
//    }
}

//-----------------------------------------------------------------
// End Of File
//----------------------------------------------------------------- 
