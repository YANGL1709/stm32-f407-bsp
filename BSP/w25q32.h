#ifndef __W25Q32_H
#define __W25Q32_H


#include "bsp_spi.h"

void W25Q32_Init(void);
void W25Q32_ReadID(uint8_t *mid, uint8_t *did, uint8_t *did2);








#endif
