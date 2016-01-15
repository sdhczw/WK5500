/****************************************************************************
  * 文件名    LED.c 
  * 功能      led灯控制函数
  * 库版本    
  * 日期      2013-11-09
  * 实验平台 :YDWIZ010.PCB(20131103) 
  * //硬件连接说明
	LED1---PC2,低电平点亮
******************************************************************************/ 
#include "stm32f10x.h"
#include "led.h"  

/****************************************** 
函数名称：void LED_GPIO_Configuration(void)
功    能：端口配置
参    数：无 
返回值  ：无 
*******************************************/ 
void LED_GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_APB2PeriphClockCmd( RCC_GPIO_LED , ENABLE); 						 
/**
 *	LED -> PC2
 */					 
  GPIO_InitStructure.GPIO_Pin = DS1_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
  GPIO_Init(GPIO_LED, &GPIO_InitStructure);
}

/****************************************** 
函数名称：void LED_Test(void)
功    能：端口配置
参    数：无 
返回值  ：无 
*******************************************/ 
void LED_Test(void)
{
			/*====LED-ON=======*/
		LED1_ON;
		Delay(0xfffff);
		Delay(0xfffff);
		Delay(0x5ffff);	

		/*====LED-OFF=======*/ 
		LED1_OFF;
		Delay(0xfffff);
		Delay(0xfffff);
		Delay(0x5ffff);		
}

/****************************************** 
函数名称：void  Delay (uint32_t nCount)
功    能：端口配置
参    数： 
返回值  ：无 
*******************************************/ 
void  Delay (uint32_t nCount)
{
  for(; nCount != 0; nCount--);
}

