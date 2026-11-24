#include "can.h"

#define CAN_BTR_500K_42M   ((0U << 24) | (1U << 20) | (10U << 16) | 5U)


void CAN_Init(void)
{
	//开时钟
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
	
	/* 2. PB8 = CAN1_RX，PB9 = CAN1_TX，都是 AF9 */
	GPIOB->MODER &=~((3U<<16)|(3U<<18));
	GPIOB->MODER |= ((2U<<16)|(2U<<18));
	
	GPIOB->AFR[1] &=~((0xFU<<0)|(0xFU<<4));
	GPIOB->AFR[1] |= ((9U<<0)|(9U<<4));
	
	GPIOB->OSPEEDR |= ((1U << 16) | (1U << 18));      /* 中速 */
	
	/* ① 先唤醒（如果之前在睡眠） */
	CAN1->MCR &= ~CAN_MCR_SLEEP;
	while (CAN1->MSR & CAN_MSR_SLAK) { }      /* 等 SLAK 清零 = 已唤醒 */

	/* ② 再进初始化 */
	CAN1->MCR |= CAN_MCR_INRQ;
	while (!(CAN1->MSR & CAN_MSR_INAK)) { }   /* 等 INAK 置位 = 进了初始化 */
		
	CAN1->MCR |= CAN_MCR_ABOM;   /* 自动离线恢复，调试期先开着 */
	CAN1->BTR = CAN_BTR_500K_42M|CAN_BTR_SILM|CAN_BTR_LBKM;
	
		/*  退出初始化 */
	CAN1->MCR &= ~CAN_MCR_INRQ;
	 while (CAN1->MSR & CAN_MSR_INAK) { }
	
	 //过滤器初始化
	 CAN1->FMR |=CAN_FMR_FINIT;
	 
	 CAN1->FM1R &=~CAN_FM1R_FBM0;
	 
	 CAN1->FS1R &=~CAN_FS1R_FSC0;
	 
	 CAN1->FFA1R &= ~CAN_FFA1R_FFA0;
	 
	 CAN1->sFilterRegister[0].FR1=0;
	 CAN1->sFilterRegister[0].FR2=0;
	 
	 CAN1->FA1R |=CAN_FA1R_FACT0;
	 
	 CAN1->FMR &=~CAN_FMR_FINIT; 
	 
	 
}

void CAN_SendMsg(uint32_t stdID,uint8_t *data,uint8_t len)
{    //等待发送邮箱空
	while((CAN1->TSR & CAN_TSR_TME0) ==0)
	{}
	 //设置标准ID
	CAN1->sTxMailBox[0].TIR &=~CAN_TI0R_STID;
	CAN1->sTxMailBox[0].TIR |=  (stdID<<21U);
		
	CAN1->sTxMailBox[0].TIR &=~CAN_TI0R_IDE;
	CAN1->sTxMailBox[0].TIR &=~CAN_TI0R_RTR;
		
	CAN1->sTxMailBox[0].TDTR &=~CAN_TDT0R_DLC;
	CAN1->sTxMailBox[0].TDTR |=	 (len<<0);
		
	CAN1->sTxMailBox[0].TDLR =0;
	CAN1->sTxMailBox[0].TDHR =0;

	for(uint16_t i=0;i<len;i++)
	{
		if(i<4)	
		{
			CAN1->sTxMailBox[0].TDLR |= data[i]<<(i*8);
		}
			
		else	
		{
			CAN1->sTxMailBox[0].TDHR |= data[i]<<((i-4)*8);
		}
			
	}
	
	
	CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;
	
	while((CAN1->TSR & CAN_TSR_TME0)==0)       //TXOK0 是粘滞位，没清 → 第二次开始不等
	{}

}

void CAN_ReceiveMsg(RxMsg rxMsg[],uint8_t * msgCount)
{
	 * msgCount =(CAN1->RF0R &CAN_RF0R_FMP0) >>0;  //获取报文个数
	
	for (uint8_t i=0;i<* msgCount;i++)    //循环每一个报文
	{
		RxMsg * msg =&rxMsg [i];   //定义指针
		
		msg->stdID =(CAN1->sFIFOMailBox[0].RIR >>21) & 0x7ff;
		
		msg->len   =(CAN1->sFIFOMailBox[0].RDTR>>0)  & 0x0f;
		
		uint32_t low =CAN1->sFIFOMailBox[0].RDLR;
		uint32_t high = CAN1->sFIFOMailBox[0].RDHR;
		
		for (uint8_t j=0;j<msg->len;j++)
		{
			if(j<4)
			{
				msg->data[j]=(low>>(8 * j)) &0xff;
				
			}
			else
			{
				msg->data[j] = (high>>(8 * (j-4)))  & 0xff;
			}
		}
		
		
			 CAN1->RF0R |= CAN_RF0R_RFOM0;      //释放邮箱
	}

	
	
}



