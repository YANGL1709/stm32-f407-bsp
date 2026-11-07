#ifndef __BSP_UART_H
#define __BSP_UART_H

#include "main.h"



void BspUart_Init(void);
void BspUart_SendByte(uint8_t buff);
void BspUart_SendString(const char *s);



#endif
