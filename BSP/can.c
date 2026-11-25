#include "can.h"
#include "bsp_uart.h"
							  //SJW          TS2        TS1         BRP
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
	CAN1->BTR = CAN_BTR_500K_42M;
	
		/*  退出初始化 */
	CAN1->MCR &= ~CAN_MCR_INRQ;
	 while (CAN1->MSR & CAN_MSR_INAK) { }
	
	 //过滤器初始化模式
	 CAN1->FMR |=CAN_FMR_FINIT;
	 
	 CAN1->FM1R &=~CAN_FM1R_FBM0;   //过滤器0 32 位寄存器处于标识符屏蔽模式。
	 
	 CAN1->FS1R &=~CAN_FS1R_FSC0;  // 32 位尺度配置
	 
	 CAN1->FFA1R &= ~CAN_FFA1R_FFA0;    //关联fifo0
	 
	 CAN1->sFilterRegister[0].FR1=0;
	 CAN1->sFilterRegister[0].FR2=0;
	 
	 CAN1->FA1R |=CAN_FA1R_FACT0;  // 过滤器 0 激活
	 
	 CAN1->FMR &=~CAN_FMR_FINIT;   //    过滤器工作模式  
	 
	 
	 	//中断接收
	CAN1->IER |= CAN_IER_FMPIE0;
	NVIC_SetPriority(CAN1_RX0_IRQn, 3);   //比 USART1(4)/TIM2(5) 高
	NVIC_EnableIRQ(CAN1_RX0_IRQn);
	 
	 
}
	   
void CAN_SendMsg(uint32_t id, uint8_t ide, uint8_t *data, uint8_t len)
{
    uint32_t timeout, tir;

    /* ---- 参数校验 ---- */
    if (len > 8U) return;
    if (ide) { if (id > 0x1FFFFFFFU) return; }   /* 29 位最大 */
    else     { if (id > 0x7FFU)      return; }   /* 11 位最大 */

    /* ---- 等邮箱空 ---- */
    while((CAN1->TSR & CAN_TSR_TME0) ==0)
	{}

    /* ---- 拼 TIR ---- */
    /* TIR 布局：STID[31:21] | EXID[20:3] | IDE[2] | RTR[1] | TXRQ[0] */
    if (ide)
    {
        uint32_t stid = (id >> 18) & 0x7FFU;    /* ID[28:18] 11 位 */
        uint32_t exid =  id        & 0x3FFFFU;  /* ID[17:0]  18 位 */
        tir = (stid << 21) | (exid << 3) | CAN_TI0R_IDE;
    }
    else
    {
        tir = (id & 0x7FFU) << 21;
    }

    CAN1->sTxMailBox[0].TIR  = tir;                 /* RTR=0，TXRQ=0 */
    CAN1->sTxMailBox[0].TDTR = len;
    CAN1->sTxMailBox[0].TDLR = 0;
    CAN1->sTxMailBox[0].TDHR = 0;

    for (uint8_t i = 0; i < len; i++)
    {
        if (i < 4U)
            CAN1->sTxMailBox[0].TDLR |= (uint32_t)data[i] << (i * 8U);
        else
            CAN1->sTxMailBox[0].TDHR |= (uint32_t)data[i] << ((i - 4U) * 8U);
    }

    /* ---- 请求发送 + 等完成 ---- */
    CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;

    timeout = 1000000U;
    while (!(CAN1->TSR & CAN_TSR_TME0) && --timeout) { }
}

void CAN_ReceiveMsg(RxMsg rxMsg[], uint8_t *msgCount)
{
	*msgCount = (CAN1->RF0R & CAN_RF0R_FMP0) >> 0;    //获取报文个数

	for (uint8_t i = 0; i < *msgCount; i++)    //循环每一个报文
	{
		RxMsg *msg = &rxMsg[i];   //定义指针

		uint32_t rir = CAN1->sFIFOMailBox[0].RIR;

		if (rir & CAN_RI0R_IDE)                   //扩展帧
		{
			msg->ide   = 1U;
			msg->id = ((rir >> 3) & 0x3FFFFU)               //EXID 低 18 位
			           | (((rir >> 21) & 0x7FFU) << 18);       //STID 高 11 位
		}
		else                                      //标准帧
		{
			msg->ide   = 0U;
			msg->id = (rir >> 21) & 0x7FFU;    //ID
		}

		msg->len = (CAN1->sFIFOMailBox[0].RDTR >> 0) & 0x0F;   //DLC

		uint32_t low  = CAN1->sFIFOMailBox[0].RDLR;            //低位数据
		uint32_t high = CAN1->sFIFOMailBox[0].RDHR;            //高位数据

		for (uint8_t j = 0; j < msg->len; j++)
		{
			if (j < 4)
			{
				msg->data[j] = (low >> (8 * j)) & 0xFF;
			}
			else
			{
				msg->data[j] = (high >> (8 * (j - 4))) & 0xFF;
			}
		}

		CAN1->RF0R |= CAN_RF0R_RFOM0;      //释放邮箱
	}
}

/* ================ 中断接收 ================ */

#define CAN_RXQ_DEPTH   16U      //队列深度：8h 满载测试需要 16

static RxMsg            s_canq[CAN_RXQ_DEPTH];
static volatile uint8_t s_qhead = 0;      //ISR 写
static volatile uint8_t s_qtail = 0;      //主循环写

volatile uint32_t g_can_rx   = 0;         //收到总帧数
volatile uint32_t g_can_drop = 0;         //队列满丢掉的帧数
volatile uint32_t g_can_ovr  = 0;

void CAN1_RX0_IRQHandler(void)
{
	RxMsg   tmp[3];                       //FIFO0 深度是 3，栈上临时区
	uint8_t n = 0;

	CAN_ReceiveMsg(tmp, &n);              //★ 复用：读空 FIFO + 解析

	for (uint8_t i = 0; i < n; i++)
	{
		uint8_t next = (uint8_t)((s_qhead + 1U) % CAN_RXQ_DEPTH);

		if (next != s_qtail)              //队列没满
		{
			s_canq[s_qhead] = tmp[i];     //结构体赋值，纯拷贝
			s_qhead = next;
			g_can_rx++;
		}
		else                              //队列满，丢帧
		{
			g_can_drop++;
		}
	}
	
	if (CAN1->RF0R & CAN_RF0R_FOVR0)
	{
		g_can_ovr++;
		CAN1->RF0R |= CAN_RF0R_FOVR0;    //写 1 清除
	}
}

uint8_t CAN_PopRx(RxMsg *msg)
{
	if (s_qtail == s_qhead) return 0;     //空

	*msg = s_canq[s_qtail];               //纯拷贝
	s_qtail = (uint8_t)((s_qtail + 1U) % CAN_RXQ_DEPTH);
	return 1;
}

