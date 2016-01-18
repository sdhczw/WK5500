/*
*
@file		util.h
@brief	
*/

#ifndef _UTIL_H
#define _UTIL_H

#define uint8_t unsigned char
#define uint16  unsigned short int
#define uint8   unsigned char
#define uint32_t unsigned int

//=================================================
typedef struct _CONFIG_MSG
{
	uint8 Mac[6];
	uint8 Lip[4];
	uint8 Sub[4];
	uint8 Gw[4];
	uint8 DNS_Server_IP[4];	
	uint8  DHCP;
}
CONFIG_MSG;

extern CONFIG_MSG Config_Msg;

void Set_network(void);

void Reset_W5500(void);

// void LED1_onoff(uint8_t on_off);
// void LED2_onoff(uint8_t on_off);

void USART3_Configuration(void);
void USART3_Puts(char * str);
void USART1_Configuration(void);
void Delay_us(uint8 time_us);
void Delay_ms(uint16 time_ms);

void USART3_Putc(uint8_t c);

uint8_t USART3_Getc(uint8_t c);

// int putchar(int ch);
// int getchar(void);

uint32_t time_return(void);

uint16 htons( unsigned short hostshort);	/* htons function converts a unsigned short from host to TCP/IP network byte order (which is big-endian).*/

unsigned long  htonl(unsigned long hostlong);		/* htonl function converts a unsigned long from host to TCP/IP network byte order (which is big-endian). */

unsigned long  ntohs(unsigned short netshort);		/* ntohs function converts a unsigned short from TCP/IP network byte order to host byte order (which is little-endian on Intel processors). */

unsigned long  ntohl(unsigned long netlong);		/* ntohl function converts a u_long from TCP/IP network order to host byte order (which is little-endian on Intel processors). */
void set_w5500_ip(uint8_t ip_from);
#endif
