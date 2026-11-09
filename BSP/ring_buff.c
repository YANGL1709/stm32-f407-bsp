#include "ring_buff.h"

void vRingBufInit(psRingBufTdf pstRingBuf, uint32_t u1Len,uint8_t *pucBuf)
{
	pstRingBuf->ulReadIdx=0;
	pstRingBuf->ulWriteIdx=0;
	
	pstRingBuf->ulLength= u1Len;
	
	pstRingBuf->pucBffer=pucBuf;
	for(int i=0;i<u1Len;i++)
	{
		pucBuf[i]=0;
	}
	

}         


uint8_t ucRingBufWrite (psRingBufTdf pstRingBuf, uint8_t ucValue)
//计算下一个写位置，判断满没满
{
uint32_t ulNextWrite = pstRingBuf->ulWriteIdx +1;
	
if(ulNextWrite == pstRingBuf->ulLength)
{
	ulNextWrite = 0;
}

if(ulNextWrite == pstRingBuf->ulReadIdx)
{
	return 1;//缓冲区满
}

else{
	//写入数据并更新写索引
	pstRingBuf->pucBffer[pstRingBuf->ulWriteIdx] = ucValue;
	pstRingBuf->ulWriteIdx = ulNextWrite;
	return 0;
	}

}


// 读一个字节数据
uint8_t ucRingBufRead(psRingBufTdf pstRingBuf, uint8_t *pucData)
{
    // 1. 检查缓冲区是否为空
    if (pstRingBuf->ulReadIdx == pstRingBuf->ulWriteIdx) 
    {
        return 1; // 返回 1，表示读取失败（没数据）
    } 
    else 
    {
        // 2. 读取数据（将数组中的真实数据赋值给传入的指针指向的变量）
        *pucData = pstRingBuf->pucBffer[pstRingBuf->ulReadIdx];
        
        // 3. 更新读索引（刚才读取的位置已经空了，读指针往前走一步）
        pstRingBuf->ulReadIdx += 1;
        
        // 4. 【关键修复】判断是否越界，处理绕回
        if (pstRingBuf->ulReadIdx == pstRingBuf->ulLength) 
        {
            pstRingBuf->ulReadIdx = 0; // 如果走到了数组尽头，绕回起点 0
        }
        
        return 0; // 返回 0，表示读取成功
    }
}


//读取当前占用长度的函数。
uint8_t ucRingBufGetLength(psRingBufTdf pstRingBuf)
{
    //[ (写索引-读索引 ) + 长度]  % 长度
    return (pstRingBuf->ulWriteIdx - pstRingBuf->ulReadIdx + pstRingBuf->ulLength) % pstRingBuf->ulLength;
}

//预览数据指定位置pos的数据函数：
uint8_t ucRingBufPeek(psRingBufTdf pstRingBuf, uint32_t ulPosition)
{
    //检查是否为空
    if(pstRingBuf->ulReadIdx == pstRingBuf->ulWriteIdx)
    {
        return 0xff;
    }

    //计算实际位置：(读索引+pos) % 长度
    uint32_t ulRealPos = (pstRingBuf->ulReadIdx + ulPosition) % pstRingBuf->ulLength;

    //返回这个位置的数据
    return pstRingBuf->pucBffer[ulRealPos];
}
