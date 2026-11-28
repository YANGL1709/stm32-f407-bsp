/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bsp_gpio.h"
#include "bsp_key.h" 
#include "bsp_tim.h"
#include "bsp_uart.h"
#include "ring_buff.h"
#include "bsp_spi.h"
#include "w25q32.h"
#include "can.h"
#include <string.h>
#include "stdio.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
//  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */

  BspLed_Init();
  W25Q32_Init();
  Bsp_Key_Init();
  Bsp_Tim_Init();
  BspUart_Init();
   CAN_Init();
  uint8_t frame[UART_RX_BUF_SIZE];  
  extern stRingBufTdf s_rx;  

//  uint8_t mid=0;
//  uint8_t did=0;
//  uint8_t did2=0;
//  W25Q32_ReadID(&mid,&did,&did2);
//	printf("%x  ",mid);
//	printf("%x  ",did);
//	printf("%x  ",did2);
//	printf("abc\n");
	

	uint32_t rx_in = 0, rx_out1 = 0, rx_out2 = 0;
    uint8_t tx[8] = {0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88};   //data
	uint8_t tx1[8] = {0x81, 0xa2, 0x83, 0x48, 0xc5, 0x86, 0xd7, 0x88};   //data
	uint8_t tx2[6] = {0xf7, 0xb2, 0x83, 0x84, 0xa5, 0xc1};   //data
    RxMsg   rx[3];           /* FIFO 深度是 3，正好够 */
    uint8_t cnt = 0;         //报文个数
	
	uint32_t last_printf = 0;
	uint32_t last_Send = 0;
	
	volatile uint32_t g_can_rx_poll = 0;
	uint8_t  boff_last   = 0;
    uint32_t recover_ms  = 0;      /* 按下按键那一刻的 g_ms_tick */
    uint32_t recover_cnt = 0;      /* 按下按键那一刻的 TIM2->CNT */
	
	uint32_t t_last_loop = 0;
	uint32_t max_dt      = 0;

	t_last_loop = g_ms_tick;      /* ← 关键，跳过第一次 */
	max_dt      = 0;



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  
	    uint32_t t_now = g_ms_tick;
    uint32_t dt    = t_now - t_last_loop;
    if (dt > max_dt) max_dt = dt;
    t_last_loop = t_now;  
	  
	uint32_t esr_now = CAN1->ESR;
    uint8_t  boff    = (esr_now & CAN_ESR_BOFF) ? 1U : 0U;

	if (boff_last && !boff)
	{
		uint32_t pm = __get_PRIMASK();
		__disable_irq();
		uint32_t now_ms  = g_ms_tick;
		uint32_t now_cnt = TIM2->CNT;
		__set_PRIMASK(pm);

		uint32_t dt_us = (now_ms - recover_ms) * 1000U + (now_cnt - recover_cnt);
		printf("<<< BOFF cleared: %u us\r\n", (unsigned)dt_us);
	}
    boff_last = boff;
	  
		//按键1
//uint8_t e1 = Key_Check(KEY_1, KEY_SINGLE);
//uint8_t e2 = Key_Check(KEY_2, KEY_SINGLE);
//if (e1 | e2)          /* 用位或，两边都会执行 */
//    BspLed_On();

//		if (Key_Check(KEY_1, KEY_SINGLE)||Key_Check(KEY_2, KEY_SINGLE) )
//		{
//			BspLed_On();
//		}
//		if (Key_Check(KEY_1, KEY_DOUBLE) || Key_Check(KEY_2, KEY_DOUBLE))
//		{
//			BspLed_Off();
//		}
//		if (Key_Check(KEY_1, KEY_LONG) || Key_Check(KEY_2, KEY_LONG)||Key_Check(KEY_1, KEY_REPEAT) || Key_Check(KEY_2, KEY_REPEAT))
//		{
//			BspLed_Toggle();
//		}
		//按键2
    uint8_t sing = 0, dbl = 0, lng = 0;
	  
    for (uint8_t k = KEY_1; k <= KEY_2; k++)
    {
        sing |= Key_Check(k, KEY_SINGLE);
        dbl  |= Key_Check(k, KEY_DOUBLE);
        lng  |= Key_Check(k, KEY_LONG) | Key_Check(k, KEY_REPEAT);
    }

    if (sing)
	{
		BspLed_On();

		uint32_t pm = __get_PRIMASK();
		__disable_irq();
		recover_ms  = g_ms_tick;
		recover_cnt = TIM2->CNT;
		__set_PRIMASK(pm);

		CAN_RecoverFromBusOff();
	}
    if (dbl)  BspLed_Off();
    if (lng)  BspLed_Toggle();
////	  BspUart_SendByte(g_rx_byte);   /* 只打印"最后一个字节" */
//////    HAL_Delay(100);

//串口
//    if (g_frame_ready)                            /* M3 */
//    {
//        uint32_t n = g_frame_len;                 /* M5 先快照 */
//        g_frame_ready = 0;                        /* M6 尽早清标志 */

//        if (n >= sizeof(frame))                   /* M8 截断保护 */
//            n = sizeof(frame) - 1U;

//        uint32_t got = 0;
//        for (uint32_t i = 0; i < n; i++)          /* M10 */
//        {
//            uint8_t c;
//            if (ucRingBufRead(&s_rx, &c) != 0)    /* M13 0=成功 */
//                break;
//            frame[got++] = c;
//        }
//        frame[got] = 0;                           /* M18 补结束符 */

//		printf("%s\r\n", (const char *)frame);
//		
//    }


	 
//	 CAN_SendMsg(0X1718AAF1,1, tx, 8);
//	 
//	for (volatile uint32_t i = 0; i < 10000U; i++) { }  

//    CAN_ReceiveMsg(rx, &cnt);
//	
//	for (uint8_t i = 0; i < cnt; i++)
//	{
//		if (rx[i].ide)
//			printf("EXT id=%08X len=%u data=", (unsigned)rx[i].id, rx[i].len);
//		else
//			printf("STD id=%03X  len=%u data=", (unsigned)rx[i].id, rx[i].len);

//		for (uint8_t j = 0; j < rx[i].len; j++)
//			printf("%02X ", rx[i].data[j]);

//		printf("\r\n");
//	}
	
	RxMsg m;

	//① 快速消费：只计数，不打印
	while (CAN_PopRx(&m))
	{
		
	if      (m.id == 0x1718DAF1) rx_in++;
    else if (m.id == 0x17178544) rx_out1++;
    else if (m.id == 0x17184474) rx_out2++;
		g_can_rx_poll++;          //主循环取走的帧数（可和 g_can_rx 对照）
	}

	//② 每 10 秒打印一次汇总
	if (g_ms_tick - last_printf >= 1000U)
	{
		last_printf = g_ms_tick;

		uint32_t esr = CAN1->ESR;
		uint32_t tsr = CAN1->TSR;

		printf("[%us] TEC=%u REC=%u LEC=%u  %s\r\n",
		       (unsigned)(g_ms_tick / 1000U),
		       (unsigned)((esr >> 16) & 0xFFU),
		       (unsigned)((esr >> 24) & 0xFFU),
		       (unsigned)((esr >> 4)  & 0x07U),
		       
		       (esr & CAN_ESR_BOFF) ? "BUSOFF"  :
		       (esr & CAN_ESR_EPVF) ? "PASSIVE" : "ACTIVE");
		printf("    rx_in=%u rx_out1=%u rx_out2=%u  isr_rx=%u drop=%u ovr=%u max_loop=%ums\r\n",
       (unsigned)rx_in, (unsigned)rx_out1, (unsigned)rx_out2,
       (unsigned)g_can_rx, (unsigned)g_can_drop, (unsigned)g_can_ovr ,(unsigned)max_dt);
	}
	if (g_ms_tick - last_Send >= 100U)		
	{
		last_Send = g_ms_tick;
		CAN_SendMsg(0X1718AAF1,1, tx, 8);		
	
	}

	

	
}
											  



  
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PD9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
