#ifndef __BSP_GPIO_H
#define __BSP_GPIO_H

#include <stdint.h>

/* ===== 板级定义：换板子只改这三行 ===== */
#define LED_PORT          GPIOC
#define LED_PIN           13
#define LED_ACTIVE_LOW    1     /* 1 = 低电平点亮 */

void BspLed_Init(void);
void BspLed_On(void);
void BspLed_Off(void);
void BspLed_Toggle(void);

#endif
