#ifndef __BSP_KEY_H
#define __BSP_KEY_H

#include <stdint.h>
#include "main.h" 

///* ===== 板级定义：换板子只改这三行 ===== */
//换引脚记得开启对应时钟
#define KEY_PORT          GPIOA
#define KEY_PIN           15
#define KEY1_PORT          GPIOE
#define KEY1_PIN           6

#define KEY_HOLD				0x01
#define KEY_DOWN				0x02
#define KEY_UP					0x04
#define KEY_SINGLE				0x08
#define KEY_DOUBLE				0x10
#define KEY_LONG				0x20
#define KEY_REPEAT				0x40

#define KEY_COUNT				2

#define KEY_1					0
#define KEY_2					1



void Bsp_Key_Init(void);

void Key_Tick(void);
uint8_t Key_GetState(uint8_t Key);
uint8_t Key_Check(uint8_t n, uint8_t Flag);


#endif
