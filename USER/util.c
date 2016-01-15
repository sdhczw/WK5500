#include "stm32f10x.h"
#include "config.h"
#include "util.h"
#include "W5500\w5500.h"
#include <stdio.h>
#include <stdarg.h>
#include "systick.h"
#include "led.h"

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */


extern CONFIG_MSG Config_Msg;
extern CHCONFIG_TYPE_DEF Chconfig_Type_Def;

extern uint8 txsize[MAX_SOCK_NUM];
extern uint8 rxsize[MAX_SOCK_NUM];


extern uint8 MAC[6];
extern uint8 IP[4];
extern uint8 GateWay[4];
extern uint8 SubNet[4];
extern uint8 Enable_DHCP;
extern uint8 Dest_IP[4] ;
extern uint16 Dest_PORT ;

void Set_network(void)
{
    uint8 tmp_array[6];
    uint8 i;

    // MAC ADDRESS
    for (i = 0 ; i < 6; i++) Config_Msg.Mac[i] = MAC[i];
    // Local IP ADDRESS
    Config_Msg.Lip[0] = IP[0]; Config_Msg.Lip[1] = IP[1]; Config_Msg.Lip[2] = IP[2]; Config_Msg.Lip[3] = IP[3];
    // GateWay ADDRESS
    Config_Msg.Gw[0] = GateWay[0]; Config_Msg.Gw[1] = GateWay[1]; Config_Msg.Gw[2] = GateWay[2]; Config_Msg.Gw[3] = GateWay[3];
    // Subnet Mask ADDRESS
    Config_Msg.Sub[0] = SubNet[0]; Config_Msg.Sub[1] = SubNet[1]; Config_Msg.Sub[2] = SubNet[2]; Config_Msg.Sub[3] = SubNet[3];

    setSHAR(Config_Msg.Mac);
    setSUBR(Config_Msg.Sub);
    setGAR(Config_Msg.Gw);
    setSIPR(Config_Msg.Lip);
        
    // Set DHCP
    Config_Msg.DHCP = Enable_DHCP;
    //Destination IP address for TCP Client
    Chconfig_Type_Def.destip[0] = Dest_IP[0]; Chconfig_Type_Def.destip[1] = Dest_IP[1];
    Chconfig_Type_Def.destip[2] = Dest_IP[2]; Chconfig_Type_Def.destip[3] = Dest_IP[3];
    Chconfig_Type_Def.port = Dest_PORT;

    //Set PTR and RCR register
    setRTR(2000);
    setRCR(5);

    //Init. TX & RX Memory size
    sysinit(txsize, rxsize);

    printf("\r\n----------------------------------------- \r\n");
    printf("W5500E01-M3                       \r\n");
    printf("Network Configuration Information \r\n");
    printf("----------------------------------------- ");

    getSHAR(tmp_array);
    printf("\r\nMAC : %.2X.%.2X.%.2X.%.2X.%.2X.%.2X", tmp_array[0],tmp_array[1],tmp_array[2],tmp_array[3],tmp_array[4],tmp_array[5]);

    getSIPR (tmp_array);
    printf("\r\nIP : %d.%d.%d.%d", tmp_array[0],tmp_array[1],tmp_array[2],tmp_array[3]);

    getSUBR(tmp_array);
    printf("\r\nSN : %d.%d.%d.%d", tmp_array[0],tmp_array[1],tmp_array[2],tmp_array[3]);

    getGAR(tmp_array);
    printf("\r\nGW : %d.%d.%d.%d", tmp_array[0],tmp_array[1],tmp_array[2],tmp_array[3]);
}


void Reset_W5500(void)
{
    GPIO_ResetBits(GPIOC, WIZ_RESET);
    Delay_us(2);
    GPIO_SetBits(GPIOC, WIZ_RESET);
    Delay_ms(1500);
}

// void LED1_onoff(uint8_t on_off)
// {
//     if (on_off == ON) {
//         GPIO_ResetBits(GPIOA, LED1); // LED on
//     }else {
//         GPIO_SetBits(GPIOA, LED1); // LED off
//     }
// }

// void LED2_onoff(uint8_t on_off)
// {
//     if (on_off == ON) {
//         GPIO_ResetBits(GPIOA, LED2); // LED on
//     }else {
//         GPIO_SetBits(GPIOA, LED2); // LED off
//     }
// }


/*******************************************************************************
* USARTx configured as follow:
    - BaudRate = 115200 baud
    - Word Length = 8 Bits
    - One Stop Bit
    - No parity
    - Hardware flow control disabled (RTS and CTS signals)
    - Receive and transmit enabled
*******************************************************************************/
void USART3_Init(void)
{
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    /* Configure the USARTx */
    USART_Init(USART3, &USART_InitStructure);
	  USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
//  USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
  USART_ClearFlag(USART3,USART_FLAG_TC);
    /* Enable the USARTx */
    USART_Cmd(USART3, ENABLE);
}


/*******************************************************************************
* Function Name  : Delay_us
* Description    : Delay per micro second.
* Input          : time_us
* Output         : None
* Return         : None
*******************************************************************************/
void Delay_us( u8 time_us )
{
		delay_us(time_us);
//   register u8 i;
//   register u8 j;
//   for( i=0;i<time_us;i++ )
//   {
//     for( j=0;j<5;j++ )          // 25CLK
//     {
//       asm("nop");       //1CLK
//       asm("nop");       //1CLK
//       asm("nop");       //1CLK
//       asm("nop");       //1CLK
//       asm("nop");       //1CLK
//     }
//   }                              // 25CLK*0.04us=1us
}

/*******************************************************************************
* Function Name  : Delay_ms
* Description    : Delay per mili second.
* Input          : time_ms
* Output         : None
* Return         : None
*******************************************************************************/

void Delay_ms( u16 time_ms )
{
		delay_ms(time_ms);
//   register u16 i;
//   for( i=0;i<time_ms;i++ )
//   {
//     Delay_us(250);
//     Delay_us(250);
//     Delay_us(250);
//     Delay_us(250);
//   }
}

/*******************************************************************************
* Function Name  : PUTCHAR_PROTOTYPE
* Description    : Retargets the C library printf function to the USART.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
/*
PUTCHAR_PROTOTYPE
{
  // Write a character to the USART
  USART_SendData(USART1, (uint8_t) ch);

  //  Loop until the end of transmission
  while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
  {
  }

  return ch;
}
*/

// int putchar(int ch)
// {
//   // Write a character to the USART
//   USART_SendData(USART3, (uint8_t) ch);

//   //  Loop until the end of transmission
//   while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET)
//   {
//   }

//   return ch;
// }

void USART3_Putc(uint8_t c)
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


// int getchar(void)
// {
//     int ch;
//     while(USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == RESET)
//     {
//     }
//     ch = USART_ReceiveData(USART3);
//     return ch;
// }

uint8_t USART3_Getc(uint8_t c)
{
    int ch;
    while(USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == RESET)
    {
    }
    ch = USART_ReceiveData(USART3);
    return ch;
}

//串口3接收中断
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


/*******************************************************************************
* Function Name  : time_return
* Description    :
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint32_t time_return(void)
{
    extern uint32_t my_time;
    return my_time;
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

#ifdef USE_FULL_ASSERT
/*******************************************************************************
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert_param error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert_param error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif



