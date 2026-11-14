#include "bsp_can_probe.h"


void BspCanProbe_Init(void)
{
	RCC->AHB1ENR |= (1U << 1);                 /* 开 GPIOB 时钟 */

	/* PB9 = CAN1_TX，暂时当普通推挽输出 */
	GPIOB->MODER   &= ~(3U << (9 * 2));
	GPIOB->MODER   |=  (1U << (9 * 2));        /* 01 = 通用输出 */
	GPIOB->OTYPER  &= ~(1U << 9);              /* 推挽 */
	GPIOB->OSPEEDR &= ~(3U << (9 * 2));
	GPIOB->OSPEEDR |=  (1U << (9 * 2));  
	

}

void BspCanProbe_Drive(void)
{
		    GPIOB->BSRR = (1U << (9 + 16));        /* PB9 = 0 → 显性 */
		for (volatile uint32_t i = 0; i < 200U; i++) { }

    GPIOB->BSRR = (1U << 9);               /* PB9 = 1 → 隐性 */
	  	for (volatile uint32_t i = 0; i < 200U; i++) { }
		  /* 半周期约 100μs。
   千万别把这个延时改大 —— 收发器有 TXD 显性超时保护（约 1~5ms），
   低电平持续超过阈值会关闭驱动器，表现为"总线完全没有显性"。 */


}
