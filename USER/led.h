/****************************************************************************
  * 文件名    LED.h
  * 功能      led灯控制函数
  *  编者:    lkh 
  * 日期      2013-11-09
  * 实验平台 :YDWIZ010.PCB(20131103) 
  * //硬件连接说明
	LED1---PC2,低电平点亮
******************************************************************************/ 
#include "stm32f10x.h"

/*LED灯相关定义*/
#define RCC_GPIO_LED                    RCC_APB2Periph_GPIOC    /*LED使用的GPIO时钟*/

#define LEDn                            1                       /*LED数量*/
#define GPIO_LED                        GPIOC                  /*LED灯使用的GPIO组*/

#define DS1_PIN                         GPIO_Pin_2              /*DS1使用的GPIO管脚*/

#define LED1_ON			GPIO_ResetBits(GPIO_LED , DS1_PIN)
#define LED1_OFF		GPIO_SetBits(GPIO_LED , DS1_PIN)

/****************************************** 
函数名称：void LED_GPIO_Configuration(void)
功    能：端口配置
参    数：无 
返回值  ：无 
*******************************************/ 
void LED_GPIO_Configuration(void);

/****************************************** 
函数名称：void LED_Test(void)
功    能：端口配置
参    数：无 
返回值  ：无 
*******************************************/ 
void LED_Test(void);
/****************************************** 
函数名称：void  Delay (uint32_t nCount)
功    能：端口配置
参    数： 
返回值  ：无 
*******************************************/ 
void  Delay (uint32_t nCount);

