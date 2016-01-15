/****************************************************************************
  * �ļ���    LED.h
  * ����      led�ƿ��ƺ���
  *  ����:    lkh 
  * ����      2013-11-09
  * ʵ��ƽ̨ :YDWIZ010.PCB(20131103) 
  * //Ӳ������˵��
	LED1---PC2,�͵�ƽ����
******************************************************************************/ 
#include "stm32f10x.h"

/*LED����ض���*/
#define RCC_GPIO_LED                    RCC_APB2Periph_GPIOC    /*LEDʹ�õ�GPIOʱ��*/

#define LEDn                            1                       /*LED����*/
#define GPIO_LED                        GPIOC                  /*LED��ʹ�õ�GPIO��*/

#define DS1_PIN                         GPIO_Pin_2              /*DS1ʹ�õ�GPIO�ܽ�*/

#define LED1_ON			GPIO_ResetBits(GPIO_LED , DS1_PIN)
#define LED1_OFF		GPIO_SetBits(GPIO_LED , DS1_PIN)

/****************************************** 
�������ƣ�void LED_GPIO_Configuration(void)
��    �ܣ��˿�����
��    ������ 
����ֵ  ���� 
*******************************************/ 
void LED_GPIO_Configuration(void);

/****************************************** 
�������ƣ�void LED_Test(void)
��    �ܣ��˿�����
��    ������ 
����ֵ  ���� 
*******************************************/ 
void LED_Test(void);
/****************************************** 
�������ƣ�void  Delay (uint32_t nCount)
��    �ܣ��˿�����
��    ���� 
����ֵ  ���� 
*******************************************/ 
void  Delay (uint32_t nCount);

