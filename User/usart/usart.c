//-----------------------------------------------------------------
// ��������:
// 		 ����ͨѶ��������
// ��    ��: ���ǵ���
// ��ʼ����: 2018-08-04
// �������: 2018-08-04
// �޸�����: 
// ��ǰ�汾: V1.0
// ��ʷ�汾:
//  - V1.0: (2018-08-04)���ڳ�ʼ���������ж�
// ���Թ���: ����STM32F429+Cyclone IV����ϵͳ��ƿ����塢LZE_ST_LINK2
// ˵    ��: 
//    
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// ͷ�ļ�����
//-----------------------------------------------------------------
#include "usart.h"
#include "delay.h"
#include "string.h"
//-----------------------------------------------------------------

u8 USART_RX_BUF[USART_REC_LEN]; 	// ���ջ���,���USART_REC_LEN���ֽ�.
u16 USART_RX_STA=0;       				// ����״̬��ǣ�bit15��������ɱ�־  bit14�����յ�0x0d   bit13~0�����յ�����Ч�ֽ���Ŀ��
u8 aRxBuffer[RXBUFFERSIZE];				// HAL��ʹ�õĴ��ڽ��ջ���
UART_HandleTypeDef UART_Handler;  // UART���

//-----------------------------------------------------------------
// void uart_init(u32 bound)
//-----------------------------------------------------------------
//
// ��������: ����1��ʼ��
// ��ڲ���: u32 bound��������
// ���ز���: ��
// ע������: ��
//
//-----------------------------------------------------------------
void uart_init(u32 bound)
{	
	// UART ��ʼ������
	UART_Handler.Instance=USARTx;					    			// USART1
	UART_Handler.Init.BaudRate=bound;				    		// ������
	UART_Handler.Init.WordLength=UART_WORDLENGTH_8B;// �ֳ�Ϊ8λ���ݸ�ʽ
	UART_Handler.Init.StopBits=UART_STOPBITS_1;	    // һ��ֹͣλ
	UART_Handler.Init.Parity=UART_PARITY_NONE;		  // ����żУ��λ
	UART_Handler.Init.HwFlowCtl=UART_HWCONTROL_NONE;// ��Ӳ������
	UART_Handler.Init.Mode=UART_MODE_TX_RX;		    	//�շ�ģʽ
	HAL_UART_Init(&UART_Handler);					    			//HAL_UART_Init()��ʹ��UART1
	
	// �ú����Ὺ�������жϣ���־λUART_IT_RXNE���������ý��ջ����Լ����ջ���������������
	HAL_UART_Receive_IT(&UART_Handler, (u8 *)aRxBuffer, RXBUFFERSIZE);
  
}

//-----------------------------------------------------------------
// void HAL_UART_MspInit(UART_HandleTypeDef *huart)
//-----------------------------------------------------------------
//
// ��������: UART�ײ��ʼ����ʱ��ʹ�ܣ��������ã��ж�����
// ��ڲ���: huart:���ھ��
// ���ز���: ��
// ע������: �˺����ᱻHAL_UART_Init()����
//
//-----------------------------------------------------------------
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
	GPIO_InitTypeDef GPIO_Initure;
	
	if(huart->Instance==USARTx)// ����Ǵ���1�����д���1 MSP��ʼ��
	{
		USART_RX_GPIO_CLK_ENABLE();
		USART_TX_GPIO_CLK_ENABLE();
		USART_CLK_ENABLE();				 						 // ʹ��USART1ʱ��
	
		GPIO_Initure.Pin=USART_TX_PIN;				 // PA9 -> USART1_TX
		GPIO_Initure.Mode=GPIO_MODE_AF_PP;		 // �����������
		GPIO_Initure.Pull=GPIO_PULLUP;				 // ����
		GPIO_Initure.Speed=GPIO_SPEED_FAST;		 // ����
		GPIO_Initure.Alternate=USART_TX_AF;		 // ����ΪUSART1
		HAL_GPIO_Init(USART_TX_GPIO_PORT,&GPIO_Initure);	   // ��ʼ��PA9

		GPIO_Initure.Pin=USART_RX_PIN;				 // PA10 -> USART1_RX
		HAL_GPIO_Init(USART_RX_GPIO_PORT,&GPIO_Initure);	   // ��ʼ��PA10
		
		HAL_NVIC_EnableIRQ(USART_IRQn);			   // ʹ��USART1�ж�ͨ��
		HAL_NVIC_SetPriority(USART_IRQn,3,3);  // ��ռ���ȼ�3�������ȼ�3
	}

}

//-----------------------------------------------------------------
// void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//-----------------------------------------------------------------
//
// ��������: �����жϻص�����
// ��ڲ���: huart:���ھ��
// ���ز���: ��
// ע������: ��
//
//-----------------------------------------------------------------
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance==USARTx)		// ����Ǵ���1
	{
		if((USART_RX_STA&0x8000)==0)// ����δ���
		{
			if(USART_RX_STA&0x4000)		// ���յ���0x0d
			{
				if(aRxBuffer[0]!=0x0a)	// ���մ���,���¿�ʼ
					USART_RX_STA=0;
				else 										// ��������� 
					USART_RX_STA|=0x8000;	
			}
			else // ��û�յ�0X0D
			{	
				if(aRxBuffer[0]==0x0d)
					USART_RX_STA|=0x4000;
				else
				{
					USART_RX_BUF[USART_RX_STA&0X3FFF]=aRxBuffer[0] ;
					USART_RX_STA++;
					if(USART_RX_STA>(USART_REC_LEN-1))
						USART_RX_STA=0;// �������ݴ���,���¿�ʼ����	  
				}		 
			}
		}

	}
}

//-----------------------------------------------------------------
// void USART1_IRQHandler(void) 
//-----------------------------------------------------------------
//
// ��������: ����1�жϷ������
// ��ڲ���: ��
// ���ز���: ��
// ע������: ��
//-----------------------------------------------------------------
void USART1_IRQHandler(void)                	
{ 
	u32 timeout=0;
	u32 maxDelay=0x1FFFF;
	
	HAL_UART_IRQHandler(&UART_Handler);	// ����HAL���жϴ������ú���
	
	timeout=0;
  while (HAL_UART_GetState(&UART_Handler) != HAL_UART_STATE_READY)// �ȴ�����
	{
		timeout++;// ��ʱ����
    if(timeout>maxDelay) 
			break;		
	}
     
	timeout=0;
	// һ�δ������֮�����¿����жϲ�����RxXferCountΪ1
	while(HAL_UART_Receive_IT(&UART_Handler, (u8 *)aRxBuffer, RXBUFFERSIZE) != HAL_OK)
	{
		timeout++; // ��ʱ����
		if(timeout>maxDelay) 
			break;	
	}
} 

//-----------------------------------------------------------------
// �������´���,֧��printf���� 
//-----------------------------------------------------------------
#if 1
#pragma import(__use_no_semihosting)             
// ��׼����Ҫ��֧�ֺ���                 
struct __FILE 
{ 
	int handle; 
}; 
FILE __stdout;  

// ����_sys_exit()�Ա���ʹ�ð�����ģʽ    
void _sys_exit(int x) 
{ 
	x = x; 
} 

// �ض���_ttywrch������ȥ��semihosting�����
// ��ֹ�ڽ��� #pragma import(__use_no_semihosting) ʱ���ӿ��ڲ����� _ttywrch �ᱨ��
int _ttywrch(int ch)
{
	while((USART1->SR&0X40)==0);// ѭ������,ֱ���������
	USART1->DR = (u8)ch;
	return ch;
}

// �ض���fputc���� 
int fputc(int ch, FILE *f)
{ 	
	while((USART1->SR&0X40)==0);// ѭ������,ֱ���������   
	USART1->DR = (u8) ch;      
	return ch;
}
#endif 

//-----------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------
