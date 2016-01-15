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

void Set_network(void);

void Reset_W5500(void);

// void LED1_onoff(uint8_t on_off);
// void LED2_onoff(uint8_t on_off);

void USART3_Init(void);
void USART3_Puts(char * str);

void Delay_us(uint8 time_us);
void Delay_ms(uint16 time_ms);

void USART3_Putc(uint8_t c);

uint8_t USART3_Getc(uint8_t c);

// int putchar(int ch);
// int getchar(void);

uint32_t time_return(void);

#endif
