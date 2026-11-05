#include "main.h"       
#include "bsp_tim.h"
#include "bsp_key.h"

volatile uint32_t g_ms_tick = 0;     /* ★ 必须 volatile */

void Bsp_Tim_Init(void)
{
    RCC->APB1ENR |= (1U << 0);

    TIM2->CR1 = 0;
    TIM2->PSC = 84 - 1;
    TIM2->ARR = 1000 - 1;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR  = ~TIM_SR_UIF;

    TIM2->DIER |= TIM_DIER_UIE;        /* ① 外设级：允许产生更新中断 */

    NVIC_SetPriority(TIM2_IRQn, 5);    /* ② 设优先级（0 最高，15 最低） */
    NVIC_EnableIRQ(TIM2_IRQn);         /* ③ NVIC 级：打开这个中断通道 */

    TIM2->CR1 |= TIM_CR1_CEN;
	
	
}



void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF)       /* 确认是这个中断 */
    {
        TIM2->SR = ~TIM_SR_UIF;      /* ★ 必须清标志 */
		g_ms_tick++;
        Key_Tick();
    }
}
