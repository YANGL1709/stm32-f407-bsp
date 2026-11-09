#ifndef __BSP_UART_H
#define __BSP_UART_H

#include "main.h"

#define UART_RX_BUF_SIZE  128U

extern volatile uint32_t g_frame_len;      /* 当前这一帧多长 */
extern volatile uint8_t  g_frame_ready;    /* 1 = 有一帧可取 */
extern volatile uint32_t g_uart_drop;      /* 丢了多少字节 */

void BspUart_Init(void);
void BspUart_SendByte(uint8_t buff);
void BspUart_SendString(const char *s);




#endif
