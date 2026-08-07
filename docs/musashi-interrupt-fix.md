# Musashi 中断处理问题分析与修复

## 背景

ESP32-Mini-Mac 使用 [Musashi](https://github.com/kstenerud/Musashi) 68000 CPU 模拟器，搭配 PCE 项目的 VIA (e6522) 和 SCC (e8530) 芯片模拟。中断处理在这两套代码的交互中存在设计上的不匹配。

## 问题：鼠标移动几次后停止响应

现象：启动正常，移动鼠标几次后鼠标卡死，系统其他功能正常。

## 根因分析

### 1. `CPU_INT_LEVEL` 的单值覆盖

Musashi 的内部变量 `CPU_INT_LEVEL` 模拟 68000 中断引脚 `IPL0-IPL2` 的状态。所有设备共享同一个值：

```c
// m68kcpu.c
CPU_INT_LEVEL = int_level << 8;
```

当多个设备先后 assert 中断时，后面的会覆盖前面的：

```
1. SCC assert level 2 → CPU_INT_LEVEL = 0x0200
2. VIA assert level 1 → CPU_INT_LEVEL = 0x0100  ← 覆盖了 level 2！
```

等级 2 的中断在 `CPU_INT_LEVEL` 中**永久丢失**。

### 2. PCE 的 IRQ 回调语义与 Musashi OPT_OFF 不匹配

PCE 的 IRQ 回调函数（`e6522_set_irq` / `e8530_set_irq`）原版逻辑是：

```c
// 原版 PCE：只在 de-assert 且已低时跳过
if (irq_val == val && val == 0)
    return;
```

即 **assert (val=1) 永远调回调**。这适用于 level-triggered 的中断控制器——`CPU_INT_LEVEL` 一旦设了就保持住，直到设备主动 de-assert。

但 ESP32-Mini-Mac 使用 **Musashi OPT_OFF** 配置，每次取完异常 `CPU_INT_LEVEL` 自动清 0。这就要求设备**每次都能重新 assert** 来把 `CPU_INT_LEVEL` 再拉高。

之前靠 `&& val == 0` 补丁（让 assert 永远走）规避了这个问题。但去掉这个补丁后（使用更自然的"skip if unchanged"），重新 assert 被跳过，中断丢失。

### 3. 定制版 `m68k_set_irq`

ESP32-Mini-Mac 的 Musashi 被定制过——`m68k_set_irq` 在设完 `CPU_INT_LEVEL` 后**立即调用 `m68ki_check_interrupts()`**：

```c
void m68k_set_irq(unsigned int int_level)
{
    CPU_INT_LEVEL = int_level << 8;
    // ...... 立即检查中断
    m68ki_check_interrupts();
}
```

这和标准 Musashi（只设 `CPU_INT_LEVEL`，不做检查）不同。立即检查中断可以降低响应延迟，但也让 `CPU_INT_LEVEL` 自动清 0 和单值覆盖的问题更容易暴露。

## 修复：虚拟中断控制器 `m68k_set_virq`

官方 Musashi 在 commit [`5d9df94`](https://github.com/kstenerud/Musashi/commit/5d9df9423392ef6bb0396cfb9750f9bb0e781c4c) 中加入了 `m68k_set_virq`，用**位掩码**来跟踪每个中断等级的状态，永远选择最高等级。

### 数据结构

```c
// m68kcpu.h：CPU 结构体中新增
typedef struct {
    // ...
    uint virq_state;    // 位掩码，每位代表一个中断等级
    // ...
} m68ki_cpu_core;
```

### 实现

```c
// m68kcpu.c
void m68k_set_virq(unsigned int level, unsigned int active)
{
    uint state = m68ki_cpu.virq_state;

    if (active)
        state |= 1 << level;      // 置位
    else
        state &= ~(1 << level);    // 清位

    m68ki_cpu.virq_state = state;

    uint blevel;
    for (blevel = 7; blevel > 0; blevel--)
        if (state & (1 << blevel))
            break;                // 从高到低找最高置位位

    m68k_set_irq(blevel);         // 设 CPU_INT_LEVEL 为最高等级
}
```

### 工作原理

```
1. SCC assert → m68k_set_virq(2, 1)
   → virq_state = 0000 0100 (bit 2)
   → highest = 2 → m68k_set_irq(2)

2. VIA assert → m68k_set_virq(1, 1)
   → virq_state = 0000 0110 (bit 2 + bit 1)
   → highest = 2（仍是 2！不会降级）
   → m68k_set_irq(2) ✓

3. SCC de-assert → m68k_set_virq(2, 0)
   → virq_state = 0000 0010 (只有 bit 1)
   → highest = 1 → m68k_set_irq(1)
```

等级之间不会互相覆盖，因为 `virq_state` 是位掩码，不是单值。

## 修复后的架构

```
[设备事件]
    │
    ├─ VIA: e6522_set_irq → mac_interrupt_via
    │                             │
    ├─ SCSI: → mac_interrupt_scsi │
    │                             ▼
    │                    m68k_set_virq(1, ...)
    │                             │
    └─ SCC: e8530_set_irq → mac_interrupt_scc
                                  │
                                  ▼
                         m68k_set_virq(2, ...)
                                  │
                                  ▼
                         m68k_set_irq(highest)
                                  │
                                  ▼
                    m68ki_check_interrupts()
                                  │
                                  ▼
                    m68ki_exception_interrupt()
```

## 涉及的配置文件

| 文件 | 改动 |
|------|------|
| `main/core/macplus/cpu/musashi/m68k.h` | 声明 `m68k_set_virq` / `m68k_get_virq` |
| `main/core/macplus/cpu/musashi/m68kcpu.h` | CPU 结构体加 `virq_state` 字段 |
| `main/core/macplus/cpu/musashi/m68kcpu.c` | 实现 `m68k_set_virq` / `m68k_get_virq`；`m68k_pulse_reset` 清零 `virq_state` |
| `main/core/macplus/core/interrupts.c` | 三个回调改用 `m68k_set_virq` |
| `main/core/macplus/devices/e6522.c` | 移除 `&& val == 0`，使用 skip-if-unchanged |
| `main/core/macplus/devices/e8530.c` | 同上 |
| `main/core/macplus/include/m68kconf.h` | `OPT_OFF` → `OPT_SPECIFY_HANDLER` + `m68k_int_ack` |

## 参考

- [Musashi commit 5d9df94: Fix m68k irq line support](https://github.com/kstenerud/Musashi/commit/5d9df9423392ef6bb0396cfb9750f9bb0e781c4c)
- PCE 项目 VIA/SCC 源码：`main/core/macplus/devices/e6522.c` / `e8530.c`
