#include "bsp_uart.h"
#include "main.h"

#define UART1_PORT         GPIOA
#define UART1_TX           9
#define UART1_RX           10

volatile uint32_t g_rx_cnt;

volatile uint8_t g_rx_byte[64];	

volatile uint8_t rx_flag=0;
 


void BspUart_Init(void)
{
	RCC->AHB1ENR |= (1U << 0);                        /* 开 GPIOA 时钟 */
	RCC->APB2ENR |= (1U << 4);                         /* 开 uart 时钟 */

    UART1_PORT->MODER   &= ~(3U << (UART1_TX * 2));      /* 先清 */
    UART1_PORT->MODER   |=  (2U << (UART1_TX * 2));      /* 10 = 复用 */
	UART1_PORT->MODER   &= ~(3U << (UART1_RX * 2));      /* 先清 */
    UART1_PORT->MODER   |=  (2U << (UART1_RX * 2)); 

    UART1_PORT->OTYPER  &= ~(1U << UART1_TX);            /* 推挽 */
	
	
    UART1_PORT->OSPEEDR &= ~(3U << (UART1_TX * 2));      /* 先清 */
	UART1_PORT->OSPEEDR |= (1U << (UART1_TX * 2));      //中速
	
    UART1_PORT->PUPDR   &= ~(3U << (UART1_TX * 2));      /* 无上下拉 */
	UART1_PORT->PUPDR   &= ~(3U << (UART1_RX * 2)); 
	UART1_PORT->PUPDR   |=  (1U << (UART1_RX * 2));      //上拉
	
	UART1_PORT->AFR[1]     &=~(0xfU<<4);
	UART1_PORT->AFR[1]    |= (7U<<4);
	UART1_PORT->AFR[1]     &=~(0xfU<<8);
	UART1_PORT->AFR[1]    |= (7U<<8);
	
	USART1->BRR=84000000U/115200U;
	USART1->CR1=USART_CR1_TE | USART_CR1_RE | USART_CR1_UE| USART_CR1_RXNEIE | USART_CR1_IDLEIE;
	
	NVIC_SetPriority(USART1_IRQn, 4);       /* 比 TIM2(5) 高一点，串口容易溢出 */
    NVIC_EnableIRQ(USART1_IRQn);
	
	
	

}

void BspUart_SendByte(uint8_t buff)
{
    while (!(USART1->SR & USART_SR_TXE));   /* 等发送寄存器空 */
    USART1->DR = buff;                          /* 写 DR 即发送 */
}

void BspUart_SendString(const char *s)
{
    while (*s) BspUart_SendByte((uint8_t)*s++); 
		
}

void USART1_IRQHandler(void)
{
	
	 uint32_t sr = USART1->SR;   
	
    if (sr & USART_SR_RXNE)
    {
        uint8_t b = (uint8_t)USART1->DR;   /* 读 DR：清 RXNE（顺带清 IDLE） */

        if (g_rx_cnt < (sizeof(g_rx_byte) - 1U))
            g_rx_byte[g_rx_cnt++] = b;
    }
	if (sr & USART_SR_IDLE)
	{
        (void)USART1->DR;                  /* 再读 DR 清 IDLE */
        g_rx_byte[g_rx_cnt] = 0;           /* ★ 补结束符 */
        rx_flag  = 1;
        g_rx_cnt = 0;
	}

}


 