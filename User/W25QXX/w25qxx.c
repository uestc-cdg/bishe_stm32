//-----------------------------------------------------------------
// ³ÌÐòÃèÊö:
//     W25QXXÇý¶¯³ÌÐò
// ×÷    Õß: ÁèÖÇµç×Ó
// ¿ªÊ¼ÈÕÆÚ: 2018-08-04
// Íê³ÉÈÕÆÚ: 2018-08-04
// ÐÞ¸ÄÈÕÆÚ: 
// µ±Ç°°æ±¾: V1.0
// ÀúÊ·°æ±¾:
//  - V1.0: (2018-08-04)W25QXX³õÊ¼»¯ºÍÅäÖÃ
// µ÷ÊÔ¹¤¾ß: ÁèÖÇSTM32F429+CycloneIVµç×ÓÏµÍ³Éè¼Æ¿ª·¢°å¡¢LZE_ST_LINK2
// Ëµ    Ã÷: 
//    
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// Í·ÎÄ¼þ°üº¬
//-----------------------------------------------------------------
#include "w25qxx.h"
#include "spi.h"
#include "delay.h"
//-----------------------------------------------------------------

u16 W25QXX_TYPE=W25Q128;	// Ä¬ÈÏÊÇW25Q128
												 
//-----------------------------------------------------------------
// void W25QXX_Init(void)
//-----------------------------------------------------------------
// 
// º¯Êý¹¦ÄÜ: ³õÊ¼»¯SPI FLASHµÄIO¿Ú
// Èë¿Ú²ÎÊý: ÎÞ 
// ·µ »Ø Öµ: ÎÞ
// ×¢ÒâÊÂÏî: ÎÞ
//
//-----------------------------------------------------------------
void W25QXX_Init(void)
{ 
	u8 temp;
	GPIO_InitTypeDef GPIO_Initure;
	__HAL_RCC_GPIOG_CLK_ENABLE();           // Ê¹ÄÜGPIOGÊ±ÖÓ
	
	GPIO_Initure.Pin=GPIO_PIN_3;            // PG3 -> FLASH_CS
	GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  // ÍÆÍìÊä³ö
	GPIO_Initure.Pull=GPIO_PULLUP;          // ÉÏÀ­
	GPIO_Initure.Speed=GPIO_SPEED_FAST;     // ¿ìËÙ         
	HAL_GPIO_Init(GPIOG,&GPIO_Initure);     // ³õÊ¼»¯
    
	W25QXX_CS=1;			                			// È¡ÏûÆ¬Ñ¡  
	SPI_Init();		   			        					// ³õÊ¼»¯SPI
	W25QXX_TYPE=W25QXX_ReadID();	        	// ¶ÁÈ¡FLASH ID.
	if(W25QXX_TYPE==W25Q256)              	// ÈôSPI FLASHÎªW25Q256
	{
		temp=W25QXX_ReadSR(3);            		// ¶ÁÈ¡×´Ì¬¼Ä´æÆ÷3£¬ÅÐ¶ÏµØÖ·Ä£Ê½
		if((temp&0X01)==0)			        			// Èç¹û²»ÊÇ4×Ö½ÚµØÖ·Ä£Ê½,Ôò½øÈë4×Ö½ÚµØÖ·Ä£Ê½
		{
			W25QXX_CS=0; 			        					// Ñ¡ÖÐ
			SPI1_ReadWriteByte(W25X_Enable4ByteAddr);// ·¢ËÍ½øÈë4×Ö½ÚµØÖ·Ä£Ê½Ö¸Áî   
			W25QXX_CS=1;       		        			// È¡ÏûÆ¬Ñ¡   
		}
	}
}  

//-----------------------------------------------------------------
// u8 W25QXX_ReadSR(u8 regno)  
//-----------------------------------------------------------------
// 
// º¯Êý¹¦ÄÜ: ¶ÁÈ¡W25QXXµÄ×´Ì¬¼Ä´æÆ÷£¬W25QXXÒ»¹²ÓÐ3¸ö×´Ì¬¼Ä´æÆ÷
// Èë¿Ú²ÎÊý: u8 regno£º×´Ì¬¼Ä´æÆ÷ºÅ£¬·¶Î§:1~3
// ·µ »Ø Öµ: u8 byte£º×´Ì¬¼Ä´æÆ÷Öµ
// ×¢ÒâÊÂÏî: ×´Ì¬¼Ä´æÆ÷1£º
//					 BIT7  6   5   4   3   2   1   0
//					 SPR   RV  TB BP2 BP1 BP0 WEL BUSY
//					 SPR:Ä¬ÈÏ0,×´Ì¬¼Ä´æÆ÷±£»¤Î»,ÅäºÏWPÊ¹ÓÃ
//					 TB,BP2,BP1,BP0:FLASHÇøÓòÐ´±£»¤ÉèÖÃ
//					 WEL:Ð´Ê¹ÄÜËø¶¨
//					 BUSY:Ã¦±ê¼ÇÎ»(1,Ã¦;0,¿ÕÏÐ)
//					 Ä¬ÈÏ:0x00
//					 ×´Ì¬¼Ä´æÆ÷2£º
//					 BIT7  6   5   4   3   2   1   0
//					 SUS   CMP LB3 LB2 LB1 (R) QE  SRP1
//					 ×´Ì¬¼Ä´æÆ÷3£º
//					 BIT7      6    5    4   3   2   1   0
//					 HOLD/RST  DRV1 DRV0 (R) (R) WPS ADP ADS
// 
//-----------------------------------------------------------------
u8 W25QXX_ReadSR(u8 regno)   
{  
	u8 byte=0,command=0; 
	switch(regno)
	{
		case 1:
				command=W25X_ReadStatusReg1;    // ¶Á×´Ì¬¼Ä´æÆ÷1Ö¸Áî
				break;
		case 2:
				command=W25X_ReadStatusReg2;    // ¶Á×´Ì¬¼Ä´æÆ÷2Ö¸Áî
				break;
		case 3:
				command=W25X_ReadStatusReg3;    // ¶Á×´Ì¬¼Ä´æÆ÷3Ö¸Áî
				break;
		default:
				command=W25X_ReadStatusReg1;    
				break;
	}    
	W25QXX_CS=0;                          // Ê¹ÄÜÆ÷¼þ   
	SPI1_ReadWriteByte(command);          // ·¢ËÍ¶ÁÈ¡×´Ì¬¼Ä´æÆ÷ÃüÁî    
	byte=SPI1_ReadWriteByte(0Xff);        // ¶ÁÈ¡Ò»¸ö×Ö½Ú  
	W25QXX_CS=1;                          // È¡ÏûÆ¬Ñ¡     
	return byte;   
} 

//-----------------------------------------------------------------
// void W25QXX_Write_SR(u8 regno,u8 sr)   
//-----------------------------------------------------------------
// 
// º¯Êý¹¦ÄÜ: Ð´W25QXX×´Ì¬¼Ä´æÆ÷
// Èë¿Ú²ÎÊý: u8 regno£º×´Ì¬¼Ä´æÆ÷ºÅ£¬·¶Î§:1~3
//					 u8 sr£ºÐ´ÈëÒ»¸ö×Ö½Ú  
// ·µ »Ø Öµ: ÎÞ
// ×¢ÒâÊÂÏî: ÎÞ
//
//-----------------------------------------------------------------
void W25QXX_Write_SR(u8 regno,u8 sr)   
{   
	u8 command=0;
	switch(regno)
	{
		case 1:
				command=W25X_WriteStatusReg1;    // Ð´×´Ì¬¼Ä´æÆ÷1Ö¸Áî
				break;
		case 2:
				command=W25X_WriteStatusReg2;    // Ð´×´Ì¬¼Ä´æÆ÷2Ö¸Áî
				break;
		case 3:
				command=W25X_WriteStatusReg3;    // Ð´×´Ì¬¼Ä´æÆ÷3Ö¸Áî
				break;
		default:
				command=W25X_WriteStatusReg1;    
				break;
	}   
	W25QXX_CS=0;                           // Ê¹ÄÜÆ÷¼þ   
	SPI1_ReadWriteByte(command);           // ·¢ËÍÐ´È¡×´Ì¬¼Ä´æÆ÷ÃüÁî    
	SPI1_ReadWriteByte(sr);                // Ð´ÈëÒ»¸ö×Ö½Ú  
	W25QXX_CS=1;                           // È¡ÏûÆ¬Ñ¡     	      
}   
  
//-----------------------------------------------------------------
// void W25QXX_Write_Enable(void) 
//-----------------------------------------------------------------
// 
// º¯Êý¹¦ÄÜ: W25QXXÐ´Ê¹ÄÜ	
// Èë¿Ú²ÎÊý: ÎÞ 
// ·µ »Ø Öµ: ÎÞ
// ×¢ÒâÊÂÏî: ÎÞ
//
//-----------------------------------------------------------------
void W25QXX_Write_Enable(void)   
{
	W25QXX_CS=0;                            // Ê¹ÄÜÆ÷¼þ   
  SPI1_ReadWriteByte(W25X_WriteEnable);   // ·¢ËÍÐ´Ê¹ÄÜ  
	W25QXX_CS=1;                            // È¡ÏûÆ¬Ñ¡     	      
} 
 
//-----------------------------------------------------------------
// void W25QXX_Write_Disable(void) 
//-----------------------------------------------------------------
// 
// º¯Êý¹¦ÄÜ: WW25QXXÐ´½ûÖ¹	
// Èë¿Ú²ÎÊý: ÎÞ 
// ·µ »Ø Öµ: ÎÞ
// ×¢ÒâÊÂÏî: ÎÞ
//
//-----------------------------------------------------------------
void W25QXX_Write_Disable(void)   
{  
	W25QXX_CS=0;                            // Ê¹ÄÜÆ÷¼þ   
  SPI1_ReadWriteByte(W25X_WriteDisable);  // ·¢ËÍÐ´½ûÖ¹Ö¸Áî    
	W25QXX_CS=1;                            // È¡ÏûÆ¬Ñ¡     	      
} 

//-----------------------------------------------------------------
// u16 W25QXX_ReadID(void)
//-----------------------------------------------------------------
// 
// º¯Êý¹¦ÄÜ: ¶ÁÈ¡Ð¾Æ¬ID
// Èë¿Ú²ÎÊý: ÎÞ 
// ·µ »Ø Öµ: u16 Temp£ºÐ¾Æ¬ÐÍºÅ
// ×¢ÒâÊÂÏî: 0XEF13,±íÊ¾Ð¾Æ¬ÐÍºÅÎªW25Q80  
//					 0XEF14,±íÊ¾Ð¾Æ¬ÐÍºÅÎªW25Q16    
//					 0XEF15,±íÊ¾Ð¾Æ¬ÐÍºÅÎªW25Q32  
//					 0XEF16,±íÊ¾Ð¾Æ¬ÐÍºÅÎªW25Q64 
//					 0XEF17,±íÊ¾Ð¾Æ¬ÐÍºÅÎªW25Q128 	  
//					 0XEF18,±íÊ¾Ð¾Æ¬ÐÍºÅÎªW25Q256
//
//-----------------------------------------------------------------
u16 W25QXX_ReadID(void)
{
	u16 Temp = 0;	  
	W25QXX_CS=0;				    
	SPI1_ReadWriteByte(0x90);// ·¢ËÍ¶ÁÈ¡IDÃüÁî	    
	SPI1_ReadWriteByte(0x00); 	    
	SPI1_ReadWriteByte(0x00); 	    
	SPI1_ReadWriteByte(0x00); 	 			   
	Temp|=SPI1_ReadWriteByte(0xFF)<<8;  
	Temp|=SPI1_ReadWriteByte(0xFF);	 
	W25QXX_CS=1;				    
	return Temp;
}   		    

//-----------------------------------------------------------------
// void W25QXX_Read(u8* pBuffer,u32 ReadAddr,u16 NumByteToRead)   
//-----------------------------------------------------------------
// 
// º¯Êý¹¦ÄÜ: ¶ÁÈ¡SPI FLASH£¬ÔÚÖ¸¶¨µØÖ·¿ªÊ¼¶ÁÈ¡Ö¸¶¨³¤¶ÈµÄÊý¾Ý
// Èë¿Ú²ÎÊý: u8* pBuffer£ºÊý¾Ý´æ´¢Çø
//					 u32 ReadAddr:¿ªÊ¼¶ÁÈ¡µÄµØÖ·(24bit)
//					 u16 NumByteToRead:ª¶ÁÈ¡µÄ×Ö½ÚÊý(×î´ó65535)
// ·µ »Ø Öµ: ÎÞ
// ×¢ÒâÊÂÏî: ÎÞ
//
//-----------------------------------------------------------------
void W25QXX_Read(u8* pBuffer,u32 ReadAddr,u16 NumByteToRead)   
{ 
 	u16 i;   										    
	W25QXX_CS=0;                            	// Ê¹ÄÜÆ÷¼þ   
	SPI1_ReadWriteByte(W25X_ReadData);      	// ·¢ËÍ¶ÁÈ¡ÃüÁî  
	if(W25QXX_TYPE==W25Q256)                	// Èç¹ûÊÇW25Q256µÄ»°µØÖ·Îª4×Ö½ÚµÄ£¬Òª·¢ËÍ×î¸ß8Î»
	{
			SPI1_ReadWriteByte((u8)((ReadAddr)>>24));    
	}
	SPI1_ReadWriteByte((u8)((ReadAddr)>>16));	// ·¢ËÍ24bitµØÖ·    
	SPI1_ReadWriteByte((u8)((ReadAddr)>>8));   
	SPI1_ReadWriteByte((u8)ReadAddr);   
	for(i=0;i<NumByteToRead;i++)
	{ 
		pBuffer[i]=SPI1_ReadWriteByte(0XFF);    // Ñ­»·¶ÁÊý  
  }
	W25QXX_CS=1;  				    	      
}  
 
//-----------------------------------------------------------------
// void W25QXX_Write_Page(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite)
//-----------------------------------------------------------------
// 
// º¯Êý¹¦ÄÜ: PIÔÚÒ»Ò³(0~65535)ÄÚÐ´ÈëÉÙÓÚ256¸ö×Ö½ÚµÄÊý¾Ý£¬ÔÚÖ¸¶¨µØÖ·¿ªÊ¼Ð´Èë×î´ó256×Ö½ÚµÄÊý¾Ý
// Èë¿Ú²ÎÊý: u8* pBuffer£ºÊý¾Ý´æ´¢Çø
//					 u32 WriteAddr£º¿ªÊ¼Ð´ÈëµÄµØÖ·(24bit)
//					 u16 NumByteToWrite£ºÒªÐ´ÈëµÄ×Ö½ÚÊý(×î´ó256),¸ÃÊý²»Ó¦¸Ã³¬¹ý¸ÃÒ³µÄÊ£Óà×Ö½ÚÊý!!!
// ·µ »Ø Öµ: ÎÞ
// ×¢ÒâÊÂÏî: ÎÞ
//
//-----------------------------------------------------------------
void W25QXX_Write_Page(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite)
{
 	u16 i;  
  W25QXX_Write_Enable();                  // SET WEL 
	W25QXX_CS=0;                            // Ê¹ÄÜÆ÷¼þ   
	SPI1_ReadWriteByte(W25X_PageProgram);   // ·¢ËÍÐ´Ò³ÃüÁî   
	if(W25QXX_TYPE==W25Q256)                // Èç¹ûÊÇW25Q256µÄ»°µØÖ·Îª4×Ö½ÚµÄ£¬Òª·¢ËÍ×î¸ß8Î»
	{
		SPI1_ReadWriteByte((u8)((WriteAddr)>>24)); 
	}
	SPI1_ReadWriteByte((u8)((WriteAddr)>>16)); // ·¢ËÍ24bitµØÖ·    
	SPI1_ReadWriteByte((u8)((WriteAddr)>>8));   
	SPI1_ReadWriteByte((u8)WriteAddr);   
	for(i=0;i<NumByteToWrite;i++)
		SPI1_ReadWriteByte(pBuffer[i]);			// Ñ­»·Ð´Êý  
	W25QXX_CS=1;                          // È¡ÏûÆ¬Ñ¡ 
	W25QXX_Wait_Busy();					   				// µÈ´ýÐ´Èë½áÊø
} 

//-----------------------------------------------------------------
// void W25QXX_Write_NoCheck(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite) 
//-----------------------------------------------------------------
// 
// º¯Êý¹¦ÄÜ: ÎÞ¼ìÑéÐ´SPI FLASH 
// Èë¿Ú²ÎÊý: u8* pBuffer£ºÊý¾Ý´æ´¢Çø
//					 u32 WriteAddr£º¿ªÊ¼Ð´ÈëµÄµØÖ·(24bit)
//					 u16 NumByteToWrite£ºÒªÐ´ÈëµÄ×Ö½ÚÊý(×î´ó65535)
// ·µ »Ø Öµ: ÎÞ
// ×¢ÒâÊÂÏî: ±ØÐëÈ·±£ËùÐ´µÄµØÖ··¶Î§ÄÚµÄÊý¾ÝÈ«²¿Îª0XFF,·ñÔòÔÚ·Ç0XFF´¦Ð´ÈëµÄÊý¾Ý½«Ê§°Ü!
//					 ¾ßÓÐ×Ô¶¯»»Ò³¹¦ÄÜ!
//					 ÔÚÖ¸¶¨µØÖ·¿ªÊ¼Ð´ÈëÖ¸¶¨³¤¶ÈµÄÊý¾Ý,µ«ÊÇÒªÈ·±£µØÖ·²»Ô½½ç!
//
//-----------------------------------------------------------------
void W25QXX_Write_NoCheck(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite)   
{ 			 		 
	u16 pageremain;	   
	pageremain=256-WriteAddr%256; // µ¥Ò³Ê£ÓàµÄ×Ö½ÚÊý		 	    
	if(NumByteToWrite<=pageremain)
		pageremain=NumByteToWrite;	// ²»´óÓÚ256¸ö×Ö½Ú
	while(1)
	{	   
		W25QXX_Write_Page(pBuffer,WriteAddr,pageremain);
		if(NumByteToWrite==pageremain)
			break;										// Ð´Èë½áÊøÁË
	 	else // NumByteToWrite>pageremain
		{
			pBuffer+=pageremain;
			WriteAddr+=pageremain;	

			NumByteToWrite-=pageremain;			// ¼õÈ¥ÒÑ¾­Ð´ÈëÁËµÄ×Ö½ÚÊý
			if(NumByteToWrite>256)
				pageremain=256; 							// Ò»´Î¿ÉÒÔÐ´Èë256¸ö×Ö½Ú
			else 
				pageremain=NumByteToWrite; 	  // ²»¹»256¸ö×Ö½ÚÁË
		}
	}	    
} 
 
u8 W25QXX_BUFFER[4096];
//-----------------------------------------------------------------
// void W25QXX_Write(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite)  
//-----------------------------------------------------------------
// 
// º¯Êý¹¦ÄÜ: Ð´SPI FLASH  
// Èë¿Ú²ÎÊý: u8* pBuffer£ºÊý¾Ý´æ´¢Çø
//					 u32 WriteAddr£º¿ªÊ¼Ð´ÈëµÄµØÖ·(24bit)
//					 u16 NumByteToWrite£ºÒªÐ´ÈëµÄ×Ö½ÚÊý(×î´ó65535) 
// ·µ »Ø Öµ: ÎÞ
// ×¢ÒâÊÂÏî: ¸Ãº¯Êý´ø²Á³ý²Ù×÷!
//
//-----------------------------------------------------------------
void W25QXX_Write(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite)   
{ 
	u32 secpos;
	u16 secoff;
	u16 secremain;	   
 	u16 i;    
	u8 * W25QXX_BUF;	  
  W25QXX_BUF=W25QXX_BUFFER;	     
 	secpos=WriteAddr/4096;	// ÉÈÇøµØÖ·  
	secoff=WriteAddr%4096;	// ÔÚÉÈÇøÄÚµÄÆ«ÒÆ
	secremain=4096-secoff;	// ÉÈÇøÊ£Óà¿Õ¼ä´óÐ¡   
 	if(NumByteToWrite<=secremain)
		secremain=NumByteToWrite;//²»´óÓÚ4096¸ö×Ö½Ú
	while(1) 
	{	
		W25QXX_Read(W25QXX_BUF,secpos*4096,4096);	// ¶Á³öÕû¸öÉÈÇøµÄÄÚÈÝ
		for(i=0;i<secremain;i++)								 	// Ð£ÑéÊý¾Ý
		{
			if(W25QXX_BUF[secoff+i]!=0XFF)
				break;// ÐèÒª²Á³ý  	  
		}
		if(i<secremain)// ÐèÒª²Á³ý
		{
			W25QXX_Erase_Sector(secpos);	// ²Á³ýÕâ¸öÉÈÇø
			for(i=0;i<secremain;i++)	    // ¸´ÖÆ
			{
				W25QXX_BUF[i+secoff]=pBuffer[i];	  
			}
			W25QXX_Write_NoCheck(W25QXX_BUF,secpos*4096,4096);// Ð´ÈëÕû¸öÉÈÇø  

		}
		else 
			W25QXX_Write_NoCheck(pBuffer,WriteAddr,secremain);// Ð´ÒÑ¾­²Á³ýÁËµÄ,Ö±½ÓÐ´ÈëÉÈÇøÊ£ÓàÇø¼ä. 				   
		if(NumByteToWrite==secremain)
			break;	// Ð´Èë½áÊøÁË
		else			// Ð´ÈëÎ´½áÊø
		{
			secpos++;// ÉÈÇøµØÖ·Ôö1
			secoff=0;// Æ«ÒÆÎ»ÖÃÎª0 	 

		   	pBuffer+=secremain;  	// Ö¸ÕëÆ«ÒÆ
			WriteAddr+=secremain;		// Ð´µØÖ·Æ«ÒÆ	   
		   	NumByteToWrite-=secremain;				// ×Ö½ÚÊýµÝ¼õ
			if(NumByteToWrite>4096)
				secremain=4096;	// ÏÂÒ»¸öÉÈÇø»¹ÊÇÐ´²»Íê
			else 
				secremain=NumByteToWrite;			// ÏÂÒ»¸öÉÈÇø¿ÉÒÔÐ´ÍêÁË
		}	 
	} 
}

//-----------------------------------------------------------------
// void W25QXX_Erase_Chip(void) 
//-----------------------------------------------------------------
// 
// º¯Êý¹¦ÄÜ: ²Á³ýÕû¸öÐ¾Æ¬	
// Èë¿Ú²ÎÊý: ÎÞ 
// ·µ »Ø Öµ: ÎÞ
// ×¢ÒâÊÂÏî: ÎÞ
//
//-----------------------------------------------------------------
void W25QXX_Erase_Chip(void)   
{                                   
	W25QXX_Write_Enable();                  // SET WEL 
	W25QXX_Wait_Busy();   
	W25QXX_CS=0;                            // Ê¹ÄÜÆ÷¼þ   
	SPI1_ReadWriteByte(W25X_ChipErase);     // ·¢ËÍÆ¬²Á³ýÃüÁî  
	W25QXX_CS=1;                            // È¡ÏûÆ¬Ñ¡     	      
	W25QXX_Wait_Busy();   				   				// µÈ´ýÐ¾Æ¬²Á³ý½áÊø
}   

//-----------------------------------------------------------------
// void W25QXX_Erase_Sector(u32 Dst_Addr)  
//-----------------------------------------------------------------
// 
// º¯Êý¹¦ÄÜ: ²Á³ýÒ»¸öÉÈÇø
// Èë¿Ú²ÎÊý: u32 Dst_Addr£ºÉÈÇøµØÖ· ¸ù¾ÝÊµ¼ÊÈÝÁ¿ÉèÖÃ
// ·µ »Ø Öµ: ÎÞ
// ×¢ÒâÊÂÏî: ²Á³ýÒ»¸öÉÈÇøµÄ×îÉÙÊ±¼ä:150ms
//
//-----------------------------------------------------------------
void W25QXX_Erase_Sector(u32 Dst_Addr)   
{   
 	Dst_Addr*=4096;
	W25QXX_Write_Enable();                  // SET WEL 	 
	W25QXX_Wait_Busy();   
	W25QXX_CS=0;                            // Ê¹ÄÜÆ÷¼þ   
	SPI1_ReadWriteByte(W25X_SectorErase);   // ·¢ËÍÉÈÇø²Á³ýÖ¸Áî 
	if(W25QXX_TYPE==W25Q256)                // Èç¹ûÊÇW25Q256µÄ»°µØÖ·Îª4×Ö½ÚµÄ£¬Òª·¢ËÍ×î¸ß8Î»
	{
		SPI1_ReadWriteByte((u8)((Dst_Addr)>>24)); 
	}
	SPI1_ReadWriteByte((u8)((Dst_Addr)>>16));		// ·¢ËÍ24bitµØÖ·    
	SPI1_ReadWriteByte((u8)((Dst_Addr)>>8));   
	SPI1_ReadWriteByte((u8)Dst_Addr);  
	W25QXX_CS=1;                            // È¡ÏûÆ¬Ñ¡     	      
  W25QXX_Wait_Busy();   				    			// µÈ´ý²Á³ýÍê³É
}  

//-----------------------------------------------------------------
// void W25QXX_Wait_Busy(void)   
//-----------------------------------------------------------------
// 
// º¯Êý¹¦ÄÜ: µÈ´ý¿ÕÏÐ
// Èë¿Ú²ÎÊý: ÎÞ 
// ·µ »Ø Öµ: ÎÞ
// ×¢ÒâÊÂÏî: ÎÞ
//
//-----------------------------------------------------------------
void W25QXX_Wait_Busy(void)   
{   
	while((W25QXX_ReadSR(1)&0x01)==0x01);   // µÈ´ýBUSYÎ»Çå¿Õ
}  

//-----------------------------------------------------------------
// void W25QXX_PowerDown(void)  
//-----------------------------------------------------------------
// 
// º¯Êý¹¦ÄÜ: ½øÈëµôµçÄ£Ê½
// Èë¿Ú²ÎÊý: ÎÞ 
// ·µ »Ø Öµ: ÎÞ
// ×¢ÒâÊÂÏî: ÎÞ
//
//-----------------------------------------------------------------
void W25QXX_PowerDown(void)   
{ 
	W25QXX_CS=0;                            // Ê¹ÄÜÆ÷¼þ   
	SPI1_ReadWriteByte(W25X_PowerDown);     // ·¢ËÍµôµçÃüÁî  
	W25QXX_CS=1;                            // È¡ÏûÆ¬Ñ¡     	      
  delay_us(3);                            // µÈ´ýTPD  
}   

//-----------------------------------------------------------------
// void W25QXX_WAKEUP(void)  
//-----------------------------------------------------------------
// 
// º¯Êý¹¦ÄÜ: »½ÐÑ
// Èë¿Ú²ÎÊý: ÎÞ 
// ·µ »Ø Öµ: ÎÞ
// ×¢ÒâÊÂÏî: ÎÞ
//
//-----------------------------------------------------------------
void W25QXX_WAKEUP(void)   
{  
	W25QXX_CS=0;                                // Ê¹ÄÜÆ÷¼þ   
	SPI1_ReadWriteByte(W25X_ReleasePowerDown);  // send W25X_PowerDown command 0xAB    
	W25QXX_CS=1;                                // È¡ÏûÆ¬Ñ¡     	      
  delay_us(3);                                // µÈ´ýTRES1
}   

//-----------------------------------------------------------------
// End Of File
//----------------------------------------------------------------- 
