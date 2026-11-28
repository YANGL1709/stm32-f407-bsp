## 实验五：过滤器掩码验证（2026.11.28）


# W06 CAN 错误状态与 Bus-Off（2026.11.27）

## 一、结论

| 实验 | 现象 | 关键寄存器 | 结论 |
|---|---|---|---|
| 单节点无应答 | TEC 卡 128，无限重传 | `ESR=0x00800033`，LEC=3 | ACK 错误在 error-passive 后被豁免，**到不了 Bus-Off** |
| CAN_H 短地 | TEC 冲到 248 → BOFF | `ESR=0x00F80057`，LEC=5 | 位显性错误不受豁免，**能到 Bus-Off** |
| ABOM=0 | BOFF 停 3 秒不自愈 | BOFF 位恒 1 | 硬件不数隐性位，**等软件下令** |
| INRQ 1→0 恢复 | 2841 μs 后 BOFF 清零 | 理论 2816 μs | 误差 0.9%，**128×11 理论被实测验证** |

## 二、环境

- MCU：STM32F407ZGT6，168MHz，CAN1 在 PB8/PB9（AF9）
- 收发器：TJA1050
- 位定时：`BTR=0x001A0005`（BRP=5→÷6，TS1=10，TS2=1，14 tq，采样点 85.7%），500 kbps
- ABOM=0，NART=0（自动重传）
- 采样：主循环轮询，1 s 打印一次

## 三、实验 1：单节点无应答

```
[10s] TEC=128 REC=0 LEC=3 ESR=00800033 TSR=19000008 PASSIVE
[11s] TEC=128 REC=0 LEC=3 ESR=00800033 TSR=19000008 PASSIVE
```

`ESR=0x00800033` 解码：

```
31:24 REC=0x00
23:16 TEC=0x80 = 128
 6:4  LEC=3  → ACK 错误
   2  BOFF=0
   1  EPVF=1 → 错误被动
   0  EWGF=1
```

**分析**：总线上没有第二个节点，发送方在 ACK 槽读不到显性 → ACK 错误 → TEC += 8。
8×16 = 128 后节点进入 error-passive，之后再遇 ACK 错误**不再累加**（ISO 11898 规则 3 例外 1）。
所以 TEC 永远到不了 256，**单节点不可能产生 Bus-Off**。

`TSR=0x19000008`：`TME0=0`（邮箱 0 一直被占着）+ `TERR0=1`（发送失败）→ 说明在无限重传。

## 四、实验 2：CAN_H 短接到地

```
[16s] TEC=248 REC=0 LEC=5 ESR=00F80057 TSR=19000008 BUSOFF
[17s] TEC=248 REC=0 LEC=5 ESR=00F80057 TSR=19000008 BUSOFF
```

`0x00F80057`：TEC=0xF8=248，LEC=5（**位显性错误**），BOFF=1。

**分析**：短接把总线钉成常显性。节点发**隐性位**时回读成显性 → 位显性错误。
位错误不在豁免清单里 → error-passive 后 TEC 照样 +8 → 冲破 255 → BOFF。

248 = 128 + 15×8。第 16 次 +8 会到 256 > 255，触发 Bus-Off 的这一次不再计入，所以计数器冻结在 248。

> 注意：BOFF 后 `ESR.TEC` 只是 9 位计数器的低 8 位，**判 Bus-Off 只能看 BOFF 位**。

## 五、实验 3：ABOM=0 不会自愈

BOFF 位连续 3 秒保持 1，总线早已恢复空闲也不动。

**原因**：ABOM=0 时硬件**根本不启动** 128×11 的计数（不是“数了不生效”）。

## 六、实验 4：恢复与计时

```c
void CAN_RecoverFromBusOff(void)
{
	CAN1->MCR |= CAN_MCR_INRQ;
	while (!(CAN1->MSR & CAN_MSR_INAK)) { }
	CAN1->MCR &= ~CAN_MCR_INRQ;
	while (CAN1->MSR & CAN_MSR_INAK) { }
}
```

```
<<< BOFF cleared: 2841 us
[18s] TEC=128 REC=0 LEC=3 ESR=00800033 TSR=19000008 PASSIVE
```

时间戳用 `g_ms_tick`（1ms）+ `TIM2->CNT`（1μs）拼成，读取时用临界区保证原子：

```c
uint32_t pm = __get_PRIMASK();
__disable_irq();
uint32_t ms = g_ms_tick, cnt = TIM2->CNT;
__set_PRIMASK(pm);

uint32_t dt_us = (ms - recover_ms) * 1000U + (cnt - recover_cnt);
```

恢复后 TEC 归零，随即重新发帧、又没人 ACK，4ms 内爬回 128 —— 这恰好证明恢复是真的成功。

## 七、踩过的坑

| 坑 | 教训 |
|---|---|
| `ESR` 的 TEC/REC 位序写反 | 查 CMSIS 头文件 `CAN_ESR_TEC_Pos`，别凭记忆 |
| LEC 编码记错（以为 4=位错、5=CRC） | bxCAN 把位错误拆成 4=位隐性、5=位显性，6 才是 CRC |
| 以为短接 CAN_H 会卡 128 | 短接产生的是**位错误**，不是 ACK 错误 |
| 1 秒采样看不到 128→248 | 整个过程几毫秒，采样必须 ≤10ms |
| BOFF 后 TEC 显示 0 以为没进去 | 看 BOFF 位 |
| `recover_req` / `recover_ms` 变量名不一致 | 两版代码拼在一起导致的编译错 |

## 八、面试问答

**Q：Bus-Off 怎么产生？**
A：TEC 超过 255。产生的前提是“不可被豁免的错误”——ACK 错误在 error-passive 后会被豁免，所以拔线只能到 128；而位错误/填充错/格式错不受豁免，能冲到 255。

**Q：Bus-Off 怎么恢复？**
A：硬件要检测到 128 次 × 11 个连续隐性位（500k 下 2816 μs），实测 2841 μs。ABOM=1 时硬件自动做；ABOM=0 时软件必须写 `INRQ 1→0` 重新武装这个计数。

**Q：为什么 TEC 停在 128？**
A：ISO 11898 规则 3 的例外——error-passive 的发送节点检测到 ACK 错误时不再累加 TEC。这是故障隔离设计：坏节点应该安静地停在 error-passive，而不是升级到 Bus-Off。

### 配置

```c
CAN1->FMR  |=  CAN_FMR_FINIT;      /* 进入过滤器初始化窗口 */
CAN1->FM1R &= ~CAN_FM1R_FBM0;      /* 0 = 掩码模式（1 = 列表） */
CAN1->FS1R |=  CAN_FS1R_FSC0;      /* 1 = 32 位尺度（0 = 16 位） */
CAN1->FFA1R &= ~CAN_FFA1R_FFA0;    /* 关联 FIFO0 */

CAN1->sFilterRegister[0].FR1 = (0x1718AAF1U << 3) | 0x04;   /* 0xB8C5578C */
CAN1->sFilterRegister[0].FR2 =  0xFFF80000U      | 0x04;    /* 0xFFF80004 */

CAN1->FA1R |=  CAN_FA1R_FACT0;     /* 激活过滤器 0 */
CAN1->FMR  &= ~CAN_FMR_FINIT;      /* 退出，过滤器开始工作 */
```

**掩码含义**（32 位尺度位布局）：

| 寄存器位 | 对应 ID 位 | 说明 |
|---|---|---|
| 31:21 | ID[28:18] 基本 ID | 必须匹配 |
| 20:19 | ID[17:16] 扩展 ID 高 2 位 | 必须匹配 |
| 18:3  | **ID[15:0] 扩展 ID 低 16 位** | **不关心** |
| 2 | IDE | 必须 = 1（只收扩展帧） |
| 1 | RTR | 不关心 |

→ 放行范围 **`0x17180000 ~ 0x1718FFFF`**（65536 个 ID）

### 激励

PCAN-View（normal 模式，会自动 ACK）周期发送三个 ID：

| ID | 周期 | 速率 | 是否在段内 |
|---|---|---|---|
| `0x1718DAF1` | 1 ms | 1000 /s | ✅ |
| `0x17184474` | 1 ms | 1000 /s | ✅ |
| `0x17178544` | 2 ms | 500 /s | ❌（`0x1717`） |

### 结果

```
[1s] TEC=0 REC=0 LEC=0 ESR=00000000 TSR=1C000003 ACTIVE
     rx_in=1194  rx_out1=0  rx_out2=1195  isr_rx=2399  drop=0  ovr=0
[3s] TEC=0 REC=0 LEC=0 ESR=00000000 TSR=1C000003 ACTIVE
     rx_in=3189  rx_out1=0  rx_out2=3189  isr_rx=6388  drop=11 ovr=0
[7s] TEC=0 REC=0 LEC=0 ESR=00000000 TSR=1C000003 ACTIVE
     rx_in=7178  rx_out1=0  rx_out2=7175  isr_rx=14364 drop=35 ovr=0
```

- 两个段内 ID 各 **+1000/s**
- 段外 ID **恒为 0**
- `isr_rx = 2000/s`，正好等于两个段内 ID 之和

**结论：掩码精确生效，段外一帧未进。**

### 踩的坑

**坑 1：计数器声明在 `while(1)` 里面 → 每轮被清零**

```c
while (1) {
    uint32_t rx_in = 0, rx_out1 = 0, rx_out2 = 0;   /* ← 错：每圈重置 */
    while (CAN_PopRx(&m)) { if (m.id == ...) rx_in++; }
    printf("rx_in=%u", rx_in);   /* 永远是 0 */
}
```

现象是“`isr_rx` 在涨、`rx_in` 恒 0”，很容易误判成“过滤器把该收的也挡了”。
**教训：凡是要跨循环累计的量，声明必须在循环外。**

**坑 2：变量名和实际含义脱节**

`rx_out2` 后来装的其实是段内的 `0x17184474`，看名字会误以为“段外也在收”。命名要跟着含义走。

**坑 3：`printf` 会吃掉接收带宽**

`drop` 以约 **6 帧/秒** 增长：

```
每行 printf ≈ 1~2 ms 阻塞（115200，60 字符）
每秒 2 行 → 主循环停 3~4 ms
2000 fps × 0.004 s ≈ 8 帧 → 16 深队列扛不住
```

产品里日志要**环形缓冲 + DMA 发送**，主循环只入队不阻塞。

**坑 4：`FMR` 读出异常值**

```
FMR = 0x2A1C0E00
```

按手册只有 `CAN2SB[13:8]` 和 `FINIT[0]` 有效，高 18 位是保留位（应为 0）。功能位正确（`CAN2SB=14`、`FINIT=0`），但**疑似兼容片**（GD32 / APM32 / CKS32 之类）。以后遇到“寄存器行为和手册不符”，先怀疑这个。

### 面试点

- **掩码 vs 列表**：掩码适合“一段连续 ID”，列表适合“几个无关 ID”；32 位列表一个 bank 放 2 个，16 位列表放 4 个
- **怎么只收 0x100–0x1FF**：`FR1 = 0x100<<21`、`FR2 = 0x700<<21`，只看高 3 位
- **IDE 必须进掩码**：否则标准帧只要 STID 碰巧相同也会被收进来