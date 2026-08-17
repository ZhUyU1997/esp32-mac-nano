# ANSI.SYS 兼容性调研

ANSI art 渲染的本质是 **ANSI.SYS 兼容**：BBS 艺术家以 MS-DOS ANSI.SYS 的行为为目标画图。
本项目 libvterm（xterm 语义）与 ANSI.SYS 存在多处差异，已用宏 `VTERM_ANSI_SYS_MODE`
控制，本文档记录调研结论（2025，对照 libansilove 源码与实测）。

## 历史背景

- **ANSI.SYS**（MS-DOS 1981）= IBM PC 上 X3.64 标准的 **11 个命令 + 3 个扩展**
  （Kermit 95 glossary），定义了 PC BBS 时代"ANSI 终端"的全部语义。
- BBS 艺术家以 ANSI.SYS 行为为目标作图；xterm/VT100 是另一条线。
- xterm 的 `xenl`（"newline glitch"）wrap 语义正是右边界 wrap 差异的根源
  （Debian xterm 手册明确记载）。
- ansi-bbs.org 规范直言：行尾 wrap "没有官方正确行为"，各终端实现不同，
  艺术家被迫避开最后一列 —— wrap 语义是 art 兼容的第一痛点。

## 已修复的 3 处（宏 `VTERM_ANSI_SYS_MODE`）

宏在 `libvterm/src/state.c` 内 `#ifdef` 控制，构建系统（xmake/ESP32 CMake）默认开启。

| 序列 | ANSI.SYS / libansilove | xterm / libvterm | 位置 |
|---|---|---|---|
| 右边界写字符 | 立即换行（不设 phantom）| phantom wrap（xenl）| `on_text` |
| CUF 越界 | clamp 右边界 | 允许越界 | `on_csi` 0x43 |
| ED 2 清屏 | 清屏 + **归位** + 丢弃之前字符 | 只清屏（光标不动）| `on_csi` 0x4A case 2 |

ED 2 差异的发现过程：对比工具显示 2J 类文件（WWANS 系列）内容整体下移，
libansilove 跳过 2J 前的 BBS header 文本、我们从行 0 开始画。最小复现：

```
header line 1\r\n
header line 2\r\n
\x1b[2J\x1b[31mCONTENT\x1b[0m\r\n
```

修复前 vterm-ans 输出 3 行（顶部 2 空行）、ansilove 1 行；修复后均为 1 行，
WWANS370 严格对比从 61.9% 错位降到 0.21%（天然对齐，无偏移容忍）。

## 完整差异清单（对照 libansilove src/loaders/ansi.c）

### libansilove 文本层行为

| 字节 | libansilove | libvterm | 备注 |
|---|---|---|---|
| LF (0x0A) | `row++` 且 `column = 0`（**隐含 CR**）| 只换行，列不变 | 未处理；素材实证无可见影响 |
| CR (0x0D) | **忽略** | 回列 0 | libansilove 简化 |
| TAB | `column += 8`（无 tab stop）| 到 tab stop | 素材 0.1% |
| SUB (0x1A) | 终止解析 | — | 素材为文件尾 EOF 标记 |
| 列满 | **无条件自动换行**（不看 autowrap）| 受 DECAWM 控制 | 与已修 wrap 宏配合一致 |

### libansilove 序列层（只处理这些，其余跳过）

`H/f`(CUP) `A B C D`(光标移动) `s/u`(保存/恢复) `J`(仅=2) `m`(SGR) `p`(Amiga 光标，跳过)
`h/l`(SM/RM，**全部跳过**) `K`(EL，**跳过**) `t`(PabloDraw 24bit)。

| 差异 | 素材出现率（1580 样本）| 实测影响 |
|---|---|---|
| LF 隐含 CR（\n 行尾）| 0.8%（13 文件）| EDDY 严格对比 0.00%，**无影响** |
| EL（\x1b[K）| 0.5%（8 文件）| libansilove 不擦行（它的简化）；我们正确擦除 |
| DECAWM off（\x1b[?7l）| 0.0% | libansilove 忽略总换行；我们正确生效 |
| TAB 无 tab stop | 0.1% | 罕见 |
| CUF clamp 值 | `columns`（下字符换行）vs 我们 `columns-1` | 边界差 1（越界+立即写字时）|
| SGR 33 黄/棕 | libansilove 调色板 3 号=棕 (170,85,0) | 对比工具用归一化索引容忍 |
| bold 顺序 bug | `32;1` 不加亮、`1;32` 加亮 | 对比工具计入 bright 计数 |
| 90-97 亮色 | 全部落灰 (170,170,170) | libansilove 不支持 |
| rowMax vs 内容行 | libansilove 用光标最大行 | 尾部空行差 1-2 行（对比按 min 行比较）|

### 关键结论

1. 已修三处（wrap、CUF、ED2）是主战场，素材中高频出现。
2. 其余差异中 **libansilove 是简化方**（EL 不擦、?7l 忽略、CR 忽略、90-97 不支持）——
   这些我们比它更正确，**无需对齐**。
3. **LF 隐含 CR 是我们缺的**（它 `row++` 且 `column=0`），但素材实证无可见影响
   （0.8% 文件、EDDY 对比 0.00%）——**低优先候选**，若加也走 `VTERM_ANSI_SYS_MODE`。
4. 反向原则：只要不依赖 xterm 专属语义（已修三处），其余保持终端正确行为即可。

## 无限画布 vs 固定屏幕滚动（vterm-ans 渲染工具语义）

**现象**：长 art（>256 行）渲染后内容整体上偏（顶部被滚出）。
TDT-CE1.ANS（1993）270 行内容 → vterm-ans 输出 255 行、ansilove 270 行。

**根因**：libvterm 是固定屏幕（256 行）+ 滚动；libansilove 是无限画布
（rowMax 追踪光标最大行，不滚动）。BBS art 的模型是"画布无限"——长 art
逐行画，任何一行都可能落笔。

**修复**（`vterm-ans.c` `scan_max_row()`）：渲染前预扫描字节流，统计
LF 行数、CUP（`\x1b[n;mH/f`）跳转、CUD/CUU（`nB/nA`）移动、ED 2 归位，
得到内容可达的最大行，动态设定 libvterm 屏幕高度（下限 256，上限
2048）。像素缓冲随行数增长（2048 行 ≈ 84MB，可接受）。

**注意**：这是**渲染工具**语义。ESP32 画廊是真实终端，**保持滚动**
（正确行为）；只有 vterm-ans 用无限画布对齐 libansilove。

## scan_max_row CUP 漏算（vterm-ans 屏幕高度低估）

**现象**：CUP 定位 + LF 流结构的 art（90 年代典型）内容整体上偏。
TT-TT.ICE（1993）首个内容行 34 vs libansilove 56（差 22 行）；
FS-FF3（50.7%）、IM-EOT1（34.7%）同类。

**定位过程**：二分切文件——前后两半独立渲染都 0.0%，完整文件 57.7%
（差异跨段）；前缀（14839B）渲染内容在行 56，前缀 + 500 空行后
（抬高 scan 值）行 34 一致 → 确认 scan_max_row 低估 → rows 太小 → 滚动。

**根因**（`vterm-ans.c` `scan_max_row()`）：CUP 分支只更新 `max`
不更新 `row`——文件头 CUP 2-25 初始化定位被漏算（22 行），后续 LF
从 0 而非 CUP 位置累积 → 屏幕高度低估 → 内容滚动上偏。

**修复**：CUP 时同步 `row = r - 1`（CUP 移动光标，后续 LF 从该行
起算）。验证：TT-TT.ICE 57.7%→0.0%、FS-FF3 50.7%→0.0%、
IM-EOT1 34.7%→0.7%。

## SUB (0x1A) 终止 art 内容（vterm-ans）

**现象**：IM-EOT1.ANS（1994）我们比 libansilove 多 1 行——尾部多出
一行字串。

**根因**：文件末尾 `\x1a`（SUB/EOF 标记）+ BBS 元数据
（`IM-EOT1.ANS\x00End of Time`）。libansilove 遇 0x1A 进 STATE_END
**停止解析**；我们把 0x1A 转成 U+2192（→）字形并把后面的文件名/
描述全部渲染出来。

**修复**（`vterm-ans.c`）：扫描 art 字节，遇到第一个 0x1A 即截断
（与 libansilove 一致），再 trim 尾部 EOF/NUL。0x1A 是 BBS 文件的
内容终止标记，其后是上传元数据。

**验证**：IM-EOT1 272→271 行（与 libansilove 一致）。

**排查工具**：`tools/art/compare.js one` 输出并排对比图（带行号），
`tools/art/compare.js list` 按 list.txt 批量跑（坏 CRC 自动剔除）。

## CGA 调色板还原（libvterm pen.c）

**目标**：还原 BBS 时代 DOS 真实颜色。libvterm 原调色板是 xterm 风格
（0/224/64 阶梯，SGR 33=黄）；改为 CGA / ANSI.SYS 表（libansilove
实测同款）：暗色 0/170 阶梯、亮色 85/255 阶梯、**SGR 33 = 棕**
(170,85,0)。影响 vterm-ans 和 ESP32 画廊（同用 libvterm）。

```
0黑 1红(170,0,0) 2绿(0,170,0) 3棕(170,85,0) 4蓝 5紫 6青 7灰
8深灰(85) 9亮红(255,85,85) 10亮绿(85,255,85) ... 15白
```

## 默认前景色修复（vterm-ans）

**现象**：SGR 0 重置后未指定颜色的字符（`\x1b[0m\xdb` 等）渲染成
xterm 白 240，libansilove 是 7 号灰 170 —— IM-EOT1 0.7% 差异全来自此。

**修复**：`vterm-ans.c` 初始化时设置默认前景 = CGA 7 号灰、背景黑：

```c
VTermColor def_fg, def_bg;
vterm_color_indexed(&def_fg, 7);
vterm_color_indexed(&def_bg, 0);
vterm_state_set_default_colors(vterm_obtain_state(vt), &def_fg, &def_bg);
```

**验证**：IM-EOT1 0.7% → 0.0%。

## LF 隐含 CR（bare-LF art）

**现象**：SC-ICE1/2/6.ICE（1994 ice logo 包）对比 FAIL（48%/35%/31%）
——最后两行内容整体右偏。

**根因**：这三个文件是 **bare-LF 行尾**（CR=0）——libansilove 的 LF
是 `row++ 且 column=0`（隐含 CR）；libvterm 的 LF 只换行（列保持
，仅 LNM 模式归列）。行尾不在列 0 时，下一行从行尾列开始写 → 右偏。

**修复**（`libvterm/src/state.c` 0x0A case）：`VTERM_ANSI_SYS_MODE`
下 LF 后 `state->pos.col = 0`（ANSI.SYS 行为，与 libansilove 一致）。
CRLF 文件不受影响（\r 已归列）。

**验证**：SC-ICE1/2/6 全部 0.0%；LD-COD 回归 0.0%。

## SAUCE 解析固定官方偏移（sauce.c，删除 version 检测）

**现象**：BS-SOFT!.ANS（1994 lbo-r4）100% FAIL——我们 79 列 vs
libansilove 80 列（这是 libansilove 漏读 SAUCE 宽度，非我们解析错）。
另 009.ans（2024）裸跑渲染 60 列宽（真实 80 列）。

**根因**：早期假设存在"无 version 布局"（title@5/7），加了 version
检测 `r[7:9]`——但官方 ACiD rev5 布局是 `ID(5)+Version(2)@5+Title@7`，
r[7:9] 是 **title 前两字符**。title 以 "00" 开头时误触发偏移 +2，
整条记录错位（009.ans：title"009"→"9"、cols 80→60）。

**修复**（`sauce.c`）：删除 version 检测，固定官方偏移
（title@7、cols@96、rows@98、TFlags@105）。`sauce_locate` 已要求
"SAUCE00/01" 前缀，"无 version 记录"不可能到达解析。

**验证**：42,158 个素材文件两种解析对比——42,156 完全一致（v 检测
从未起作用，全走官方 v=0），仅 2 个 "00" 开头 title 文件不同且官方
全部修对（CRS-CCL title "0j"→"000j"、009 title "9"→"009" cols
60→80）；三个历史案例（HO-COLLY/ANSI0197/BS-SOFT!）官方解析全部
正确；vterm-test 夹具同步改回官方布局；vterm-test 231 passed。

**说明**：ansilove 对部分 SAUCE 记录漏检宽度（BS-SOFT! 79 列）——
属 libansilove 检测问题，非我们解析错误。

## PabloDraw 24-bit 色（\x1b[0/1;R;G;Bt）+ rows cap 8192

**现象**：tatooine.ans（2014）51.1% 颜色差异——PabloDraw 真彩色
（`\x1b[0;R;G;Bt` 序列 336 个）我们忽略；Blocktronics-WTF4_Megajoint
4641 行超 rows cap 4096 截断。

**修复**：
- `libvterm/src/state.c` 加 case 0x74（PabloDraw t）：RGB 设置
  pen fg/bg，通过 setpenattr 回调同步 screen->pen。
- `vterm-ans.c` rows cap 4096 → 8192（现代 megajoint 超长）。

**验证**：tatooine 51.1%→0.5%；Megajoint 4641=4641 行 0.0%。

## CP437 0x7F 字形（实心方块）

**现象**：mmc20-05.ans / mmc21-03.ans（2001 bommc01）25=25 行但
39.6% 内容差异（大量 `\x7f` 字符缺失）。

**根因**：0x7F 在 CP437/VGA 是 **■ 实心方块**（U+25A0）——我们的
映射 `b < 0x80` 原样透传（DEL 控制）→ libvterm 忽略；libansilove
画 ■。C0 区（0x01-0x1F）已修，0x7F 遗漏。

**修复**（`cp437.c`）：0x7F → U+25A0（■）。

**验证**：mmc20-05/mmc21-03 0.0%；回归全过；vterm-test 231 passed。

## SAUCE cols 校验范围收紧（<40 回退）

**现象**：M2-SCR.ANS（1997 fsn-0397）我们 21 列 vs 80——宽度不对。

**根因**：损坏记录 cols 字段是垃圾值（21，非真实列宽——art 是
80 列）。

**修复**（`sauce.c`）：cols 校验范围 40..200（BBS art 40-200 列，
<40 不是真实 art 宽度）→ 回退默认 80。

**验证**：M2-SCR 0.0%；BS-SOFT!（真实 79 列）回归正常（40..200
内保留）。

## 空 SAUCE 记录回退（title 全 \x00）

**现象**：GN-COL#1.ANS（1996 pyro02）我们 78 列 vs libansilove 80 列、
行数多 62（更窄 wrap 更多）。

**根因**：SAUCE 记录全 \x00（title 空）——cols 位置读到垃圾 78
（真实 art 是 80 列）——我们跟 78 列 → wrap 多 → 行数膨胀。
前缀（去 SAUCE）80 列渲染完全一致。

**修复**（`sauce.c`）：title[0] == '\0'（全空记录）→ columns 回退 0
（默认 80）。

**验证**：GN-COL#1 346=346 行（0.0%）；回归全过；vterm-test 231 passed。

## ED 2 清屏默认背景（screen.c）

**现象**：BV-FIRE.ANS（1996 fire0696）我们 256 行 vs libansilove 95 行
（背景颜色不对——我们紫背景填满）。

**根因**：`\x1b[45m\x1b[2J`（先设紫底再清屏）——libvterm 的 ED 2
用**当前 pen 背景**（紫）清屏 → 全屏紫背景（screen_last_row 把
非黑背景算内容 → 256 行）；libansilove 的 2J 丢弃 buffer（黑背景）。
ANSI.SYS 的 2J 清屏用默认背景（黑）。

**修复**（`libvterm/src/screen.c` erase_internal）：`VTERM_ANSI_SYS_MODE`
下全屏 erase（ED 2）用默认黑背景，其他 erase 保持当前 bg。

**验证**：BV-FIRE 256→95 行（0.0%）；回归全过；vterm-test 231 passed。

## bare-CR 文件豁免（libansilove CR 忽略 bug）

**现象**：MEM0595.ANS（1995 flat0595）我们 1 行 vs libansilove 38 行
（71.3%）。

**根因**：文件用 `\r` 单独（无 `\n`，48 个）做"回列重画"——
ANSI.SYS 的 CR = 回列 0（不换行）；libansilove `case CR: break`
**忽略**（列不动）→ 列漂移 → 错乱 wrap 成 38 行（bug 展开）。
我们与 ANSI.SYS 一致（回列重画，1 行真实画面）。

**处理**：`isBareCR()`（`\r` > 0 且 `\n` = 0）→ 豁免（skip，标注
libansilove CR-ignore bug）。

## SAUCE 宽度差异（列数差按 min 列比较）

**现象**：一批 100% FAIL（PI-JABY1、PH-TAC0、MJ-* 系列等）——列数差
（我们 79 vs libansilove 80）。

**根因**：无 version 截断 SAUCE（记录 121 字节）——vterm-ans 兼容
解析读到 79 列；libansilove 严格检测漏检 → 默认 80。

**处理**（diff-lib.js cellDiff）：列数不同时**不再直接 rate=1**，而是
按 min 列数比较重叠列的内容——内容一致（rate 0）自然通过；内容
不一致照 FAIL。比"豁免标注"更严谨：真实验证 79 列内容吻合。

**验证**：PI-JABY1 79 列内容 cmp=0.0%（自然通过）；回归全过。

## scan 加普通字符列推进（长行 wrap）

**现象**：CO-CATS1.ANS（1995 bdp-1095）545 vs 544 行、顶部内容被
覆盖（行 0 从 8904px 变 4256px）。

**定位**：二分——前 24 行行 0 保持、行 24（32KB 超长段）加入后行 0
变——行 24 wrap 97 行超 rows → 滚动顶出行 0。

**根因**：scan_max_row 只模拟 LF + CUF 超界的 wrap——普通字符
（`\xdc` 块等）写满 80 列也 wrap——长行（32KB）wrap 数百行——
scan 低估 → rows 小 → 滚动。

**修复**（`vterm-ans.c` `scan_max_row`）：普通字符（≥0x20）和 NUL
推进列，列满（≥80）wrap 推进行。

**验证**：CO-CATS1 545=545 行（0.0%）；全部回归 0.0%；vterm-test
231 passed。

## NUL（\x00）列推进（cp437.c）

**现象**：T1-ICB2.ANS（1995 bleach04）78 vs 53 行、内容错位。

**定位**：二分——前 8 行一致、前 10 行差（行 9 触发）——行 9 是
`\x1b[D\x00`（退格+NUL 擦除）模式。

**根因**：`\x00`（NUL）列推进差异——libansilove 把 NUL 当空白字符
画（推进列）；libvterm 忽略 NUL（不推进）→ `\x1b[D\x00` 退格+NUL
模式：libansilove 净列 0（-1+1）、我们净 -1 → 列差累积 → 内容错位。

**修复**（`cp437.c`）：`\x00` → U+0020（空格）——VGA NULL 字形是
空白，渲染为空格并推进列（与 libansilove 一致）。

**验证**：T1-ICB2 78=78 行（0.0%）；回归全过；vterm-test 231 passed。

## scan_max_row 加 CUF/CUB/CR 模拟（CUF 超界 wrap）

**现象**：HD-HW.ANS（1994 uni-0894）357 vs 254 行——顶部内容被滚出。

**根因**：`\x1b[nC`（CUF）超右边界时，下一个字符 wrap 到下一行
（libansilove clamp 到列宽、字符触发换行）——BBS art 大量用
`\x1b[79C\x1b[1C` 定位——HD-HW 有 575 个 CUF，wrap 推进 147 行——
scan 只数 LF（210）→ 低估 → rows 小 → 滚动。

**修复**（`vterm-ans.c` `scan_max_row`）：模拟列位置——CUF 超界
时 row++（wrap）、CUB 回列、CR 归列。

**验证**：HD-HW 254→357 行（0.0%）；GS-S1002/LD-COD/SC-ICE1/
TDT-CE1 回归 0.0%；vterm-test 231 passed。

## scan 低估余量（rows +128，滚动丢内容）

**现象**：GS-S1002.ANS（1994 stl-001）我们 269 行 vs libansilove 310 行
——顶部内容被滚出（"上移几行、上面没有"）。

**定位**：二分——前 225 行一致、前 226 行开始差（行 0 被覆盖 72 cell）、
前 260 行行 0 全空；rows 强制 600 后内容完整（310 行）→ 滚动确认。

**根因**：scan_max_row 无法精确预测行号膨胀（CUF 列 wrap、CUU
覆盖、边界情况）——GS 低估 41 行（scan 269 vs 内容 310）→ rows 小
→ 内容写满后滚动 → 顶部滚出。

**修复**（`vterm-ans.c`）：rows 余量从 +1 加大到 +128（宁大勿小——
只费内存，滚动才丢内容）。

**验证**：GS-S1002 269→310 行（0.0% 完整）；LD-COD/SC-ICE1 回归
0.0%。

## 键盘重映射序列（\x1b[...p）豁免

**现象**：CLS.ANS / FORMAT.ANS（1994 ansib 包）对比 FAIL（50%/95%）。

**本质**：`\x1b[P1;P2;...;Pk p` 是 **ANSI.SYS 专有扩展**——按键重定义
配置（如 `\x1b[32;99;108;115;126;13p` = 空格键→"cls\r"）。**不产生
任何屏幕输出**（终端配置操作）。此类 .ANS 是 BBS 键盘宏脚本，非 art。

**三方行为**：ANSI.SYS 配置无显示 ✓；libvterm 忽略（无显示语义）✓；
**libansilove 解析 bug**（跳转边界算错，把参数字符当文本画出）。

**处理**：与 libansilove 一致——我们行为正确（同 ANSI.SYS），此类
文件豁免（非 art，对比/统计跳过）。

## scan_max_row 的 CUU（\x1b[A）处理（滚动修复）

**现象**：LD-COD.ANS（1994）我们 793 行 vs libansilove 794 行、画面
整体上偏 1 行（已画的行也偏）。

**定位**：逐行二分——前 657 行一致，前 658 行开始差 1。

**根因**：`scan_max_row` 对 `\x1b[A`（CUU）减 row——LD-COD 全文件
2156 个 `\x1b[A`（回上覆盖画）——覆盖行被算小 → 屏幕高度低估
（scan=82 vs 内容 255）→ 内容写满 rows 后**滚动** → 整个画面
（含已画行）上移 1 行，顶部滚出。

**修复**：`\x1b[A` 不减 row（宁大勿小——多分配无害，低估才滚动
丢内容）。验证：LD-COD 28.0% → 0.0%（794=794 行）。

## diff 实现统一（tools/art/diff-lib.js）

test-art.js（批量）与 compare.js one（单文件）原先各写一套 diff
（node 逐 cell vs python），量化表不同步曾导致同一文件 rate 不一致
（14.9% vs 0.7%）。抽出 `tools/art/diff-lib.js` 为唯一实现
（decodePngRgb / nearestIdx / cellTopIdx / cellDiff），两个工具
require 同一模块；compare one 的 python 只剩 PIL 拼图（diff map
数据由 node 计算后传 JSON）。

## 相关工具

- `tools/art/test-art.js --compare`：与 libansilove 逐文件对比
  （索引空间量化 + 归一化 + 严格 1:1 行比较，无偏移容忍）。
- `tools/art/compare.js one`：单文件渲染对比（输出项目根 `compare.png`）。
- `tools/term-mode.sh`：判别终端是 ANSI.SYS 还是 DECAWM 语义。

## SGR 1（bold）与 24-bit 前景色的交互（t 序列）

**现象**：nf-ape.ANS（2017 fuel21，PabloDraw 24-bit art）36/8320 cells
差异（0.4%）——集中在 t 色 + bold 组合处（行 17 列 50 等）：libansilove
的 `\xdf` 用 16 色（亮白/黑）、我们保持 t 色 RGB（230,209,188）。

**定位**：最小复现 `\x1b[1;230;209;188t\x1b[1;47m\xdf`：
- libansilove：SGR 1（bold）执行 `foreground += 8; bold = true;
  foreground24 = 0` —— **清掉 t 序列设的 24-bit 前景色**，fg 回 16 色
- vterm：bold 只是属性，pen.fg 的 RGB（t 色）保留 —— xterm 语义
  （查证：xterm/现代终端 bold 不清 38;2 真彩色，仅 legacy 16 色加亮）

**判断**：两种都是完整语义（libansilove 选"bold 时回 16 色 bright"、
xterm/vterm 选"bold 保持 RGB"）——非 bug，是语义选择差异。ANSI.SYS
无 24-bit 概念（n/a）。PabloDraw 编辑器内的实际显示无法确认（作者
意图参考缺失）。

**处理**：豁免（libansilove 语义选择类）。vterm 保持 xterm 语义
（bold 保留 t 色 RGB）。

## 上游 libvterm bug：UTF-8 跨块切分损坏（已反馈）

**现象**：限速流式传输（`--baud` art 画廊）下随机出现灰色空心框
（U+FFFD 渲染成 hollow-box placeholder）；慢速时更多。

**根因**：libvterm `on_text()` 按**块首字节**选解码器实例（ASCII 首块 →
`encoding[gl_set]`，高字节首块 → `encoding_utf8`）——utf8 模式下两个
实例解码函数相同但 **pending 状态（`bytes_remaining`）独立**。一个
UTF-8 序列跨"ASCII 首块 → 高字节首块"边界时，前半段 pending 丢失，
后半段变孤立 continuation → U+FFFD。上游原样代码（官方 0.3.3 复现）。

**修复**（本地 vendored `libvterm/src/state.c`）：utf8 模式统一走
`encoding_utf8`，pending 状态跨块连续（1 行顺序调整）。

**反馈**：https://bugs.launchpad.net/libvterm/+bug/2163595

**复现包**：`tools/vterm/libvterm-bug-report/`（官方上游一键复现，零本项目依赖）
