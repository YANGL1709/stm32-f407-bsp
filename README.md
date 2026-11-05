# STM32F407 寄存器级 BSP

不依赖 HAL 的 STM32F407 板级驱动库：直接用寄存器操作外设，每个外设一个 `bsp_xxx.c/h`，为后续 CAN / UDS 汽车电子项目打地基。

## 为什么不用 HAL

`HAL_CAN_Init()` 会帮你算好位定时，你永远不知道 `BRP / TS1 / TS2` 是什么；`HAL_UART_Receive_IT()` 会帮你屏蔽掉中断标志位的清除顺序。

这个项目的目标是把这些全部自己写一遍：

- 每一个外设都从 `RCC->AHB1ENR` 开时钟开始
- 每一次写寄存器都对照 RM0090 的位定义
- 每一层都要能在示波器 / 逻辑分析仪上立刻验证

**代价**：比抄例程慢 3 倍。**收益**：CAN 位定时、Bus-Off 恢复、Flash 擦写这些"只能靠寄存器"的东西，不再是黑盒。

## 硬件

| 项 | 型号 / 引脚 |
|---|---|
| MCU | STM32F407ZGT6（144 脚，1MB Flash） |
| 调试器 | CMSIS-DAP，SWD 模式，`Connect: under Reset`，`1MHz` |
| LED | PC13（低电平点亮） |
| 按键 KEY1 | PA15（内部上拉，按下为低） |
| 按键 KEY2 | PE6（内部上拉，按下为低） |

## 目录结构

```
BSP/            板级驱动，本项目自己写的部分
  bsp_gpio.c/h    LED：寄存器驱动，BSRR 原子写，极性可配
  bsp_tim.c/h     TIM2：1ms 时基中断，g_ms_tick 计数
  bsp_key.c/h     按键：10ms 采样状态机，单击 / 双击 / 长按 / 连发
Core/           CubeMX 生成的工程骨架，主循环在 Core/Src/main.c
Drivers/        ST 官方 HAL + CMSIS（未修改）
MDK-ARM/        Keil 工程
key.ioc         CubeMX 配置
```

## 已完成

| 模块 | 状态 | 说明 |
|---|---|---|
| GPIO 输出 | ✅ | 寄存器直写 MODER / OTYPER / OSPEEDR / PUPDR，用 `BSRR` 保证原子性 |
| GPIO 输入 | ✅ | 内部上拉 + 软件消抖 |
| TIM 时基 | ✅ | TIM2 1ms 更新中断，`g_ms_tick` 提供全局毫秒计数 |
| 按键 | ✅ | 20ms 采样状态机，支持单击 / 双击 / 长按 / 长按连发 |
| UART | ⏳ | 轮询 → 中断 → 环形缓冲 |
| SPI / I2C | ⏳ | |
| CAN | ⏳ | 位定时 / 过滤器 / Bus-Off 恢复 |

## 编译与烧录

1. Keil MDK-ARM 打开 `MDK-ARM/led.uvprojx`
2. `Rebuild`，应输出 `0 Error(s), 0 Warning(s)`
3. CMSIS-DAP 下载（`Connect: under Reset`，`Max Clock 1MHz`）

## 设计要点

### 1. 位操作一律用 `BSRR`，不用 `ODR`

```c
LED_PORT->BSRR = (1U << (LED_PIN + 16));   /* 拉低：写高 16 位 */
LED_PORT->BSRR = (1U << LED_PIN);          /* 拉高：写低 16 位 */
```

`ODR |= (1 << n)` 是读-改-写三步，被中断打断会丢状态。`BSRR` 是单次写，硬件保证原子。

### 2. `Init()` 里显式设置初值，不依赖复位默认值

```c
void BspLed_Init(void)
{
    ...
    LED_PORT->BSRR = (1U << LED_PIN);   /* 显式关灯 */
}
```

**规律**：凡是"第一次不对、后面都对"，第一个怀疑对象就是初值。

### 3. 中断与主循环共享的标志，`volatile` 和临界区缺一不可

```c
static volatile uint8_t Key_Flag[KEY_COUNT];   /* volatile：禁止编译器缓存 */
```

```c
pm = __get_PRIMASK();
__disable_irq();                                /* 临界区：保证读-改-写不被打断 */
hit = (Key_Flag[n] & Flag) ? 1 : 0;
if (hit && Flag != KEY_HOLD) Key_Flag[n] &= (uint8_t)~Flag;
__set_PRIMASK(pm);
```

两者解决的是不同问题：

| 手段 | 解决 | 不解决 |
|---|---|---|
| `volatile` | 编译器优化（缓存进寄存器、删掉读写） | 原子性，`x++` 照样被打断 |
| 临界区 | 原子性（三步不被打断） | 可见性 |

用 `__get_PRIMASK()` / `__set_PRIMASK()` 保存恢复，而不是 `__enable_irq()`，是为了支持嵌套临界区——将来 Bootloader 里擦写 Flash 时，这段时间必须严格关中断。

### 4. 按键：时间用 1ms 单位，状态机用 20ms 节拍

- `Time[]` 每 1ms 递减，所以 `KEY_TIME_LONG = 2000` 就是 2 秒
- 状态机每 20ms 采样一次，避开机械抖动
- 事件通过位标志传出，`Key_Check()` 取用后自动清零，主循环不需要自己管状态

## 踩过的坑

| 现象 | 根因 | 记住什么 |
|---|---|---|
| `MODER &= ~(11 << 26)` 行为异常 | `11` 是十进制 11（`1011`），不是二进制 | 用 `3` 或 `0b11`，**别写 `11`** |
| `OTYPER &= 0 << 13` 清空全寄存器 | `0 << 13` 等于 0 | 清一位用 `&= ~(1U << n)` |
| LED 第一次翻转"没反应" | 逻辑状态与上电实际电平不一致 | `Init()` 里显式关灯 |
| 按键按住时程序卡死 | `while` 等松手 = 阻塞 | 状态机 + TIM 中断 |
| 偶尔按一下没反应 | 标志位读-改-写被中断打断 | 临界区 |

## 进度规划

```
W1  GPIO + TIM 时基          ✅
W2  非阻塞按键状态机          ✅
W3  UART（环形缓冲 / 超时断帧）
W4  SPI / I2C
W5  CAN 位定时 + 收发 + 过滤器
W6  CAN 中断 / TEC-REC / Bus-Off 恢复
W7  FreeRTOS 核心 + miniRTOS
W8  队列 / 信号量 / 互斥量
→   ISO-TP → UDS → Bootloader → 双 CAN 网关
```

最终目标：**双 CAN 网关 + UDS 诊断刷写系统**。
