//-----------------------------------------------------------------
// 程序描述:
//     SPI驱动程序头文件
// 作    者: 凌智电子
// 开始日期: 2018-08-04
// 完成日期: 2018-08-04
// 修改日期: 
// 当前版本: V1.0
// 历史版本:
//  - V1.0: (2018-08-04)LED IO 配置
// 调试工具: 凌智STM32F429+CycloneIV电子系统设计开发板、LZE_ST_LINK2
// 说    明: 
//    
//-----------------------------------------------------------------
#ifndef __SPI_H
#define __SPI_H
#include "stm32f429_winner.h"
//-----------------------------------------------------------------
// 声明
//-----------------------------------------------------------------
extern SPI_HandleTypeDef SPI_Handler;  // SPI句柄

//-----------------------------------------------------------------
// SPI引脚定义
//-----------------------------------------------------------------
#define SPIx																				SPI1
#define SPI_CLK_ENABLE()                   					__HAL_RCC_SPI1_CLK_ENABLE()  

#define SPI_MISO_PIN                                GPIO_PIN_6
#define SPI_MISO_GPIO_PORT                          GPIOA
#define SPI_MISO_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOA_CLK_ENABLE()  
#define SPI_MISO_GPIO_CLK_DISABLE()                 __HAL_RCC_GPIOA_CLK_DISABLE() 
#define SPI_MISO_AF																	GPIO_AF5_SPI1

#define SPI_MOSI_PIN                                GPIO_PIN_7
#define SPI_MOSI_GPIO_PORT                          GPIOA
#define SPI_MOSI_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOA_CLK_ENABLE()  
#define SPI_MOSI_GPIO_CLK_DISABLE()                 __HAL_RCC_GPIOA_CLK_DISABLE() 
#define SPI_MOSI_AF																	GPIO_AF5_SPI1

#define SPI_CLK_PIN                                 GPIO_PIN_3
#define SPI_CLK_GPIO_PORT                           GPIOB
#define SPI_CLK_GPIO_CLK_ENABLE()                   __HAL_RCC_GPIOB_CLK_ENABLE()  
#define SPI_CLK_GPIO_CLK_DISABLE()                  __HAL_RCC_GPIOB_CLK_DISABLE()  
#define SPI_CLK_AF																	GPIO_AF5_SPI1

//-----------------------------------------------------------------
// 函数声明
//-----------------------------------------------------------------
extern void SPI_Init(void);
extern void SPI1_SetSpeed(u8 SPI_BaudRatePrescaler);
extern u8 SPI1_ReadWriteByte(u8 TxData);

#endif
//-----------------------------------------------------------------
// End Of File
//----------------------------------------------------------------- 

