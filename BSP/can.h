#ifndef __CAN_H
#define __CAN_H

#include "main.h"

typedef struct
{
    uint32_t id;        /* 11 位或 29 位，统一放这里 */
    uint8_t  ide;       /* 0 = 标准帧(11位)，1 = 扩展帧(29位) */
    uint8_t  len;       /* DLC 0-8 */
    uint8_t  data[8];
} RxMsg;

/* 发送：ide=0 标准帧，ide=1 扩展帧 */
void CAN_SendMsg(uint32_t id, uint8_t ide, uint8_t *data, uint8_t len);
void CAN_ReceiveMsg(RxMsg rxMsg[], uint8_t *msgCount);
void CAN_Init(void);

uint8_t CAN_PopRx(RxMsg *msg);        //主循环取一帧，1=取到 0=空

extern volatile uint32_t g_can_rx;    //收到总帧数
extern volatile uint32_t g_can_drop;  //队列满丢掉的帧数
extern volatile uint32_t g_can_ovr;      //硬件 FIFO 溢出次数




#endif
