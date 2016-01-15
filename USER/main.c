#include "stm32f10x.h"
#include "stdio.h"
#include "config.h"
#include "util.h"
#include "W5500\w5500.h"
#include "W5500\socket.h"
#include "W5500\SPI2.h"
#include "APPs\loopback.h"
#include "systick.h"
#include "led.h"
#include "zc_adpter.h"
#include <string.h>
#include <stdlib.h>

/* Private typedef -----------------------------------------------------------*/
CONFIG_MSG Config_Msg;
CHCONFIG_TYPE_DEF Chconfig_Type_Def;


/* Private variables ---------------------------------------------------------*/
// Configuration Network Information of W5500
uint8 Enable_DHCP = OFF;
uint8 MAC[6] = {0x00, 0x08, 0xDC, 0x47, 0x47, 0x54};//MAC Address

uint8 IP[4] = {192, 168, 199, 137};//IP Address
uint8 GateWay[4] = {192, 168, 199, 1};//Gateway Address
uint8 SubNet[4] = {255, 255, 255, 0};//SubnetMask Address

//TX MEM SIZE- SOCKET 0-7:2KB
//RX MEM SIZE- SOCKET 0-7:2KB
uint8 txsize[MAX_SOCK_NUM] = {2,2,2,2,2,2,2,2};
uint8 rxsize[MAX_SOCK_NUM] = {2,2,2,2,2,2,2,2};

//FOR TCP Client
//Configuration Network Information of TEST PC
uint8 Dest_IP[4] = {120, 132, 77, 0}; //DST_IP Address
uint16 Dest_PORT = 9100; //DST_IP port


uint8 ch_status[MAX_SOCK_NUM] = { 0, }; /** 0:close, 1:ready, 2:connected */
uint8 TX_BUF[TX_RX_MAX_BUF_SIZE]; // TX Buffer for applications
uint8 RX_BUF[TX_RX_MAX_BUF_SIZE]; // RX Buffer for applications

/* Private function prototypes -----------------------------------------------*/
void RCC_Configuration(void);
void NVIC_Configuration(void);
void GPIO_Configuration(void);
void Timer_Configuration(void);

/* Private functions ---------------------------------------------------------*/
__IO uint32_t Timer2_Counter;
uint32_t my_time;
uint32_t presentTime;
void Timer2_ISR(void)
{
    my_time++;
}

void WIZ_Config(void)
{
    // Call Set_network(): Set network configuration, Init. TX/RX MEM SIZE., and  Set RTR/RCR
    Set_network(); // at util.c
}

u8 Dhcp_Task(void)
{      
    return 0;
}
/*******************************************************************************
* 名称: do_dns
* 功能: 查询DNS报文信息，解析来自DNS服务器的回复
* 形参: 域名 *domain_name    
* 返回: 成功: 返回1, 失败 :返回 -1
* 说明: 
*******************************************************************************/
u8 Dns_Task(u8* name,u8 *ip)
{
  return RET_SUCCESS;
}
/*******************************************************************************
 * 函数名：Get_ChipInfo(void)
 * 描述  ：获取芯片Flash 大小
 * 输入  ：无
 * 输出  ：无
 * 说明  ：
*******************************************************************************/
void Get_ChipInfo(void)
{
   uint32_t ChipUniqueID[3];
  u16 STM32_FLASH_SIZE;
   ChipUniqueID[0] = *(__IO u32 *)(0X1FFFF7F0); // 高字节
   ChipUniqueID[1] = *(__IO u32 *)(0X1FFFF7EC); //
   ChipUniqueID[2] = *(__IO u32 *)(0X1FFFF7E8); // 低字节
   STM32_FLASH_SIZE= *(u16*)(0x1FFFF7E0);    //闪存容量寄存器  
   printf("\r\n########### 芯片的唯一ID为: %X-%X-%X \n",
           ChipUniqueID[0],ChipUniqueID[1],ChipUniqueID[2]);  
   printf("\r\n########### 芯片flash的容量为: %dK \n", STM32_FLASH_SIZE);
   printf("\r\n########### 烧录日期: "__DATE__" - "__TIME__"\n");
      //输出使用固件库版本号
    printf("\r\n########### 代码固件库版本: V %d.%d.%d \n",__STM32F10X_STDPERIPH_VERSION_MAIN,__STM32F10X_STDPERIPH_VERSION_SUB1,__STM32F10X_STDPERIPH_VERSION_SUB2);  
}
/*******************************************************************************
* Function Name  : main
* Description    : Main program.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
int main(void)
{
    u8 res=0;
	delay_init();
    RCC_Configuration();  // Configure the system clocks
    NVIC_Configuration(); // NVIC Configuration
    GPIO_Configuration();
	
    USART3_Init();	

	
    Timer_Configuration();
	LED_GPIO_Configuration();
  	LED1_ON;;		

    Reset_W5500();
    WIZ_SPI_Init();

    ZC_Init();
    HF_ReadDataFormFlash();
    WIZ_Config(); // network config & Call Set_network ();
    Get_ChipInfo();
    presentTime = my_time; // For TCP client's connection request delay
    // Start Application
    printf("\r\n\r\n------------------------------------------- \r\n");
    printf("Loopback using W5500\r\n");  
    printf("------------------------------------------- ");
//    AC_ConfigWifi();

 do{
    res = Dhcp_Task();
    }while(res!=0);
   HF_WakeUp();
    while(1)
    {
     
      HF_Cloudfunc();
      
    }

}

void GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

    // Port A output
    GPIO_InitStructure.GPIO_Pin   = WIZ_SCS | LED1 | LED2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB, WIZ_SCS);
    
    GPIO_InitStructure.GPIO_Pin   =  LED1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_SetBits(GPIOA, LED1); // led off

    // Configure the GPIO ports( USART1 Transmit and Receive Lines)
    // Configure the USART1_Tx as Alternate function Push-Pull
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin   =  USART3_TX;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Configure the USART1_Rx as input floating
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin   = USART3_RX;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // SPI 1
    /* Configure SPIy pins: SCK, MISO and MOSI */
    GPIO_InitStructure.GPIO_Pin   = WIZ_SCLK | WIZ_MISO | WIZ_MOSI;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Port B
    GPIO_InitStructure.GPIO_Pin   = WIZ_RESET ;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_SetBits(GPIOC, WIZ_RESET);
  //  GPIO_ResetBits(GPIOB, WIZ_PWDN);
    GPIO_SetBits(GPIOC, WIZ_RESET);

    // Port B input
    GPIO_InitStructure.GPIO_Pin   = WIZ_INT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

}


/*******************************************************************************
* Function Name  : RCC_Configuration
* Description    : Configures the different system clocks.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void RCC_Configuration(void)
{
ErrorStatus HSEStartUpStatus;

 /* RCC system reset(for debug purpose) */
  RCC_DeInit();

  /* Enable HSE */
  RCC_HSEConfig(RCC_HSE_ON);

  /* Wait till HSE is ready */
  HSEStartUpStatus = RCC_WaitForHSEStartUp();

  if(HSEStartUpStatus == SUCCESS)
  {
    /* Enable Prefetch Buffer */
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

    /* Flash 2 wait state */
    FLASH_SetLatency(FLASH_Latency_2);

    /* HCLK = SYSCLK */
    RCC_HCLKConfig(RCC_SYSCLK_Div1);

    /* PCLK2 = HCLK */
    RCC_PCLK2Config(RCC_HCLK_Div1);

    /* PCLK1 = HCLK/2 */
    RCC_PCLK1Config(RCC_HCLK_Div2);

    /* PLLCLK = 8MHz * 9 = 72 MHz */
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);

    /* Enable PLL */
    RCC_PLLCmd(ENABLE);

    /* Wait till PLL is ready */
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
    {
    }

    /* Select PLL as system clock source */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

    /* Wait till PLL is used as system clock source */
    while(RCC_GetSYSCLKSource() != 0x08)
    {
    }
  }

  /* TIM2 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2| RCC_APB1Periph_SPI2|RCC_APB1Periph_USART3, ENABLE);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOB |RCC_APB2Periph_GPIOC
                        |RCC_APB2Periph_AFIO , ENABLE);



}

/*******************************************************************************
* Function Name  : NVIC_Configuration
* Description    : Configures Vector Table base location.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

#ifdef  VECT_TAB_RAM
  /* Set the Vector Table base location at 0x20000000 */
  NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);

#else  /* VECT_TAB_FLASH  */
  /* Configure one bit for preemption priority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

  /* Enable the USART1 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

//   /* Enable the USART2 Interrupt */
//   NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
//   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//   NVIC_Init(&NVIC_InitStructure);


  /* Enable the TIM2 global Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

   /* Enable the TIM2 global Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Set the Vector Table base location at 0x08000000 */
NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);

  /* Set the Vector Table base location at 0x08002000 -> USE AIP*/
  // NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x2000);
  // NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x4000);
#endif
}



void Timer_Configuration(void)
{

  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 1000;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

  /* Prescaler configuration */
  TIM_PrescalerConfig(TIM2, 71, TIM_PSCReloadMode_Immediate);

  /* TIM enable counter */
  TIM_Cmd(TIM2, ENABLE);

  /* TIM IT enable */
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

}





