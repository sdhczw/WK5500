#include "stm32f10x.h"
#include "config.h"
#include "util.h"
#include "W5500\w5500.h"
#include <stdio.h>
#include <stdarg.h>
#include "systick.h"
#include "led.h"
#include "dhcp.h"
#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */


extern CONFIG_MSG Config_Msg;
extern CHCONFIG_TYPE_DEF Chconfig_Type_Def;
extern DHCP_Get     DHCP_GET;
extern uint8 txsize[MAX_SOCK_NUM];
extern uint8 rxsize[MAX_SOCK_NUM];


extern uint8 MAC[6];
extern uint8 IP[4];
extern uint8 GateWay[4];
extern uint8 SubNet[4];
extern uint8 Enable_DHCP;
extern uint8 Dest_IP[4] ;
extern uint16 Dest_PORT ;
uint8 dhcp_ok=0;
uint32	dhcp_time= 0;																		/*DHCP���м���*/
uint8  ip_from;	        //DHCP
void Set_network(void)
{
    uint8 tmp_array[6];
    uint8 i;
     ip_from       = IP_FROM_DHCP;	        //DHCP
    // MAC ADDRESS
    for (i = 0 ; i < 6; i++) Config_Msg.Mac[i] = MAC[i];

    setSHAR(Config_Msg.Mac);

        
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
    printf("\r\nMAC : %.2X.%.2X.%.2X.%.2X.%.2X.%.2X\r\n", tmp_array[0],tmp_array[1],tmp_array[2],tmp_array[3],tmp_array[4],tmp_array[5]);
}

/*******************************************************************************
* ����: W5500_Init
* ����: ����W5500��IP��ַ
* �β�:       
* ����: ��
* ˵��: ip_from=0,��dhcp,������̬IP
*******************************************************************************/
void set_w5500_ip(u8 ip_from)
{	
   uint8 tmp_array[6];
  /*���ƶ����������Ϣ�����ýṹ��*/
  memcpy(Config_Msg.Mac, MAC, 6);
  
  
  /*ʹ��DHCP��ȡIP�����������DHCP�Ӻ���*/		
  if(ip_from==0)								
  {
    /*����DHCP��ȡ��������Ϣ�����ýṹ��*/
    if(dhcp_ok==1)
    {
      printf(" IP from DHCP\r\n");		 
      memcpy(Config_Msg.Lip,DHCP_GET.lip, 4);
      memcpy(Config_Msg.Sub,DHCP_GET.sub, 4);
      memcpy(Config_Msg.Gw,DHCP_GET.gw, 4);
      memcpy(Config_Msg.DNS_Server_IP,DHCP_GET.dns,4);
    }
    
  }
  else
  {
    memcpy(Config_Msg.Lip,IP,4);
    memcpy(Config_Msg.Sub,SubNet,4);
    memcpy(Config_Msg.Gw,GateWay,4);
    memcpy(Config_Msg.DNS_Server_IP,GateWay,4);
  }
  
   
  /*��IP������Ϣд��W5500��Ӧ�Ĵ���*/	
  setSUBR(Config_Msg.Sub);
  setGAR(Config_Msg.Gw);
  setSIPR(Config_Msg.Lip);
  

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
����ԭ�ͣ�USART2_Puts(char * str)
�������ܣ�char *str���ݴ�ӡ������
��ڲ�����char *str
���ڲ�����
��    ע��
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

//����3�����ж�
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

/**
*@brief	 	16λ�ַ���8λ��8λת��
*@param		i:Ҫת��������
*@return	ת���������
*/
uint16 swaps(uint16 i)
{
  uint16 ret=0;
  ret = (i & 0xFF) << 8;
  ret |= ((i >> 8)& 0xFF);
  return ret;	
}
/**
*@brief	 	32λ�ַ��ߵ�λ�任
*@param		i:Ҫת��������
*@return	ת���������
*/
uint32 swapl(uint32 l)
{
  uint32 ret=0;
  ret = (l & 0xFF) << 24;
  ret |= ((l >> 8) & 0xFF) << 16;
  ret |= ((l >> 16) & 0xFF) << 8;
  ret |= ((l >> 24) & 0xFF);
  return ret;
}

/**
*@brief		��һ�� ����ģʽ��unsigned short������ת�������ģʽ��TCP/IP �����ֽڸ�ʽ������.
*@param		Ҫת��������
*@return 	���ģʽ������
*/ 
uint16 htons( 
	uint16 hostshort	/**< A 16-bit number in host byte order.  */
	)
{
#if ( SYSTEM_ENDIAN == _ENDIAN_LITTLE_ )
	return swaps(hostshort);
#else
	return hostshort;
#endif		
}

/**
*@brief		��һ�� ����ģʽ��unsigned long������ת�������ģʽ��TCP/IP �����ֽڸ�ʽ������.
*@param		Ҫת��������
*@return 	���ģʽ������
*/ 
unsigned long htonl(
	unsigned long hostlong		/**< hostshort  - A 32-bit number in host byte order.  */
	)
{
#if ( SYSTEM_ENDIAN == _ENDIAN_LITTLE_ )
	return swapl(hostlong);
#else
	return hostlong;
#endif	
}



/**
*@brief		��һ�����ģʽ��TCP/IP �����ֽڸ�ʽ������ת��������ģʽ��unsigned short������
*@param		Ҫת��������
*@return 	unsigned shortģʽ������
*/ 
unsigned long ntohs(
	unsigned short netshort	/**< netshort - network odering 16bit value */
	)
{
#if ( SYSTEM_ENDIAN == _ENDIAN_LITTLE_ )	
	return htons(netshort);
#else
	return netshort;
#endif		
}


/**
*@brief		��һ�����ģʽ��TCP/IP �����ֽڸ�ʽ������ת��������ģʽ��unsigned long������
*@param		Ҫת��������
*@return 	unsigned longģʽ������
*/ 
unsigned long ntohl(unsigned long netlong)
{
#if ( SYSTEM_ENDIAN == _ENDIAN_LITTLE_ )
	return htonl(netlong);
#else
	return netlong;
#endif		
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



