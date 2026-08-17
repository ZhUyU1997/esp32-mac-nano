# ANSI art 对比验证工作流程

目标：`vterm-ans` 渲染结果与 **libansilove（对比参考）**一致、**内容完整**
（不滚动/不截断）、符合 **ANSI.SYS（语义基准）**。

## 工具链

| 工具 | 用途 |
|---|---|
| `vterm-ans` | ANSI art → PNG（渲染器）|
| `vterm-ans --cell R,C` | **反向逆推**：cell → 源文件字节偏移（一次进程内，推荐）|
| `vterm-ans --trace-cells FILE` | 全量 cell→字节偏移映射表 |
| `tools/art/test-art.js --compare` | 批量对比（vs ansilove），逐文件 rate + 豁免 |
| `tools/art/compare.js one` | 单文件对比 → `compare.png`（三列：ansilove \| vterm-ans \| diff map）|
| `tools/art/compare.js list` | 按 `list.txt` 批量跑（→ test-art.js）|
| `tools/art/cell2byte.js` | 基于 trace 查表：pack entry row col → 字节偏移 + 上下文 |
| `tools/art/ddmin.js` | 自动最小化复现序列（delta debugging）|
| `tools/art/diff-lib.js` | 共享 diff 实现（test-art 与 compare one 同源）|
| `tools/art/art-lib.js` | 共享工具（zip、SAUCE、键盘脚本检测）|

图固定输出：`art-diff/latest.png`（最新对比图，每次分析自动更新）。

## 标准工作流

```
1. 批量跑      node tools/art/compare.js list > compare-output.txt
2. 提取 FAIL   → list.txt（pack/entry）
3. 单个检查    node tools/art/compare.js one <pack> <entry>  → art-diff/latest.png
4. 用户看图    判断差异类型（偏移/颜色/缺失/乱码）
5. 定位        vterm-ans --cell R,C → 源文件字节偏移（见下）
6. 理解+验证   读字节上下文 + 查参考源码 + 最小对照实验（见下）
7. 修复        vterm-ans / libvterm / cp437 / sauce / 对比工具
8. 回归        之前修复的文件全部 0.0%（防回退）
9. 记录文档    docs/ansi-sys-compat.md
10. 重跑批量   看 FAIL 降幅 → 回到 3
```

## 差异定位（定位 → 理解 → 验证）

**为什么需要**：批量对比报出 diff cell（如 `r12c55`）后，要找到源文件里
对应的字节才能分析触发序列。**正确做法是让渲染器自己报告**——不要二分
cut 全量渲染（慢、截断序列产生假差异）、不要手写状态机模拟（与真实渲染
不一致）。

### 1. 定位：`vterm-ans --cell R,C`（一次进程，秒级）

```bash
vterm-ans file.ans --cell 12,55
# => cell 12,55 last written by byte 3893 of 173012
#    + 该字节前后的上下文（ESC 序列可读）
```

原理：与正常渲染相同的 libvterm 状态机，逐字符 push，每推一个字符后用
`vterm_screen_get_cell` 监视目标 cell，记录最后一次改变它的输入字节
（天然处理覆盖/清屏：最后一次变化 = 最终内容）。

### 2. 理解：读字节上下文 + 查参考实现源码

定位到字节后，读该处序列 + **直接查对比参考的源码**（libansilove 在
`/tmp/libansilove-src`）——"为什么"的答案通常就在实现里，比任何实验都快。

案例（sm-dngnun.ans）：4 个 diff cell 定位后，源码一行确认根因——
libansilove `SGR 5`（blink）处理无条件 `background24 = 0`
（`src/loaders/ansi.c`），bg t 序列（`ESC[0;34;82;29t`）设置在 blink 之前
就被丢弃 → 背景 fallback 到 42m palette 绿。vterm 的 blink 是独立属性不
清 bg24，渲染正确。

### 3. 验证：最小对照实验

顺序反转对照（机制确认）：

```
ESC[42m ESC[0;34;82;29t ESC[5m ▀  → diff（bg t 被 blink 清掉）
ESC[42m ESC[5m ESC[0;34;82;29t ▀  → 一致（bg t 在 blink 后）
```

最小复现（20B）：`ESC[42m ESC[0;34;82;29t ESC[5m ▀`

### 4. 行为合理性考察（修复前判断"谁对"）

定位+验证给出"差异"后，判断**哪个实现的行为合理**（决定改 vterm 还是豁免）。
考察四个基准：

| 基准 | 源码位置 | 角色 |
|---|---|---|
| **ANSI.SYS** | `/tmp/MS-DOS/v4.0/src/DEV/ANSI/ANSI.ASM`（GRMODE 表）| 语义基准（BBS 时代）|
| **xterm / libvterm** | `libvterm/src/pen.c` | 标准终端语义 |
| **PabloDraw** | `/tmp/pablodraw/Source/.../Ansi.load.cs` | **t 序列等扩展的权威** |
| **libansilove** | `/tmp/libansilove-src/src/loaders/ansi.c` | 对比参考（可能复刻 PabloDraw）|

步骤：
1. **识别扩展语义的权威**：差异涉及的序列若是扩展（如 `t` 24-bit），
   **扩展作者的实现就是标准**——xterm/ANSI.SYS 没有该概念，无法裁决
2. **查各实现源码**，列行为矩阵
3. **判定**：
   - 标准场景（16 色/属性）：以 ANSI.SYS + xterm 为准（vterm 应符合两者）
   - 扩展场景（24-bit t 等）：以 PabloDraw 为准（libansilove 复刻它时视为一致）
4. **修复方向**：vterm 对齐权威行为——注意**对称性**（如 fg 侧已有
   `fg_from_t` 补丁，bg 侧缺失就是不完整实现）

案例（sm-dngnun）：`SGR 5`（blink）重置 24-bit bg——PabloDraw `case 5`
`rgbAttribute.Background = attribute.Background`（Ansi.load.cs）、libansilove
`background24 = 0` 一致；xterm/ANSI.SYS 无 24-bit 概念不参与裁决——
**结论：vterm 应加 bg_from_t**（与 fg_from_t 对称），修复后 4 diff → 0。

### 最小化注意事项

- **目标锁定**：删减必须保持**目标 diff cell** 仍 diff——只检查"任意 diff>0"
  会让 diff 漂移（13B 假象的教训）
- **补全截断**：输出若在 CSI 中间截断是**不合格测试**——从原文件补全完整
  序列再验证（128B 假复现的教训）
- **查源码优先**：最小化是验证手段，不是定位手段——定位完成先读源码
- **性能**：ddmin 对全文件单元级删减太慢——先粗删（远离 diff 区域）再对
  候选区单元级删减

## 决策原则

- **用户判断唯一标准是 diff 图**：所有分析/验证结论必须配 `art-diff/latest.png`
  （三栏 diff 图）——数字（diff 计数）只是辅助，用户看图判定。
- **不擅自动手**：豁免/下一个/修复都要**用户确认**。
- **参考基准**：libansilove（对比）+ ANSI.SYS（语义）——**修复优先于对齐**
  （内容完整 > 模仿参考）。
- **豁免需用户确认**：libansilove bug 类、文件损坏类、非 art 类——加进
  `test-art.js` 的 `EXEMPT_FILES`。
- **定位用 --cell**（不是二分）：让用户看图判断差异类型，再用工具定位。

## 已修复问题类别

| 类别 | 内容 |
|---|---|
| ANSI.SYS 语义（宏 `VTERM_ANSI_SYS_MODE`）| 右边界立即 wrap、CUF 钳制、ED2 归位、LF 隐含 CR、SGR 7 反显=属性覆盖（黑字白底）、0x0e 画 ♪ |
| 滚动（`vterm-ans.c` `scan_max_row`）| CUP row 修正、CUU 不减、CUF wrap 模拟、字符列推进、rows 余量 128 + cap 8192 |
| CP437（`cp437.c`）| C0 区 27 字形、0x7F ■、0x00 空格占位 |
| SAUCE（`sauce.c`）| 字段偏移（version 双布局）、cols 校验 40..200、title 空回退、version 只认 "00" |
| 颜色 | 默认前景 7 号、CGA 调色板（pen.c）、PabloDraw 24-bit（`t` 序列）、ED2 默认背景（screen.c）|
| 工具 | decode-worker 任务 ID 匹配（并发串线修复）、compare.js 行数差灰色标记、vterm-ans --cell/--trace-cells、diff 图全高修复 |

## 豁免名单（`test-art.js`）

**`EXEMPT_FILES`**（libansilove bug / 损坏 / 用户判定）：
- 旧批：MEM0595 / PK-NUCW / AL-DTD / DG-MAKC2 / -------- / US! / cm-MIST /
  sk!n-abstrakt_nfo_fb / sk!n-motiv8_logo_ansi / mz-piece / grx-comp2 /
  grx-comp7 / ldn-vandalism / +l-ds / g80-hmm / ru8_factory
- 2026-08-17 追加：ITSOVER（光标动画）/ FILE_ID（SAUCE 32 列）/ lmn-siouxie
  （blink 亮背景）/ MM-ONE（行数）/ NAUGFLAG（反显语义）/ Arl-Rat、
  us-mistimpure（数据损坏）/ Swansi / BYM_FOREVER / die-already（TAB）/
  fil-metal（CR 归位）

**`EXCLUDE_FILES`**（非 80 列固定设备无法显示）：fool27.zip-file_id /
mist0918.zip-FILE_ID / impure84.zip-lmn-siouxie / mist0823.zip-MM-ONE

## 当前状态

- 全量对比：2026-08-17 跑（52,888 素材，21m46s）——FAIL 9（4 渲染差 +
  5 inflate 损坏），渲染差已全部豁免/修复
- decode-worker 串线修复后，全量/单文件结果一致（假 FAIL 消失）
- 待办：sm-dngnun 是否豁免（vterm 正确，ansilove SGR 5 清 bg24 bug，
  0.01% 差异）
