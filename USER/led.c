/****************************************************************************
  * �ļ���    LED.c 
  * ����      led�ƿ��ƺ���
  * ��汾    
  * ����      2013-11-09
  * ʵ��ƽ̨ :YDWIZ010.PCB(20131103) 
  * //Ӳ������˵��
	LED1---PC2,�͵�ƽ����
******************************************************************************/ 
#include "stm32f10x.h"
#include "led.h"  

/****************************************** 
�������ƣ�void LED_GPIO_Configuration(void)
��    �ܣ��˿�����
��    ������ 
����ֵ  ���� 
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
�������ƣ�void LED_Test(void)
��    �ܣ��˿�����
��    ������ 
����ֵ  ���� 
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
�������ƣ�void  Delay (uint32_t nCount)
��    �ܣ��˿�����
��    ���� 
����ֵ  ���� 
*******************************************/ 
void  Delay (uint32_t nCount)
{
  for(; nCount != 0; nCount--);
}

