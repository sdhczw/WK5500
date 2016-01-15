/**--------------File Info---------------------------------------------------------------------------------
** File name:               uart3.c
** Descriptions:            The USART3 application function
**
**--------------------------------------------------------------------------------------------------------
** Created by:              feiji
** Created date:            20131109
** Version:                 v1.0
** Descriptions:            The original version
**
//YDWIZ010.PCB(20131109)
//硬件连接：Uart3(PB10 PB11)，使用平行串口线连接到PC机。


**--------------------------------------------------------------------------------------------------------
** Modified by:             
** Modified date:           
** Version:                 
** Descriptions:            
**
*********************************************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "uart3.h"
#include "systick.h"
#include "stm32f10x_systick.h"
#include "led.h"

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/***********************************************************
函数原型：void USART3_Configuration(void)
函数功能：YART2串口配置初始化程序
入口参数：
出口参数：
备    注：
***********************************************************/
void USART3_Configuration(void)
{ 
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure; 
  NVIC_InitTypeDef NVIC_InitStructure;

  RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO,ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);

  /*
  *  USART3_TX -> PB10, USART3_RX ->	PB11   485E -》PB5
  */				
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;	         
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_Init(GPIOB, &GPIO_InitStructure);		   

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;	        
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_Init(GPIOB, &GPIO_InitStructure);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	//USART3
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);			  

  USART_InitStructure.USART_BaudRate = USART3_BaudRate;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  USART_Init(USART3, &USART_InitStructure); 
  USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
//  USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
  USART_ClearFlag(USART3,USART_FLAG_TC);
  USART_Cmd(USART3, ENABLE);	  
}

/***********************************************************
函数原型：void USART2_Putc(unsigned char c)
函数功能：将uchar c内容打印到串口
入口参数：uchar c
出口参数：
备    注：
***********************************************************/
void USART3_Putc(unsigned char c)
{
    USART_SendData(USART3, c);
    while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET );
}

/***********************************************************
函数原型：USART2_Puts(char * str)
函数功能：char *str内容打印到串口
入口参数：char *str
出口参数：
备    注：
***********************************************************/
void USART3_Puts(char * str)
{
    while(*str)
    {
        USART_SendData(USART3, *str++);
        while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
    }
}

//串口2接收中断
void USART3_IRQHandler(void)
{
	  uint8_t str1;     
  	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
  	{
	    USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);  
			str1=USART_ReceiveData(USART3); 
			
			if(str1 == 0x77)
			{
					LED1_ON;
			}
			else if(str1 == 0x88)
			{
					LED1_OFF;
			}			
		  
			USART_ITConfig( USART3,USART_IT_RXNE, ENABLE);	
  	}
}

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART */
  USART_SendData(USART3, (uint8_t) ch);

  /* Loop until the end of transmission */
  while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET)
  {}
  return ch;
}

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/

