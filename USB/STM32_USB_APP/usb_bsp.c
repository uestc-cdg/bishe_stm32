//-----------------------------------------------------------------
// ��������:
//     USB-BSP(HS)����
// ��    ��: ���ǵ���
// ��ʼ����: 2018-08-04
// �������: 2018-08-04
// �޸�����: 
// ��ǰ�汾: V1.0
// ��ʷ�汾:
//  - V1.0: (2018-08-04)	USB-BSP(FS)��ʼ��
// ���Թ���: ����STM32F429+CycloneIV����ϵͳ��ƿ����塢LZE_ST_LINK2
// ˵    ��: 
//    
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// ͷ�ļ�����
//-----------------------------------------------------------------
#include "stm32f429_winner.h"
#include "usb_bsp.h"
#include "delay.h" 
#include "usart.h" 
//-----------------------------------------------------------------  

//-----------------------------------------------------------------
// void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE *pdev)
//-----------------------------------------------------------------
// 
// ��������: USB OTG HS�ײ�IO��ʼ��
// ��ڲ���: USB_OTG_CORE_HANDLE *pdev��USB OTG�ں˽ṹ��ָ��
// �� �� ֵ: ��
// ע������: ��
//
//-----------------------------------------------------------------
void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE *pdev)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	__HAL_RCC_GPIOA_CLK_ENABLE();                   // ʹ��GPIOAʱ��
	__HAL_RCC_GPIOB_CLK_ENABLE();                   // ʹ��GPIOBʱ��
	__HAL_RCC_GPIOC_CLK_ENABLE();                   // ʹ��GPIOCʱ��
	__HAL_RCC_GPIOI_CLK_ENABLE();                   // ʹ��GPIOIʱ��

	// ����PA3->USB_OTG_HS_ULPI_D0	|	PA5->USB_OTG_HS_ULPI_CLK
	GPIO_InitStruct.Pin=GPIO_PIN_3 | GPIO_PIN_5;    // PA3	|| PA5
	GPIO_InitStruct.Mode=GPIO_MODE_AF_PP;           // ����
	GPIO_InitStruct.Pull=GPIO_NOPULL;               // ��������
	GPIO_InitStruct.Speed=GPIO_SPEED_FREQ_VERY_HIGH;// ����
	GPIO_InitStruct.Alternate=GPIO_AF10_OTG_HS;     // ����ΪOTG HS
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);         // ��ʼ��

	// ����PB0->USB_OTG_HS_ULPI_D1  | PB1->USB_OTG_HS_ULPI_D2 | PB5->USB_OTG_HS_ULPI_D7 
	//		 PB10->USB_OTG_HS_ULPI_D3 | PB11->USB_OTG_HS_ULPI_D4
	//		 PB12->USB_OTG_HS_ULPI_D5 | PB13->USB_OTG_HS_ULPI_D6
	GPIO_InitStruct.Pin=GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_5 | 
											GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
	GPIO_InitStruct.Mode=GPIO_MODE_AF_PP;           // ����
	GPIO_InitStruct.Pull=GPIO_NOPULL;               // ��������
	GPIO_InitStruct.Speed=GPIO_SPEED_FREQ_VERY_HIGH;// ����
	GPIO_InitStruct.Alternate=GPIO_AF10_OTG_HS;     // ����ΪOTG HS
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);         // ��ʼ��

	// ����PC0->USB_OTG_HS_ULPI_STP | PC3->USB_OTG_HS_ULPI_NXT
	GPIO_InitStruct.Pin=GPIO_PIN_0 | GPIO_PIN_3;    // PC0	||	PC3
	GPIO_InitStruct.Mode=GPIO_MODE_AF_PP;           // ����
	GPIO_InitStruct.Pull=GPIO_NOPULL;               // ��������
	GPIO_InitStruct.Speed=GPIO_SPEED_FREQ_VERY_HIGH;// ����
	GPIO_InitStruct.Alternate=GPIO_AF10_OTG_HS;     // ����ΪOTG HS
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);         // ��ʼ��

	// ����PI11->USB_OTG_HS_ULPI_DIR
	GPIO_InitStruct.Pin=GPIO_PIN_11;    						// PI11
	GPIO_InitStruct.Mode=GPIO_MODE_AF_PP;           // ����
	GPIO_InitStruct.Pull=GPIO_NOPULL;               // ��������
	GPIO_InitStruct.Speed=GPIO_SPEED_FREQ_VERY_HIGH;// ����
	GPIO_InitStruct.Alternate=GPIO_AF10_OTG_HS;     // ����ΪOTG HS
	HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);         // ��ʼ��
	
	__HAL_RCC_USB_OTG_HS_CLK_ENABLE();              // ʹ��OTG HSʱ��
	__HAL_RCC_USB_OTG_HS_ULPI_CLK_ENABLE();         // ʹ��OTG HS ULPIʱ��
}

//-----------------------------------------------------------------
// void USB_OTG_BSP_EnableInterrupt(USB_OTG_CORE_HANDLE *pdev)
//-----------------------------------------------------------------
// 
// ��������: USB OTG HS�ж�����,����USB HS�ж�
// ��ڲ���: USB_OTG_CORE_HANDLE *pdev��USB OTG�ں˽ṹ��ָ��
// �� �� ֵ: ��
// ע������: ��
//
//-----------------------------------------------------------------
void USB_OTG_BSP_EnableInterrupt(USB_OTG_CORE_HANDLE *pdev)
{
	HAL_NVIC_SetPriority(OTG_HS_IRQn,0,0);    			// ��ռ���ȼ�1�������ȼ�3
  HAL_NVIC_EnableIRQ(OTG_HS_IRQn);        				// ʹ��OTG USB HS�ж�  
	
	HAL_NVIC_SetPriority(OTG_HS_EP1_OUT_IRQn,1,2);  // ��ռ���ȼ�1�������ȼ�2
  HAL_NVIC_EnableIRQ(OTG_HS_EP1_OUT_IRQn);        // ʹ��OTG_HS_EP1_OUT_IRQn�ж�  
	
	HAL_NVIC_SetPriority(OTG_HS_EP1_IN_IRQn,1,1);   // ��ռ���ȼ�1�������ȼ�1
  HAL_NVIC_EnableIRQ(OTG_HS_EP1_IN_IRQn);        	// ʹ��OTG_HS_EP1_IN_IRQn�ж�  
}

//-----------------------------------------------------------------
// void USB_OTG_BSP_DisableInterrupt(void)
//-----------------------------------------------------------------
// 
// ��������: USB OTG FS�ж�����,�ر�USB FS�ж�
// ��ڲ���: ��
// �� �� ֵ: ��
// ע������: ��
//
//-----------------------------------------------------------------
void USB_OTG_BSP_DisableInterrupt(void)
{ 
	
}

//-----------------------------------------------------------------
// void USB_OTG_BSP_DriveVBUS(USB_OTG_CORE_HANDLE *pdev, uint8_t state)
//-----------------------------------------------------------------
// 
// ��������: USB OTG �˿ڹ�������(������δ�õ�)
// ��ڲ���: USB_OTG_CORE_HANDLE *pdev��USB OTG�ں˽ṹ��ָ��
//					 uint8_t state��0,�ϵ�;1,�ϵ�
// �� �� ֵ: ��
// ע������: ��
//
//-----------------------------------------------------------------
void USB_OTG_BSP_DriveVBUS(USB_OTG_CORE_HANDLE *pdev, uint8_t state)
{ 
	
}

//-----------------------------------------------------------------
// void  USB_OTG_BSP_ConfigVBUS(USB_OTG_CORE_HANDLE *pdev)
//-----------------------------------------------------------------
// 
// ��������: USB_OTG �˿ڹ���IO����(������δ�õ�)
// ��ڲ���: USB_OTG_CORE_HANDLE *pdev��USB OTG�ں˽ṹ��ָ��
// �� �� ֵ: ��
// ע������: ��
//
//-----------------------------------------------------------------
void  USB_OTG_BSP_ConfigVBUS(USB_OTG_CORE_HANDLE *pdev)
{ 
	
} 

//-----------------------------------------------------------------
// void USB_OTG_BSP_uDelay (const uint32_t usec)
//-----------------------------------------------------------------
// 
// ��������: USB_OTG us����ʱ����
// ��ڲ���: const uint32_t usec��Ҫ��ʱ��us��
// �� �� ֵ: ��
// ע������: ��
//
//-----------------------------------------------------------------
void USB_OTG_BSP_uDelay (const uint32_t usec)
{ 
   	delay_us(usec);
}

//-----------------------------------------------------------------
// void USB_OTG_BSP_mDelay (const uint32_t msec)
//-----------------------------------------------------------------
// 
// ��������: USB_OTG ms����ʱ����
// ��ڲ���: const uint32_t msec��Ҫ��ʱ��ms��
// �� �� ֵ: ��
// ע������: ��
//
//-----------------------------------------------------------------
void USB_OTG_BSP_mDelay (const uint32_t msec)
{  
	delay_ms(msec);
}

//-----------------------------------------------------------------
// End Of File
//----------------------------------------------------------------- 

