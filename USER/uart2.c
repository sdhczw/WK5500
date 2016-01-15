/**--------------File Info---------------------------------------------------------------------------------
** File name:               uart2.c
** Descriptions:            The USART2 application function
**
**--------------------------------------------------------------------------------------------------------
** Created by:              feiji
** Created date:            20131109
** Version:                 v1.0
** Descriptions:            The original version
**
//YDWIZ010.PCB(20131109)
//硬件连接：Uart(PA2 PA3)，使用平行串口线连接到PC机。


**--------------------------------------------------------------------------------------------------------
** Modified by:             
** Modified date:           
** Version:                 
** Descriptions:            
**
*********************************************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "uart2.h"
#include "systick.h"
#include "stm32f10x_systick.h"
#include "led.h"

// #ifdef __GNUC__
//   /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
//      set to 'Yes') calls __io_putchar() */
//   #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
// #else
//   #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
// #endif /* __GNUC__ */

/***********************************************************
函数原型：void USART2_Configuration(void)
函数功能：YART2串口配置初始化程序
入口参数：
出口参数：
备    注：
***********************************************************/
void USART2_Configuration(void)
{ 
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure; 
	NVIC_InitTypeDef NVIC_InitStructure;

  RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA,ENABLE);
  RCC_APB1PeriphClockCmd( RCC_APB1Periph_USART2,ENABLE);
  /*
  *  USART2_TX -> PA2 , USART2_RX ->	PA3
  */				
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;	         
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_Init(GPIOA, &GPIO_InitStructure);		   

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;	        
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  USART_InitStructure.USART_BaudRate = USART2_BaudRate;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  USART_Init(USART2, &USART_InitStructure); 
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
//  USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
//  USART_ClearFlag(USART2,USART_FLAG_TC);
  USART_Cmd(USART2, ENABLE); 
	
		//中断处理
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	//USART3
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);			  
}

/***********************************************************
函数原型：void MAX485ENABLE_Configuration(void)
函数功能：485使能引脚控制
入口参数：
出口参数：
备    注：PC4
***********************************************************/
void MAX485ENABLE_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

  RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC,ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;	         
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//GPIO_Mode_Out_PP; 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  MAX485_Enable_L;
}

/***********************************************************
函数原型：void USART2_Putc(unsigned char c)
函数功能：将uchar c内容打印到串口
入口参数：uchar c
出口参数：
备    注：
***********************************************************/
void USART2_Putc(unsigned char c)
{
		MAX485_Enable_H;
	  delay_us(100);
    USART_SendData(USART2, c);
    while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET );
	  delay_us(1000);
		MAX485_Enable_L;
}

/***********************************************************
函数原型：USART2_Puts(char * str)
函数功能：char *str内容打印到串口
入口参数：char *str
出口参数：
备    注：
***********************************************************/
void USART2_Puts(char * str)
{
	  MAX485_Enable_H;
	  delay_us(100);
    while(*str)
    {
        USART_SendData(USART2, *str++);
        while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    }
		delay_us(1000);
		MAX485_Enable_L;
}

//串口2接收中断
void USART2_IRQHandler(void)
{
	  uint8_t str1;     
  	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
  	{
	    USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);  
			str1=USART_ReceiveData(USART2); 
			
			if(str1 == 0x55)
			{
					LED1_ON;
			}
			else if(str1 == 0xaa)
			{
					LED1_OFF;
			}
			
//			USART2_Putc(str1);
//		MAX485_Putc(str1);
//		receive_data(&str1,1,1);
//		put_data_to_fifo(&str1,1);		 
		  
			USART_ITConfig( USART2,USART_IT_RXNE, ENABLE);	
  	}
}

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
// PUTCHAR_PROTOTYPE
// {
//   /* Place your implementation of fputc here */
//   /* e.g. write a character to the USART */
// 	MAX485_Enable_H;
// 	delay_us(100);
// 	
//   USART_SendData(USART2, (uint8_t) ch);

//   /* Loop until the end of transmission */
//   while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
//   {}
// 	delay_us(1000);
//   MAX485_Enable_L;
//   return ch;
// }

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/

