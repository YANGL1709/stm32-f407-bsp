#include "w25q32.h"

void W25Q32_Init(void)
{
	 BspSpi_Init();
}

void W25Q32_ReadID(uint8_t *mid, uint8_t *did, uint8_t *did2)
{
    

    BspSpi_Start();
    (void)BspSpi_SwapByte(0x9F);          /* JEDEC ID √¸¡Ó */
    *mid = BspSpi_SwapByte(0xFF);         /* Manufacturer */
    *did  = BspSpi_SwapByte(0xFF);         /* Memory Type */
    *did2   = BspSpi_SwapByte(0xFF);         /* Capacity */
    
    BspSpi_Stop();
}	


