/**
  ******************************************************************************
  * @file    usbd_cdc_vcp.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   Generic media access Layer.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED 
#pragma     data_alignment = 4 
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */

/* Includes ------------------------------------------------------------------*/
#include "stm32f429_winner.h"
#include "usbd_cdc_vcp.h"
#include "usb_conf.h"
#include "string.h"	
#include "stdarg.h"		 
#include "stdio.h"	
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

u8  USART_PRINTF_Buffer[USB_USART_REC_LEN];	// usb_printf���ͻ�����

// �����ƴ���1�������ݵķ���,������USB���⴮�ڽ��յ�������.
u8 USB_USART_RX_BUF[USB_USART_REC_LEN]; 	// ���ջ���,���USART_REC_LEN���ֽ�.
// ����״̬
// bit15��	������ɱ�־
// bit14��	���յ�0x0d
// bit13~0��	���յ�����Ч�ֽ���Ŀ
u16 USB_USART_RX_STA=0;       				// ����״̬���	


// ��Щ�Ǵ�CDC���ĵ�����ⲿ����������IN�������
extern uint8_t  APP_Rx_Buffer []; // ���⴮�ڷ��ͻ��������������ԣ�
extern uint32_t APP_Rx_ptr_in;    // ���⴮�ڽ��ջ��������������Ե��Ե����ݣ�

// ���⴮�����ú��������ں˵��ã�
CDC_IF_Prop_TypeDef VCP_fops = 
{
  VCP_Init,
  VCP_DeInit,
  VCP_Ctrl,
  VCP_DataTx,
  VCP_DataRx
};

/* 弱符号：当收到一行完整命令(CRLF)后触发。任务层可重写以通知 Task_SCPI */
__weak void SCPI_Receive_Callback(uint16_t len)
{
    (void)len;
}

//-----------------------------------------------------------------
// uint16_t VCP_Init(void)
//-----------------------------------------------------------------
//
// ��������: ��ʼ��VCP
// ��ڲ���: ��
// ���ز���: USBD_OK
// ע������: ��
//
//-----------------------------------------------------------------
uint16_t VCP_Init(void)
{ 
  return USBD_OK;
}


//-----------------------------------------------------------------
// uint16_t VCP_DeInit(void)
//-----------------------------------------------------------------
//
// ��������: ��λVCP
// ��ڲ���: ��
// ���ز���: USBD_OK
// ע������: ��
//
//-----------------------------------------------------------------
uint16_t VCP_DeInit(void)
{
  return USBD_OK;
}

//-----------------------------------------------------------------
// uint16_t VCP_Ctrl (uint32_t Cmd, uint8_t* Buf, uint32_t Len)
//-----------------------------------------------------------------
//
// ��������: ����VCP������
// ��ڲ���: uint32_t Cmd������
//					 uint8_t* Buf���������ݻ�����/�������滺����
//					 uint32_t Len�����ݳ���
// ���ز���: USBD_OK
// ע������: ��
//
//-----------------------------------------------------------------
uint16_t VCP_Ctrl (uint32_t Cmd, uint8_t* Buf, uint32_t Len)
{ 
  return USBD_OK;
}

//-----------------------------------------------------------------
// uint16_t VCP_DataTx (uint8_t data)
//-----------------------------------------------------------------
//
// ��������: ����һ���ֽڸ����⴮�ڣ��������ԣ�
// ��ڲ���: uint8_t data��Ҫ���͵�����
// ���ز���: USBD_OK
// ע������: ��
//
//-----------------------------------------------------------------
uint16_t VCP_DataTx (uint8_t data)
{
	APP_Rx_Buffer[APP_Rx_ptr_in]=data;	// д�뷢��buf
	APP_Rx_ptr_in++;  									// дλ�ü�1
	if(APP_Rx_ptr_in==APP_RX_DATA_SIZE)	// ����buf��С��,����.
	{
		APP_Rx_ptr_in = 0;
	}   
	return USBD_OK;
}

//-----------------------------------------------------------------
// uint16_t VCP_DataRx (uint8_t* Buf, uint32_t Len)
//-----------------------------------------------------------------
//
// ��������: ������USB���⴮�ڽ��յ�������
// ��ڲ���: uint8_t* Buf�����ݻ�����
//					 uint32_t Len�����յ����ֽ���
// ���ز���: USBD_OK
// ע������: ��
//
//-----------------------------------------------------------------
uint16_t VCP_DataRx (uint8_t* Buf, uint32_t Len)
{
 	u8 i;
	u8 res;
	for(i=0;i<Len;i++)
	{  
		res=Buf[i]; 
		if((USB_USART_RX_STA&0x8000)==0)	// ����δ���
		{
			if(USB_USART_RX_STA&0x4000)			// ���յ���0x0d
			{
				if(res!=0x0a)									// ���մ���,���¿�ʼ
					USB_USART_RX_STA=0;
				else 													// ���������
				{
					USB_USART_RX_STA|=0x8000;
					/* 通知 SCPI 任务：收到一行完整数据（长度=低14位） */
					SCPI_Receive_Callback(USB_USART_RX_STA & 0x3FFF);
				}	 
			}
			else 														// ��û�յ�0X0D
			{	
				if(res==0x0d)
					USB_USART_RX_STA|=0x4000;
				else
				{
					USB_USART_RX_BUF[USB_USART_RX_STA&0X3FFF]=res;
					USB_USART_RX_STA++;
					if(USB_USART_RX_STA>(USB_USART_REC_LEN-1))
						USB_USART_RX_STA=0;				// �������ݴ���,���¿�ʼ����	
				}					
			}
		}   
	}  
	return USBD_OK;
}

//-----------------------------------------------------------------
// void usb_printf(char* fmt,...)  
//-----------------------------------------------------------------
//
// ��������: USB���⴮�ڣ�printf����
// ��ڲ���: Ҫ���������
// ���ز���: USBD_OK
// ע������: ȷ��һ�η������ݲ��ᳬ����󳤶�
//
//-----------------------------------------------------------------
void usb_printf(char* fmt,...)  
{  
	u16 i,j;
	va_list ap;
	va_start(ap,fmt);
	vsprintf((char*)USART_PRINTF_Buffer,fmt,ap);
	va_end(ap);
	i=strlen((const char*)USART_PRINTF_Buffer);// �˴η������ݵĳ���
	for(j=0;j<i;j++)// ѭ����������
	{
		VCP_DataTx(USART_PRINTF_Buffer[j]); 
	}
} 

//-----------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------
