#ifndef __CAN_H
#define __CAN_H

#include "main.h"

typedef struct
{
	uint16_t stdID;
	uint8_t  data[8];
	uint8_t   len;

} RxMsg;	


void CAN_Init(void);
void CAN_SendMsg(uint32_t stdID,uint8_t *data,uint8_t len);
void CAN_ReceiveMsg(RxMsg rxMsg[],uint8_t * msgCount);	



#endif
