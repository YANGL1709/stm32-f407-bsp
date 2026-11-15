#ifndef __BSP_SPI_H
#define __BSP_SPI_H

#include "main.h"



void BspSpi_Init(void); 

void BspSpi_Start(void);
void BspSpi_Stop(void);

uint8_t BspSpi_SwapByte(uint8_t byte);




#endif
