#ifndef __BSP_UART_H
#define __BSP_UART_H

#include "main.h"



void BspUart_Init(void);
void BspUart_SendByte(uint8_t buff);
void BspUart_SendString(const char *s);

extern volatile uint32_t g_rx_cnt;


extern volatile uint8_t g_rx_byte[64];	

extern volatile uint8_t rx_flag;



#endif
