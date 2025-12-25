//-----------------------------------------------------------------
// 程序描述:
//     SPI驱动程序
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

//-----------------------------------------------------------------
// 头文件包含
//-----------------------------------------------------------------
#include "spi.h"
//-----------------------------------------------------------------

SPI_HandleTypeDef SPI_Handler;  // SPI句柄

//-----------------------------------------------------------------
// void SPI_Init(void)
//-----------------------------------------------------------------
// 
// 函数功能: SPI驱动程序，配置成主机模式 	
// 入口参数: 无 
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void SPI_Init(void)
{
	SPI_Handler.Instance=SPIx;                         // SP1
	SPI_Handler.Init.Mode=SPI_MODE_MASTER;             // 设置SPI工作模式，设置为主模式
	SPI_Handler.Init.Direction=SPI_DIRECTION_2LINES;   // 设置SPI单向或者双向的数据模式:SPI设置为双线模式
	SPI_Handler.Init.DataSize=SPI_DATASIZE_8BIT;       // 设置SPI的数据大小:SPI发送接收8位帧结构
	SPI_Handler.Init.CLKPolarity=SPI_POLARITY_HIGH;    // 串行同步时钟的空闲状态为高电平
	SPI_Handler.Init.CLKPhase=SPI_PHASE_2EDGE;         // 串行同步时钟的第二个跳变沿（上升或下降）数据被采样
	SPI_Handler.Init.NSS=SPI_NSS_SOFT;                 // NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制
	SPI_Handler.Init.BaudRatePrescaler=SPI_BAUDRATEPRESCALER_2;//定义波特率预分频的值:波特率预分频值为2,设置为45M时钟,高速模式
	SPI_Handler.Init.FirstBit=SPI_FIRSTBIT_MSB;        // 指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始
	SPI_Handler.Init.TIMode=SPI_TIMODE_DISABLE;        // 关闭TI模式
	SPI_Handler.Init.CRCCalculation=SPI_CRCCALCULATION_DISABLE;// 关闭硬件CRC校验
	SPI_Handler.Init.CRCPolynomial=7;                  // CRC值计算的多项式
	HAL_SPI_Init(&SPI_Handler);												 // 初始化
	
	__HAL_SPI_ENABLE(&SPI_Handler);                    // 使能SPI1

	SPI1_ReadWriteByte(0Xff);                          // 启动传输
}

//-----------------------------------------------------------------
// void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
//-----------------------------------------------------------------
// 
// 函数功能: SPI底层驱动，时钟使能，引脚配置
// 入口参数: SPI_HandleTypeDef *hspi：SPI句柄
// 返 回 值: 无
// 注意事项: 此函数会被HAL_SPI_Init()调用
//
//-----------------------------------------------------------------
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
	GPIO_InitTypeDef GPIO_Initure;
	
	SPI_MISO_GPIO_CLK_ENABLE();       	// 使能SPI_MISO的GPIO时钟
	SPI_MOSI_GPIO_CLK_ENABLE();       	// 使能SPI_MOSI的GPIO时钟
	SPI_CLK_GPIO_CLK_ENABLE();       		// 使能SPI_CLK的GPIO时钟
	SPI_CLK_ENABLE();        						// 使能SPI1时钟
	
	GPIO_Initure.Pin=SPI_MISO_PIN;					// PA6 -> SPI1_MISO
	GPIO_Initure.Mode=GPIO_MODE_AF_PP;      // 复用推挽输出
	GPIO_Initure.Pull=GPIO_PULLUP;          // 上拉
	GPIO_Initure.Speed=GPIO_SPEED_FAST;     // 快速            
	GPIO_Initure.Alternate=SPI_MISO_AF;   	// 复用为SPI1
	HAL_GPIO_Init(SPI_MISO_GPIO_PORT,&GPIO_Initure);
	
	GPIO_Initure.Pin=SPI_MOSI_PIN;					// PA7 -> SPI1_MOSI
	GPIO_Initure.Mode=GPIO_MODE_AF_PP;      // 复用推挽输出
	GPIO_Initure.Pull=GPIO_PULLUP;          // 上拉
	GPIO_Initure.Speed=GPIO_SPEED_FAST;     // 快速            
	GPIO_Initure.Alternate=SPI_MOSI_AF;   	// 复用为SPI1
	HAL_GPIO_Init(SPI_MOSI_GPIO_PORT,&GPIO_Initure);

	GPIO_Initure.Pin=SPI_CLK_PIN;						// PB3 -> SPI1_SCK
	GPIO_Initure.Mode=GPIO_MODE_AF_PP;      // 复用推挽输出
	GPIO_Initure.Pull=GPIO_PULLUP;          // 上拉
	GPIO_Initure.Speed=GPIO_SPEED_FAST;     // 快速            
	GPIO_Initure.Alternate=SPI_CLK_AF;   		// 复用为SPI1
	HAL_GPIO_Init(SPI_CLK_GPIO_PORT,&GPIO_Initure);		
}

//-----------------------------------------------------------------
// void SPI1_SetSpeed(u8 SPI_BaudRatePrescaler)
//-----------------------------------------------------------------
// 
// 函数功能: SPI速度设置函数
// 入口参数: u8 SPI_BaudRatePrescaler：SPI_BAUDRATEPRESCALER_2~SPI_BAUDRATEPRESCALER_2 256
// 返 回 值: 无
// 注意事项: SPI速度=fAPB1/分频系数，fAPB1时钟一般为45Mhz
//
//-----------------------------------------------------------------
void SPI1_SetSpeed(u8 SPI_BaudRatePrescaler)
{
	assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));	// 判断有效性
	__HAL_SPI_DISABLE(&SPI_Handler);            									 	// 关闭SPI
	SPI_Handler.Instance->CR1&=0XFFC7;          									 	// 位3-5清零，用来设置波特率
	SPI_Handler.Instance->CR1|=SPI_BaudRatePrescaler;						 		// 设置SPI速度
	__HAL_SPI_ENABLE(&SPI_Handler);             									 	// 使能SPI 
}

//-----------------------------------------------------------------
// u8 SPI1_ReadWriteByte(u8 TxData)
//-----------------------------------------------------------------
// 
// 函数功能: SPI1读写一个字节
// 入口参数: u8 TxData： 要写入的字节
// 返 回 值: u8 Rxdata：读取到的字节
// 注意事项: 无
//
//-----------------------------------------------------------------
u8 SPI1_ReadWriteByte(u8 TxData)
{
	u8 Rxdata;
	HAL_SPI_TransmitReceive(&SPI_Handler,&TxData,&Rxdata,1, 1000);       
 	return Rxdata;          		    // 返回收到的数据		
}

//-----------------------------------------------------------------
// End Of File
//----------------------------------------------------------------- 
