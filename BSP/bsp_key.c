#include "main.h"       
#include "bsp_key.h"

#define KEY_PRESSED				1   //按键按下
#define KEY_UNPRESSED			0

#define KEY_TIME_DOUBLE			200   //双击时长
#define KEY_TIME_LONG			2000  //长按时长
#define KEY_TIME_REPEAT			100   //重复长按时长

static volatile uint8_t Key_Flag[KEY_COUNT];


void Bsp_Key_Init(void)
{
	RCC->AHB1ENR|=(1U<<0);  //GPIOA时钟
	
	KEY_PORT->MODER&=~(3U<<2*KEY_PIN);
	
	KEY_PORT->PUPDR&=~(3U<<2*KEY_PIN);
	KEY_PORT->PUPDR|=(1<<(2*KEY_PIN));
	
	RCC->AHB1ENR|=(1U<<4);  //GPIOE时钟
	
	KEY1_PORT->MODER&=~(3U<<2*KEY1_PIN);
	
	KEY1_PORT->PUPDR&=~(3U<<2*KEY1_PIN);
	KEY1_PORT->PUPDR|=(1<<(2*KEY1_PIN));
	
	
	
}

uint8_t Key_Check(uint8_t n, uint8_t Flag)
{
    uint32_t pm;
    uint8_t  hit;

    if (n >= KEY_COUNT) return 0;        /* 边界保护 */

    if (!(Key_Flag[n] & Flag)) return 0; /* ★ 快路径：没事件，不关中断 */

    pm = __get_PRIMASK();
    __disable_irq();
    hit = (Key_Flag[n] & Flag) ? 1 : 0;  /* 慢路径里再确认一次 */
    if (hit && Flag != KEY_HOLD)
        Key_Flag[n] &= (uint8_t)~Flag;
    __set_PRIMASK(pm);

    return hit;
}

uint8_t Key_GetState(uint8_t Key)//判断按键按下
{
	if (Key == KEY_1)
	{
		if ((KEY_PORT->IDR&1<<KEY_PIN) == 0)
		{
			return KEY_PRESSED;
		}
	}
	else if (Key == KEY_2)
	{
		if ((KEY1_PORT->IDR&1<<KEY1_PIN) == 0)
		{
			return KEY_PRESSED;
		}
	}
	return KEY_UNPRESSED;
}

void Key_Tick(void)//按键中断函数
{
	static uint8_t Count,i;
	static uint8_t CurrState[KEY_COUNT], PrevState[KEY_COUNT];//上次按键和本次按键状态
	static uint8_t Mode[KEY_COUNT];//状态机模式
	static uint16_t Time[KEY_COUNT];//模式判断时间
	
	
	for (i = 0; i < KEY_COUNT; i ++)//遍历所有按键的时间，单片机运行此代码的时间约为1us，
										//远小于中断的1ms，在下次中断来前遍历完时间就不会出错
	{
		if (Time[i] > 0)
		{
			Time[i] --;
		}
	}
	
	Count ++;
	if (Count >= 20)
	{
		Count = 0;
		
		for (i = 0; i < KEY_COUNT; i ++)
		{
		  PrevState[i] = CurrState[i];      //赋值给上次按键
		CurrState[i] = Key_GetState(i);    //获取本次按键状态
		
		if (CurrState[i] == KEY_PRESSED)     //按键按住不放
			{
				Key_Flag[i]|= KEY_HOLD;
			}
			else
			{
				Key_Flag[i] &= ~KEY_HOLD;
			}
													//下降沿
			if (CurrState[i] == KEY_PRESSED && PrevState[i] == KEY_UNPRESSED)
			{                                       //本次按键按下，上次按键无按下
				Key_Flag[i] |= KEY_DOWN;
			}
												  //上升沿
			if (CurrState[i] == KEY_UNPRESSED && PrevState[i] == KEY_PRESSED)
			{
				Key_Flag[i] |= KEY_UP;
			}
			
		switch(Mode[i])
		{
			case 0:   //空闲
					if (CurrState[i] == KEY_PRESSED)
					{
						Time[i] = KEY_TIME_LONG;
						Mode[i] = 1;
					}
				
				break;			
			
			case 1:   //按键已按下
					if (CurrState[i] == KEY_UNPRESSED)
					{
						Time[i]=KEY_TIME_DOUBLE;
						Mode[i] = 2;
					}
					else if(Time[i]==0)
					{
						Time[i]=KEY_TIME_REPEAT;
						Key_Flag[i] |= KEY_LONG;
						Mode[i] = 4;
					}
				
				break;
			
			case 2:    //按键已松开
					if (CurrState[i] == KEY_PRESSED)
					{
						Key_Flag[i] |= KEY_DOUBLE;
						Mode[i] = 3;
						
					}
					else if(Time[i]==0)
					{
						Key_Flag[i]|= KEY_SINGLE;
						Mode[i] = 0;
					}
				
				
				break;
			
			case 3:     //按键以双击
					if (CurrState[i] == KEY_UNPRESSED)
					{
					
						Mode[i] = 0;
					}				
				break;
			
			case 4:     //按键以长按
					if (CurrState[i] == KEY_UNPRESSED)
					{
					
						Mode[i] = 0;
					}
					else if(Time[i]==0)
					{
						Time[i] = KEY_TIME_REPEAT;
						Key_Flag[i]|= KEY_REPEAT;
						Mode[i] = 4;
					}					
				break;
								
		}
			
		
		}
		
		
			
	}
}