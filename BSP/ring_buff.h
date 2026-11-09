#ifndef __RING_BUFF_H
#define __RING_BUFF_H

#include "main.h"

typedef struct 
{
	volatile uint32_t ulReadIdx;
	volatile uint32_t ulWriteIdx;
	volatile uint32_t ulLength;
	volatile uint8_t *pucBffer;


}stRingBufTdf,*psRingBufTdf;


void vRingBufInit(psRingBufTdf pstRingBuf, uint32_t u1Len,uint8_t *pucBuf);  

uint8_t ucRingBufWrite (psRingBufTdf pstRingBuf, uint8_t ucValue);
//计算下一个写位置，判断满没满
// 读一个字节数据
uint8_t ucRingBufRead(psRingBufTdf pstRingBuf, uint8_t *pucData);
//读取当前占用长度的函数。
uint8_t ucRingBufGetLength(psRingBufTdf pstRingBuf);
//预览数据指定位置pos的数据函数：
uint8_t ucRingBufPeek(psRingBufTdf pstRingBuf, uint32_t ulPosition);






#endif
