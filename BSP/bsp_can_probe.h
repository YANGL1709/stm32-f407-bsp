#ifndef __BSP_CAN_PROBE_H
#define __BSP_CAN_PROBE_H

#include "main.h"


void BspCanProbe_Init(void);      /* PB9 = 推挽输出，初始隐性 */
void BspCanProbe_Drive(void);       /* 翻转一次：低约 100μs → 高约 100μs */




#endif
