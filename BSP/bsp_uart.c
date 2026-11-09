#include "bsp_uart.h"
#include "main.h"
#include "ring_buff.h"

#define UART1_PORT         GPIOA
#define UART1_TX           9
#define UART1_RX           10


/* 环形缓冲的存储区（改完类型后就是 128 字节） */
uint8_t     s_rx_mem[UART_RX_BUF_SIZE];
stRingBufTdf s_rx;

volatile uint32_t g_frame_len   = 0;
volatile uint8_t  g_frame_ready = 0;
volatile uint32_t g_uart_drop   = 0;





 


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
	
	 vRingBufInit(&s_rx, UART_RX_BUF_SIZE, s_rx_mem);
	
	

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

        if (ucRingBufWrite(&s_rx, b) != 0) /* ★ 0 = 成功 */
            g_uart_drop++;                 /* 满了，记一笔 */
    }
	if (sr & USART_SR_IDLE)
	{
        (void)USART1->DR;                  /* 再读 DR 清 IDLE */
		
        uint32_t n = ucRingBufGetLength(&s_rx);
        if (n > 0)                         /* 池子里有货，才算一帧 */
        {
            g_frame_len   = n;             /* 这一帧多长 */
            g_frame_ready = 1;
	}

}
	
}


 