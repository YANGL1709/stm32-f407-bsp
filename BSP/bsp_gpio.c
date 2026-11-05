#include "main.h"       /* 拿到 RCC / GPIOC 的定义 */
#include "bsp_gpio.h"

void BspLed_Init(void)
{
    RCC->AHB1ENR |= (1U << 2);                        /* 开 GPIOC 时钟 */

    LED_PORT->MODER   &= ~(3U << (LED_PIN * 2));      /* 先清 */
    LED_PORT->MODER   |=  (1U << (LED_PIN * 2));      /* 01 = 通用输出 */

    LED_PORT->OTYPER  &= ~(1U << LED_PIN);            /* 推挽 */
    LED_PORT->OSPEEDR &= ~(3U << (LED_PIN * 2));      /* 低速 */
    LED_PORT->PUPDR   &= ~(3U << (LED_PIN * 2));      /* 无上下拉 */
	
	LED_PORT->BSRR = (1U << LED_PIN);
}

void BspLed_On(void)
{
#if LED_ACTIVE_LOW
    LED_PORT->BSRR = (1U << (LED_PIN + 16));          /* 拉低 = 亮 */
#else
    LED_PORT->BSRR = (1U << LED_PIN);
#endif
}

void BspLed_Off(void)
{
#if LED_ACTIVE_LOW
    LED_PORT->BSRR = (1U << LED_PIN);
#else
    LED_PORT->BSRR = (1U << (LED_PIN + 16));
#endif
}

void BspLed_Toggle(void)
{
    static uint8_t is_on = 0;   /* 逻辑状态，和极性无关 */

    is_on = !is_on;

    if (is_on)
        BspLed_On();
    else
        BspLed_Off();
}
