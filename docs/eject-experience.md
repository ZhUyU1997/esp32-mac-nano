# 网页 eject 软盘——探索经验总结

> 目标：ESP32 Mac Plus 模拟器上，通过网页远程弹出软盘，且"软件可感知"
> （与 Finder 的 File > Eject 等效）。本文记录探索过程中的所有经验教训：
> 已确认的事实、踩过的坑、死路、可行方向。

---

## 一、已确认的 API 事实（Multiversal / Retro68）

### eject 正确调用序列（与 File > Eject 等效）

```c
FlushVol(0, vRefNum);   /* 缓存写回（卷号语义）*/
/* 不显式 UnmountVol —— System 6 的 FSM Eject 内部处理卸载，
 * 显式卸载会导致盘符消失（与 File > Eject 行为不同）*/
Eject(0, 1);            /* 驱动器号语义：1 = 内置软驱 */
```

- **`Eject` 的参数是驱动器号（drvNum）**，不是卷 vRefNum——传卷号（如 -2）会在 FSM 层返回 `nsDrvErr`（-56），且不触发 SONY 驱动（无日志）
- **`UnmountVol` 用卷号**（vRefNum）——与 Eject 的驱动器号语义不同
- `fBsyErr`（-47）= 卷忙（打开文件/默认卷占用）；`nsDrvErr`（-56）；`paramErr`（-50）

### 卷遍历（HFS + MFS 双遍历）

```c
/* HFS 卷：PBHGetVInfoSync（trap 0xA207）—— HFS-only */
/* MFS 卷：PBGetVInfoSync（trap 0xA007）—— MFS-only */
/* 两者可能返回同一卷 → 需去重（vRefNum 或卷名）*/
```

- 单一 `PBHGetVInfo` 看不到 MFS 卷，反之亦然
- `GetVInfo(drv)` 的 drv 参数在 executor 实现里被映射成 `ioVRefNum`——SCSI 启动时指向启动盘（坑）
- `PBHGetVolParms`（trap 0xA260 = FSDispatch selector 0x30）**在 System 6 上全部返回 paramErr**——该 selector 不在 System 6 的 FSM 中
- `ioVAtrb` = HFS 卷头属性（`vcbAtrb`），**没有 ejectable 位**——不能用于识别软盘
- System 6 的卷 vRefNum 可能是**负数**（可移动卷）——vRefNum 1..8 遍历会漏

### MFS vs HFS 关键差异

- **MFS 的 `UnmountVol` 在 System 6 返回 fBsyErr**（格式相关，与文件/容量无关——5 种测试盘矩阵证实）
- MFS 盘 eject 只能走 `FlushVol + Eject`（不卸载），HFS 盘两种都行
- `mfs.js` 可生成 400K/800K MFS（`sizeKB` 参数动态计算）
- MFS 无目录层级、文件名 ≤31 字符

### SHM 机制（host ↔ guest 通信）

- `sim->shm_region[512]` 映射 guest 地址 `0xF00000`（512B），host 直接写数组、guest 内存读写即见——**双向，零新机制**
- 消息队列 `mac_msg_submit` → 模拟器线程 dispatch → 写 SHM：跨线程安全的标准路径

---

## 二、Retro68 INIT（系统扩展）经验

### 构建（参考 `Samples/SystemExtension`）

- `add_executable` + `LINK_FLAGS -Wl,--mac-flat`（扁平代码）
- Rez 打包：`type 'INIT' { RETRO68_CODE_TYPE }` + `resource 'INIT' (128, locked) { dontBreakAtEntry, $$read("xxx.flt"); }`
- 需要 `REZ_INCLUDE_PATH`（Retro68.r 的 include 路径）
- 入口 `_start`：`RETRO68_RELOCATE()` + `Retro68CallConstructors()`
- 部署：System 6 = 系统文件夹根部（`'INIT'` 资源）；System 7 = Extensions 文件夹

### ⚠️ Time Manager 周期任务（全部实测失败）

| 尝试 | 结果 |
|---|---|
| 只 `InsTime`（tmCount=30）| **任务永不触发**（heart=0）|
| `InsTime` + `PrimeTime`（首次）| 触发一次 ✓ |
| `tmWakeUp` 周期 | **不自动重排**（heart 停 1）|
| 回调里 `PrimeTime` 重排 | **F-line → "coprocessor not installed"** |
| 回调里手动写 tmCount + InsTime | 不生效（ql=0）|

**结论：Retro68 + System 6 环境下，INIT 的 TM 周期轮询不可行。**

### 关键陷阱

- **A5**：TM 回调在系统上下文执行，A5 不指向 INIT 全局区——回调内**禁止字符串字面量/全局/依赖 reent 的库函数**（编译器可能 A5 相对寻址）→ 用绝对地址（SHM）+ 自写逐字节比较
- **D0**：`PrimeTime(__A0, __D0)`（`#pragma parameter`）——回调里调用会改 D0
- **F-line**：68000 无 FPU，执行 0xF000-0xFFFF opcode → "coprocessor not installed"——注意区分数据区的 F-line（反汇编 `.short 0xffxx` 可能是字符串/常量）与真执行到的
- `TMTask` 布局：qLink(0) qType(4) tmAddr(6) tmCount(0x0A) tmWakeUp(0x0E)——**tmCount 偏移是 0x0A**（写错会破坏 tmAddr）

### INIT 的正确用途（参考经典软件）

- **一次性**：图标（ShowInitIcon）、patch 系统、设备初始化——Retro68 支持良好
- **常驻 = patch trap（事件驱动）**：SuperClock/Timbuktu/DiskLight 都是 patch trap（改 trap 表，任何应用调该 trap 时执行），**不是 TM 轮询**
- **后台应用**：System 6 MultiFinder 后台应用**挂起时不运行**（主循环/TM/VBL 都停）；background-only 是否永不挂起**无文档依据**（System 7 明确，System 6 未知）

---

## 三、踩坑记录（按主题）

| 坑 | 现象 | 原因 | 解法 |
|---|---|---|---|
| Eject 传卷号 | `-56 nsDrvErr` 无 SONY 日志 | Eject 参数是驱动器号 | `Eject(0, 1)` |
| MFS eject | `-47 busy` | System 6 不支持 MFS UnmountVol | MFS 走 Flush+Eject |
| UnmountVol 后盘符消失 | 与 File > Eject 不一致 | 显式卸载多余 | 去掉 UnmountVol |
| `PBHGetVolParms` 全报错 | 全 vRefNum `paramErr` | FSDispatch selector 0x30 非 System 6 | 用 PBHGetVInfo |
| 卷遍历重复 | 同一卷出现两次 | HFS/MFS 遍历重叠 | 去重 |
| ParamText 乱码累积 | 每点一次多一串乱码 | `ctopstr` 原地转 Pascal 串污染 | 局部缓冲转换 |
| 按钮点不了 | FindControl/TrackControl 无效 | System 6 无 click-through / 控件细节 | 启动 SelectWindow；DITL 标准按钮 + ModalDialog |
| 嵌入 400K 超分区 | app 分区 2M 爆 | 原样嵌入镜像 | 压缩嵌入（zlib→ROM tinfl）或分区方案 |
| `#ifdef` + `=0` | 宏关闭无效 | `#ifdef` 检查"是否定义" | 用 `#if`（值检查）|
| embed 符号找不到 | undefined reference | 文件名大小写 → 符号名 | `_binary_Dash_400k_dsk_gz_start` 精确匹配 |
| INI 加载 F-line | coprocessor not installed | TM 回调/代码含 F-line | 见 INIT 章节 |

---

## 四、可行方案 vs 死路

### 可行（已验证或理论可靠）

1. **EjectApp（app）轮询 SHM**：普通应用事件循环查 SHM 命令 → 按卷名 eject——简单可靠，**代价：需 EjectApp 前台运行**（System 6 单任务）
2. **patch trap INIT**（Timbuktu/SuperClock 式）：INIT patch 高频 trap → 任何应用运行即检查 SHM——真后台，**复杂度高**（trap 表 + 跳板 + A5）

### 死路（实测）

1. **INIT + TM 周期轮询**：PrimeTime 回调 F-line、tmWakeUp 不重排、手动重插不生效
2. **普通后台 app 轮询**：挂起即停

---

## 五、方法论教训

1. **先找依据再动手**——executor 源码（time.c）、Retro68 samples（SystemExtension/Dialog）是权威参考；不要凭记忆/推测（D0 理论、PrimeTime 必须等，都曾被证伪）
2. **二分法**（可控最小化）：不注册 TM → 注册不触发 → 触发一次 → 回调最小——逐层定位
3. **反汇编是最快的真相**：`m68k-apple-macos-objdump -D -b binary -m 68000 xxx.flt`——F-line、寄存器污染、参数传递一目了然
4. **区分"数据区的 F-line"与"执行到的 F-line"**——反汇编里 `.short 0xffxx` 可能是字符串/常量
5. **宿主/客机分层**：core 层禁 ESP-IDF 头（PROJECT.md 规则），诊断代码放 arch 层
6. **用户价值判断**：网页只需 eject 自己插入的盘（卷名自己知道）——不需要通用卷列表/选择；写死名字 vs 属性判断的取舍要以实际场景为准

---

## 六、参考资源

- Retro68: `Samples/SystemExtension/`（INIT 构建）、`Samples/Dialog/`（按钮）、`TestApps/InitTest/`
- executor 源码: `src/time.c`（TM 递减/触发机制）、`src/refresh.c`（回调里 PrimeTime 标准用法）
- Multiversal: `defs/FileMgr.yaml`（file_trap 标记区分 HFS/MFS）、`defs/TimeMgr.yaml`
- hfsutils `libhfs.h`（HFS 卷属性位 `HFS_ATRB_*` 权威定义）
- 模拟器: `main/core/macplus/devices/sony.c`（SONY eject 处理）、`mac_traps_table.c`（trap 表）
