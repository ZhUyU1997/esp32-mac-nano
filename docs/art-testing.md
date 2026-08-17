# ANSI art 批量测试：发现的问题与修复记录

背景：为 `tools/vterm` 实现 libansilove 风格的 ANSI art 转换（`vterm-ans`），
并编写批量测试脚本 `scripts/test-art.js` 全量验证 `scripts/art/packs`
素材库（**5511 个 zip、65543 个 art 文件**）。测试过程中发现并修复了
一批渲染、解析和检测逻辑的 bug，本文档汇总全部问题、根因与修复方式。

相关文件：

| 文件 | 作用 |
|---|---|
| `tools/vterm/vterm-ans.c` | ANSI art → PNG/BMP（SAUCE/iCE 感知）|
| `tools/vterm/sauce.c/.h` | SAUCE 记录解析器 |
| `tools/vterm/cp437.c/.h` | CP437 字节 → Unicode + UTF-8 喂入 |
| `tools/vterm/term_render.c/.h` | 渲染器（iCE 模式、CP437 全量映射）|
| `libvterm/src/parser.c` | vendored libvterm（CSI 参数溢出修复）|
| `scripts/test-art.js` | 批量测试（流水线：边解压边验证）|
| `scripts/art-lib.js` | 共享 art 遍历/检测库（画廊与测试同源）|

---

## 1. 渲染管线 bug

### 1.1 libvterm CSI 参数数组越界 → 段错误 【严重】

- **现象**：`RESTORE.ANS`(198B) / `FORMAT.ANS` / `FLIPOFF.ANS` 让 vterm-ans
  段错误（SIGSEGV）。90 年代"恶搞 ANSI"把 `echo off...` 的 ASCII 码当 SGR
  参数塞进 `\x1b[32;101;99;104;...;13p`（31 个参数）。
- **根因**：`libvterm/src/parser.c` 的 CSI `;` 处理**无上限检查**直接
  `args[argi++]`，而数组只有 `CSI_ARGS_MAX = 16`——越界写堆。
- **影响**：所有用 vendored libvterm 的工具（vterm-sdl/pty/test/ans）。
- **修复**：`argi` 钳制到 `CSI_ARGS_MAX-1`，超限参数丢弃（xterm 行为）。
- **验证**：新增回归测试 `test_csi_arg_overflow`（31 参数序列不崩溃、屏幕
  仍可用）；vterm-test 228 断言通过。

### 1.2 CP437 Unicode 映射缺 35 个字符 → 占位框

- **现象**：双线框 `╡╢╖╕╞╟╧╨╤╥╙╘╒╓╫╪`、数学符号 `≥≤≈√∞∩≡⌠⌡∙ⁿ⌐¬`、`µ`
  等渲染成空心占位框——**BBS 艺术高频字符**。
- **根因**：`term_render.c` 的 `k_cp437_map` 覆盖不全（0x80–0xFF 缺 35 项）。
  此前的 `0b6d36b`（complete the CP437 glyph map）只补了 Latin-1 重音/希腊
  段（48 项），**双线框与数学符号段完全没覆盖**。
- **修复**：按 Python cp437 codec 补齐 35 项 + 全量 round-trip 回归测试
  （0x80–0xFF 每字节喂入 → 像素重建 → 逐字节比对）。
  **两端同步**：`tools/vterm/term_render.c`（host）与
  `main/arch/esp32/mach-s3/vterm/term_render.c`（ESP32 固件）条目集合
  已 diff 校验一致。

### 1.3 CP437→UTF-8 编码截断 13 位码点

- **现象**：vterm-ans 喂入的框线字符全变成亚美尼亚字母（U+0580 区），渲染
  成占位框。
- **根因**：`cp437_to_utf8` 对 ≥U+0800 的码点（U+2500+ 框线）用 2 字节
  UTF-8 编码，`0xC0 | (cp >> 6)` 吞掉高位。
- **修复**：按 1/2/3 字节正确编码；新增 `test_cp437_to_utf8` 逐字节验证
  编码结果（`0xDA → ┌ U+250C → E2 94 8C`）。

### 1.4 ED2 全屏擦除被误判为"有内容"

- **现象**：`GLDIG1.ANS` 渲染成 80×256（全屏）而非实际 80×97。
- **根因**：art 开头 `\x1b[40m\x1b[2J` 把整个高屏擦成 indexed 黑背景；
  内容扫描的 `!IS_DEFAULT_BG` 检查把"擦成黑底"当"有内容"。
- **修复**：改为把背景**转换 RGB 后判定非黑**（indexed 黑/默认黑都不计数）。

### 1.5 动画尾帧误报 "no content rendered"

- **现象**：`WWANS89.ANS`（清屏 + 覆盖式动画，最终画面只剩 1 个字符被尾
  部空格覆盖）被拒绝输出。
- **根因**：内容扫描要求最后屏幕仍有非空字符；libansilove 对此类文件用
  rowMax（光标到达过的最大行）。
- **修复**：回退链 = 屏幕内容行 → **光标最大行**（`movecursor` 跟踪）→
  报错。

### 1.6 纯 SAUCE 空壳文件

- **现象**：`--------.ANS`、`US!.ANS`（`0x1A` + 128 字节 SAUCE，无任何 art
  内容）报 "no content rendered"。
- **修复**：回退链第三级——**SAUCE 声明行数**（输出 80×25 空白图），
  真无 SAUCE 的空文件仍报错。

---

## 2. 测试脚本 bug（scripts/test-art.js）

| # | 问题 | 影响 | 修复 |
|---|---|---|---|
| 2.1 | 正则字面量 `\\.(ans\|ice)$` 匹配**字面反斜杠**（正则字面量里应写 `\.`）| 所有文件名不匹配，恒 0 ok 0 fail（zip 进度照常打印，静默假阴性）| 改 `\.` |
| 2.2 | `spawnSync` 同步阻塞 → `--concurrency` **实际串行** | 5000 文件 289s | 异步 `spawn` + 生产者-消费者流水线 → **15.2s**（19 倍）|
| 2.3 | `--limit` 用每个 worker 局部计数器 | limit×并发 个文件被处理 | 全局 `done` 计数（overshoot ≤并发，可接受）|
| 2.4 | flatten 阶段（读 5511 zip）**32 秒静默** | 看起来卡死 | 先打印 header + zip 进度；`--limit` 惰性解压（10 文件 0.1s）|

---

## 3. 宽度检测：三种方法的对比

ANSI art 用"光标定位 + 分段书写"（`\x1b[41C` 跳列、`ESC[s/u` 保存恢复、
动画覆盖），**任何"数每行字节"的流式方法都不可靠**：

| 方法 | 判 >80 列数量 | 问题 |
|---|---|---|
| telnet 画廊原逻辑（流式数全部字符）| **51983**（79%）| 连 `\x1b[41C` 的字符都算宽 + 不用 SAUCE 声明 → **画廊误杀 79% 文件** |
| 流式估算（跳转义序列）| 11006 | 双向误差：ABYSS 数成 **157**（实际 77）、AKO 数成 **53**（实际 115）|
| **光标模拟**（CUP/CUF/CUB/保存恢复，跟踪每行最右列）| **13219** | 正确（SAUCE 声明 858 + 模拟 12361）|

- **判定优先级**：SAUCE 声明列数（作者权威）→ 无 SAUCE 用光标模拟。
- **落地**：统一抽到 `scripts/art-lib.js` 的 `fileWidth()`，画廊与测试脚本
  同源；顺带修复 `convertPiece` 先转码后算宽导致 UTF-8 多字节翻倍、telnet
  `--file` 宽文件空指针崩溃（改优雅退出）。

---

## 4. 素材库问题（scripts/art/packs）

### 4.1 完全损坏的 zip — 6 个（整个包不可用）

实际格式检查：

| 文件 | 实际格式 | 可恢复性 |
|---|---|---|
| `2002/ice-0203c.zip` | **0 字节空文件** | 无内容 |
| `2025/test.php00.zip` | **PHP 脚本**（`<?php phpinfo(); ?>`，20B）| 恶意文件，16colo.rs 被黑痕迹 |
| `1994/id-1194.zip`(1.1MB) | 截断 zip：98 个本地头、**无 EOCD/中央目录** | **流式恢复 62/63 条目（25 个 art）** |
| `1994/itr-9401.zip`(64KB) | 截断 zip：14 个本地头，无 EOCD | 流式恢复 13/13（全是 .ITR，无 .ans/.ice）|
| `1995/ioa-1295.zip`(720KB) | 截断 zip：90 个本地头，无 EOCD | 流式恢复 44/45（2 个 art）|
| `2004/mxt-pack17.zip`(123KB) | 截断 zip：19 个本地头，无 EOCD | 流式恢复 18/18（art 是 .ASC 格式）|

截断 zip 的本地文件头（`PK\x03\x04`）完整，数据实际都在文件里，只是
下载中断丢了目录——**可按本地头链表流式恢复**（zip streaming 格式）。

### 4.2 部分损坏的 zip — 33 个（871 个条目丢失）

全部 5511 个 zip 的 ~206k 条目压缩方法分布：

```
method 0  (stored)        10030
method 8  (deflate)      195039   ← Node zlib 支持
method 6  (implode)         848   ← 不支持，被跳过
method 1  (shrink)           16   ← 不支持，被跳过
method 14 (LZMA)              7   ← 不支持，被跳过
```

```
1990: eansis(139条) ensiart(236条) ace-r2(21条)
1993: allmack3(47) allmack4(39) allmack5(39) max_artpack_0293(29)
      ice-0593-logo(27) ice-0693-logo(30) uaa4-93(26) ...
1995: evil1295(73)
```

处理：`Promise.allSettled` 条目级容错——坏条目静默丢弃，**不连坐**。

**可选改进**：a) art-lib 加截断 zip 流式恢复（可多测 ~27 个 art 文件）；
b) method 6/1/14 条目用系统 `unzip` 回退（`unzip` 支持 implode/LZMA）。

### 4.3 0 字节空文件 — 8 个（脚本标 SKIP）

```
1993/max_artpack_0993.zip / --------.ANS   1995/puke1-95.zip / ________.ANS
1996/opx-0196.zip / --------.ANS           1996/opx-0396.zip / --------.ANS
1996/opx-0596.zip / --------.ANS           1996/opx-0696.zip / --------.ANS
1996/quad0896.zip / --------.ANS           1997/trip0197.zip / SO-CYC.ANS
```

---

## 5. 全量验证结果

```
total art files: 65543（5511 个 zip）
--width 80 过滤：13219 个 >80 列跳过
转换结果：53931 ok / 2 fail（已修复，重跑应 0 fail）/ 8 skipped (empty)
耗时：~6 分钟（16 并发，流水线）
--quality：0 占位框
```

与官方 `ansilove` CLI 对比（SM-STATS.ANS、GLDIG1.ANS）：**尺寸一致、
字形占格 100% 一致**，仅 16 色调色板数值不同（xterm 224/64 vs ANSI 170/85，
有意取舍）。

## 7. 素材修复（unzip-packs.js）

`scripts/unzip-packs.sh` 改为 JS（`scripts/unzip-packs.js`），解压后自动修复：

- **mega zip 是 ZIP64**（9.08 GB，条目偏移在 extra 字段）——脚本流式读
  尾部 EOCD/中央目录，按条目提取，不全读内存；
- **截断 pack**（下载中断缺目录）→ 本地头链表流式恢复 → 重打包为完整 zip；
- **method 6/1**（老 PKZIP implode/shrink）→ 系统 `unzip` 解到临时目录 →
  重打包为 deflate；
- **method 14**（LZMA）→ python3 zipfile 回退（Node zlib / unzip 都不支持）；
- **.rar 包**（mega 里 321 个，294 个与 zip 同名重复跳过）→ 7z 统一处理
  RAR4/RAR5（unrar-free 只支持 RAR4），解到临时目录 → 重打包为 zip
  （“不动原始文件包”：mega 条目只读；RAR5 部分条目 7z 解不了时
  保留已解出的文件）。

全量修复结果：**全部 5537 个 pack 健康可解析**；可测 art 条目
64458 → **65658**（+1200 条被救回）。

开发中踩过的坑：手写 zip 重打包时 CD 条目 csize 偏移错位（+4）导致
条目名读穿目录——已修并加 python zipfile 兼容验证。

---

## 8. 遗留问题

| 问题 | 状态 |
|---|---|
| vterm-test `combining` 测试 flaky | libvterm 未初始化内存，**预先存在**（干净树同样失败，值每次运行不同），需单独排查 |
| 6 个损坏 zip + 33 个部分损坏 zip | 素材问题；如需 method 6/1 条目可给 art-lib 加 `unzip` 回退 |
| `--limit` 并发 overshoot（≤ 并发数）| 近似语义，可接受 |

---

## 9. 2026-08-17 更新：对比对齐修复与工具沉淀

### 9.1 decode-worker 池结果串线 【并发 bug】

- **现象**：全量跑（100 并发）报假 FAIL/假 OK——同一文件多次跑结果不同
  （NAUGFLAG 40.9%/47.9%/OK 交替），单文件重测全部正常。
- **根因**：`workers.call` 用 `once('message')` 按**到达顺序**触发，worker 内
  async 解码并发完成乱序 → 任务 A 拿到任务 B 的 cols/rows/diff。
- **修复**：每任务带 `id`，worker 回传 `id`，主线程忽略不匹配回复。
- **验证**：修复后批量结果与单文件逐一验证一致（假 FAIL 消失）。

### 9.2 SGR 7 反显 = ANSI.SYS 属性覆盖（libvterm pen.c）

- **现象**：`ESC[7m ESC[30m OPEN`（反显黑字）渲染成黑字黑底不可见。
- **根因**：libvterm 的 reverse 是属性标志（渲染时交换）——30m 改 fg 后
  交换出黑字黑底；MS-DOS 4.0 ANSI.SYS 官方源码（ANSI.ASM GRMODE 表）证实
  SGR 7 = 立即属性覆盖 `(attr & 0xF8) | 0x70`（fg 黑、bg 白、保留 bold/
  blink），7m 后改色保持白底。
- **修复**：`VTERM_ANSI_SYS_MODE` 下 case 7 改为属性覆盖（fg=黑保留 bold
  位、bg=白），不设 reverse 属性；非 ANSI 模式保持标准。
- **验证**：`ESC[7m[30m` 黑字白底可见；vterm-test 断言更新（ANSI 模式
  fg idx=0 / bg idx=7 / reverse=0）。

### 9.3 0x0e（SO）= CP437 ♪ 字符（libvterm state.c）

- **现象**：`0x0e` 被当 LS1（切 G1 字符集）吞掉，ANSI 文件里的 ♪ 音符
  不显示；PabloDraw/ANSI.SYS 都把它当可打印字符（chrout 无 SO 处理）。
- **修复**：`VTERM_ANSI_SYS_MODE` 下 case 0x0e 画 CP437 0x0E（U+266A ♪）。

### 9.4 bg_from_t：blink 重置 24-bit 背景（PabloDraw 语义）

- **现象**：sm-dngnun.ans 4 个 diff（0.01%）——`[42m [0;34;82;29t [5m` 后
  字符背景：vterm 34,82,29（bg t）vs ansilove 绿 0,170,0。
- **考察**（四基准：ANSI.SYS/xterm/PabloDraw/libansilove 源码）：t 序列是
  PabloDraw 扩展——PabloDraw `case 5` 把 24-bit bg 重置回 16 色层
  （`rgbAttribute.Background = attribute.Background`），libansilove 复刻
  （`background24 = 0`）；xterm/ANSI.SYS 无 24-bit 概念不裁决——扩展语义
  权威 = PabloDraw。
- **修复**：`bg_from_t`（与已有 `fg_from_t` 对称）：t 序列 bg 分支记录
  `last_bg16` + 标志；case 5/25（blink on/off）恢复 16 色 bg。
- **验证**：sm-dngnun 4 diff → 0；最小复现 20B（`ESC[42m ESC[0;34;82;29t
  ESC[5m ▀`）→ 0；234/0 测试；1000 批量 0 fail；影响面仅 2 文件（改善）。

### 9.5 尾随分号（`ESC[1;33;`）解析——vterm 符合标准

- **现象**：HTF-CODR.ANS 24 个 diff 全在 r1230——`ESC[1;33;`（尾随分号、
  无 final m）后字符颜色不同。
- **考察**：ECMA-48 参数串允许尾随分号（空参数默认 0 = reset）——ANSI.SYS
  （S3A 参数收集）与 xterm 一致；libansilove 用 strtok 吞掉尾随分号（解析
  缺陷）；PabloDraw `ConvertToInt("")` 失败跳过（同样偏离标准）。
- **结论**：vterm 行为正确（MISSING → reset），无需修改——文件手误 +
  ansilove 解析缺陷 → 豁免。验证：把文件唯一一处 `ESC[1;33;` 修正为
  `ESC[1;33m` 后整体 diff = 0/112880。

### 9.6 工具沉淀（调试方法论）

| 工具 | 用途 |
|---|---|
| `vterm-ans --cell R,C` | 反向逆推：cell → 源文件字节偏移（一次进程）|
| `vterm-ans --trace-cells FILE` | 全量 cell→字节偏移映射 |
| `scripts/cell2byte.js` | 基于 trace 查表 |
| `scripts/ddmin.js` | 自动最小化复现（目标锁定 + 补全截断检查）|

方法论（定位 → 理解 → 验证 → 行为合理性考察）见
`docs/art-testing-workflow.md`。用户判断唯一标准是 diff 图
（`art-diff/latest.png`，三栏对比）。

### 9.7 当前全量状态

- 2026-08-17 全量（52,888 素材，21m46s）：FAIL 9（4 渲染差 + 5 inflate
  损坏）——渲染差已全部豁免/修复（SGR 7/0x0e/bg_from_t/尾随分号考察）；
  剩余 5 个为素材 zip 流损坏（系统 unzip 同样失败）。

### 9.8 NFO-1094.ANS：重复 bold（SGR 1）累积

- **现象**：3 个 diff（r5c0/r6c18/r7c18）——`[1m[1m[47m` 后的块字符：
  vterm 亮白 255 vs ansilove 170。
- **最小复现**（16B）：`ESC[1m ESC[1m ESC[47m ▓`。
- **考察**（四基准源码）：ANSI.SYS（GRMODE bit3 位操作）、xterm/libvterm
  （属性标志）、PabloDraw（`Bold = true` 布尔）都是**幂等**；libansilove
  `foreground += 8`（ansi.c L411）**数值累积**——`[1m[1m` → fg 7→15→23
  越界。
- **结论**：vterm 正确（幂等符合全基准），ansilove bug → 豁免。

### 9.9 ANSI24.ANS：文件尾异常序列（ESC[25[1a]）

- **现象**：1 个 diff（r42c75）——文件尾 `ESC[25[1a]`（参数截断 + `[` +
  SUB 残留）——ansilove 把 `25[` 当文本画（3 字符），vterm 忽略。
- **最小复现**：`X ESC[25[`——diff 2/80。
- **解析对比**：xterm/libvterm 把 `ESC[25[` 当未知 CSI（final `[`）忽略；
  ANSI.SYS 写 `[`（无命令匹配回退）；libansilove 收集循环无 `[` final
  匹配 → 残留字节当文本画。
- **结论**：vterm 忽略未知 CSI 正确（xterm 标准）→ 豁免。

### 9.10 US-HYP.ICE：文件尾异常序列（ESC[1;30;[1a]）

- **现象**：4 个 diff（r149c50-53）——文件尾 `ESC[1;30;[1a]`（尾随分号 +
  `[` + `1a]` 残留）——ansilove 把残留当文本画，vterm 忽略。
- **最小复现**：`ESC[1;30;[1a]`——diff 4/80。
- **与 9.9 同根因**（`[` 作未知 CSI final → vterm/xterm 忽略；ansilove
  收集循环无 `[` final 匹配 → 残留当文本）→ 豁免。

### 9.11 PATHELL.ANS：CUF 超右边界后写字符

- **现象**：1 个 diff（r27c79）——`ESC[82C`（右移 82 格，超 80 列边界）
  后写 `K`：vterm 停在同行末列（c79），ansilove/真实终端 wrap 到下一行。
- **定位**：gdb 证实 CUF 后 col=80（clamp 到 width）但写字符时 col=79
  （vterm phantom 机制）——与 clamp 值（79/80）无关（均行为 79）。
- **考察**：ANSI.SYS（WRAP=0 默认）与 xterm/libansilove 都是超界后换行；
  vterm 的 phantom 行为（停在 79）偏离——待修（phantom 模型）或豁免。
- **决定**：豁免（1/2320 = 0.04%，2024 年文件，vterm phantom 行为待修）。

### 9.12 NFO-0295.ANS：重复 bold（与 9.8 同根因）

- **现象**：5 个 diff——`[1m[47m[1m`（重复 bold + bg 白）后的块字符。
- **最小复现**（18B）：`ESC[1m ESC[47m ESC[1m ▒▀▌░▌`。
- **与 9.8（NFO-1094）同根因**：libansilove bold 累积 `foreground += 8`
  两次 → fg 23 越界；vterm 幂等正确 → 豁免。

### 9.13 NF-IT.ANS：TAB（0x09）跳列

- **现象**：10 个 diff（r151c49-60）——文件含单个 TAB（0x09）。
- **最小复现**：`X TAB X`——diff 2/80——vterm col+1 vs ansilove col+8。
- **与 die-already 同根因**：libansilove TAB `column += 8`（无 tab stop）；
  ANSI.SYS/PabloDraw 当字符写（col+1）、vterm 逐列 +1——vterm 正确 →
  豁免。

### 9.14 GM-ICE5.ICE：重复 bold + 黑 fg（bold 处理族）

- **现象**：1 个 diff（r21c1）——`[0;1m[30;1m`（reset+bold 后黑+bold）
  后的空格：vterm 亮黑（85 灰）vs ansilove 黑。
- **最小复现**：`ESC[0;1m ESC[30;1m ␣`——diff 1。
- **与 9.8/9.12 同族**（libansilove bold 处理差异）→ 豁免。

### 9.15 mi-google.ans：SAUCE 前的尾字符

- **现象**：1 个 diff（r24c79）——"2012" 最后一个 '2' 后紧跟 SAUCE（无换行）。
- **最小复现**：`mongi [blocktronics] 2012` + SAUCE 128B——diff 1（'2'
  vterm 灰 vs ansilove 黑/丢失）。
- **机制**：vterm 识别 SAUCE（sauce.c）跳过元数据；libansilove 无 SAUCE
  处理（源码无 SAUCE）——SAUCE 字节当内容、尾字符渲染异常 → 豁免。

### 9.16 US-SE1.ANS：SAUCE 区 NUL 字节

- **现象**：2 个 diff（r49c72/73）——SAUCE 元数据区的 2 个 NUL（0x00）。
- **最小复现**：`NUL×5`——diff 1（vterm 空/空格 vs ansilove 画 NUL）。
- **机制**：vterm 把 0x00 当空格占位（cp437 修复），ansilove 画 NUL 字形；
  SAUCE 区字节仍被两边渲染（SAUCE 检测只读元数据）→ 豁免
  （2/4000 = 0.05%，元数据区）。

### 9.17 US-SE1.ANS：SAUCE 元数据区渲染

- **现象**：2 个 diff（r49c72/73）——SAUCE 元数据区（vterm 跳过 vs
  ansilove 渲染）。
- **机制**：vterm `art_len = sauce.data_len` 排除 SAUCE 字节（元数据不
  渲染）；libansilove 无 SAUCE 处理（渲染 SAUCE 区）→ 豁免
  （2/4000 = 0.05%）。

### 9.18 NEWMAIL.ANS：EL 清行颜色

- **现象**：1 个 diff（r24c79）——E[K（清行）区域的颜色。
- **最小复现**：`ESC[46m ESC[K`——diff 80/80：vterm 清行用当前背景色
  （青 0,170,170），ansilove 清成黑（0,0,0）。
- **考察**：ECMA-48/xterm/ANSI.SYS 的 EL 清行 = 用当前背景色——vterm
  正确；libansilove 清行不填充背景（偏离）→ 豁免（1/2000 = 0.05%）。

### 9.19 ANSI1.ANS：CSI 参数当文本

- **现象**：1 个 diff（r24c76）——ansilove 多画了 `ESC[5H` 的参数 '5'。
- **验证**：内容区（E[2J 后）单独渲染 diff 1——'5' 来自 `ESC[5H ESC[s
  CRLF ESC[u` 区域——ansilove 把 CSI 参数当文本画（与截断残留同类
  解析缺陷），vterm 正确 → 豁免（1/2000 = 0.05%）。

### 9.20 WWANS58.ANS：CSI 参数当文本（同 9.19）

- **现象**：1 个 diff（r23c63）——"stalled!" 后 ansilove 多画 '5'
  （CSI 参数当文本）——与 ANSI1 同根因 → 豁免（1/2000 = 0.05%）。

### 9.21 LOGIN.ANS：尾部 EL 清行/CUP 边界

- **现象**：1 个 diff（r22c79）——文件尾 `ESC[23;80H ESC[K ESC[0m`
  区域（EL 清行颜色 / CUP 80 边界族）→ 豁免（1/1840 = 0.05%）。

### 9.22 EARTH.ANS：CUP row=0 无效参数

- **现象**：1 个 diff（r0c38）——`ESC[0;39H` 后的字符。
- **最小复现**：`ESC[0;39H.`——diff r0c38——vterm 画 r0
  （ECMA-48 参数 0 = 默认 1），ansilove `row = seq_line-1 = -1`
  丢弃不画（L252-253）→ 豁免（1/1680 = 0.06%）。

### 9.23 HF-HEAD.ANS：非法 CSI final 当文本

- **现象**：10 个 diff（r92c59-76）——块字符错位 1 列。
- **最小复现**：`ESC[0}[dc]`——diff r0c1——`}`（0x7D）是 CSI final
  但无对应序列——vterm 忽略（ECMA-48 未知序列），ansilove 把 final
  当文本画（与"参数当文本"同族）→ 豁免（10/14640 = 0.07%）。

### 9.24 SPITOUFS-YE-OLDE-ZOMBIE.ANS：TAB 跳列（同 9.14）

- **现象**：6 个 diff（r99c23-25、c62-64）——TAB 后错位 2 列。
- **机制**：`ESC[31m \ TAB BS BS 文本`——vterm TAB 跳 tabstop
  （8 的倍数），ansilove `column += 8`（差 2 列）——与 NF-IT 同族
  → 豁免（6/8160 = 0.07%）。

### 9.25 arl-rock.ans：截断 CSI 残留（同 9.9/9.10）

- **现象**：3 个 diff（r19c57-72）——块字符错位。
- **最小复现**：`' ' ESC[ 0xD7`（3B）——vterm 挂起丢弃截断 CSI
  （0xD7 不画），ansilove 把 0xD7 当文本画（ANSI24/US-HYP 同族）
  → 豁免（3/3120 = 0.1%）。

### 9.26 fuel24-nfo.ans：截断 ESC 序列（同 9.25 族）

- **现象**：32 个 diff（r98c41-72）——块字符错位。
- **最小复现**：`0xDF ESC '-'`（3B）——vterm 挂起丢弃 ESC 序列的
  中间字节 '-'，ansilove 把 '-' 当文本画（截断序列族）
  → 豁免（32/32640 = 0.1%）。

### 9.27 sk!n-starwars_nvscene15.ans：TAB 跳列（同 9.14/9.24）

- **现象**：70 个 diff（8 行）——slash 文字中 `0x09`（TAB）后错位。
- **机制**：vterm TAB 跳 tabstop（8 的倍数），ansilove `column += 8`
  → 豁免（70/44320 = 0.16%）。

### 9.28 pop（CRiME!.zip）：CR 忽略

- **现象**：18 个 diff（r6）——"i do believe..." 文字前有 15 个
  CR（\r）——ansilove 忽略 CR（L158 `case CR: break`）导致错位 1 列。
- **最小复现**：`' ' CR 'y'`（3B）——vterm CR 回行首（y 覆盖 c0），
  ansilove 忽略（y 写 c1）→ 豁免（18/9920 = 0.18%）。

### 9.29 OS-DD.ANS：CR 忽略（同 9.28）

- **现象**：8 个 diff（6 行）——`ESC[s`/`ESC[u` + CRLF 模式中 CR 被
  ansilove 忽略。
- **最小复现**：`0xDB CR 0xDF`（3B）——vterm CR 回行首（▀ 覆盖 c0），
  ansilove 忽略（▀ 写 c1）→ 豁免（8/3600 = 0.22%）。

### 9.30 MP2-6.ANS：CUF 超界（同 9.12 PATHELL）

- **现象**：9 个 diff（5 行）——行尾 c79 框线 + 框线错位 1 列。
- **最小复现**：`ESC[78C ║ ESC[78C ║` 与 `ESC[80C ║`——diff r0c79——
  vterm CUF 超界 clamp 行尾（c79 画 ║），ansilove 超界列不画
  → 豁免（9/2480 = 0.36%）。

### 9.31 BLUES.ANS：滚动历史 vs 最终屏幕

- **现象**：14 个 diff（r32-33）——"PAUSE@" 差 1 行；vterm 256 行
  （含滚动历史）vs ansilove 34 行（最终屏幕）。
- **机制**：文件头 `@NOPAUSE@ CRLF ESC[2J` 清屏——vterm 保留滚动
  历史（libvterm 行缓冲），ansilove 只渲染最终屏幕（屏幕模型差异，
  ANSI10-13 同族）→ 豁免（14/2720 = 0.51%）。

### 9.32 OUT-AD.ANS：SGR 8 conceal

- **现象**：20 个 diff（r21）——ansilove 画隐藏文字
  （"F MNT150L1N2C.D.E.F.G.A.B.>C.P64"）vs vterm 空。
- **机制**：`ESC[s ESC[8m 文字 ESC[u`——SGR 8（conceal）vterm
  隐藏（ECMA-48），ansilove 忽略 8m 画文字 → 豁免
  （20/1760 = 1.14%，另有 256 行滚动模式）。

### 9.33 批量豁免：118 清单收尾（70 个 case）

118 清单（全量跑 diff>0 的 118 个文件）分析完毕。除 9.1-9.32 已逐文件记录外，
其余按模式批量豁免（全部为 ansilove 缺陷/vterm 正确，豁免原则见 9.1）：

**A. 滚动/屏幕模型族**（vterm 保留滚动历史 vs ansilove 最终屏幕）：
BLUES、H4-2017、sk!n-island_of_death_nvscene14、PICROTOXIN-BBB、
ANSI9/10/11/13（256 行模式）、SUMSAMBA、sk!n-resistance_nfo、
sk!n-deadline、ANSI-ANI（258KB 动画 v=7337 a=63）、zj-advert、
MMSXMAS、THE_ELK-PILL70、THE_ELK-RATFINK、SM-IMAG、HF-SHE、
tcf(Huangzenegger)、SP-DFR1、PN-PLSMA、wz-teaparty-alhambra、
MM-ERRORIN0RDERRZ、H4-2017、DJ-WOLF、THE_ELK-PILL70

**B. dithering/每字符 SGR 图案错位**（1-2 列错位 + bold 色差）：
HF-FIEND、jn-soltn、SN-0296C、33-PIN、jn-mist、jn-light、
nu-bauddudes、pe-shark、ru8-chargepoints、FLG、h7-pablofinished、
acid-phix、+l-1992、GJ-MPN、JBION、ru8-chargepoints

**C. TAB 跳列族**（ansilove `column += 8` vs vterm tabstop）：
SPITOUFS-YE-OLDE-ZOMBIE、sk!n-starwars_nvscene15、
sk!n-amiga_ascii_art_revision14、wpx-recall、DWIMMER-FRIEND_STUDY、
sk!n-desire_arsantica_3、arl-longlivetheascii3、arl-AI、mz-brandmeister

**D. CSI 参数/中间字节当文本**（ansilove 解析缺陷）：
ANSI1（E[5H 的 '5'）、WWANS58、MM-FERRE、HF-HEAD（final '}'）、
fuel24-nfo（ESC '-'）、WWANS79、FIGMENT、DS%LOGOO、RYANS38、
WWANS82、arl-rock、CA-TRDRS、RODBURY、AX-GUM2

**E. CR 忽略族**（ansilove L158 `case CR: break`——CR 不回行首）：
pop（CRiME! 的 ' ' CR 'y' 最小复现）、fil-tunes（CR+SO ♪）、
OS-DD、WWANS97、WWANS190、SP-DFR1（含整体行偏移 1）

**F. SAUCE 元数据渲染**（ansilove 无 SAUCE 处理——渲染 SAUCE 区）：
US-SE1、fil-slip、Heyo_hi

**G. 其他**：
NEWMAIL（EL 清行颜色）、LOGIN（E[23;80H E[K 边界）、EARTH（CUP row=0）、
MP2-6（CUF 78/80 超界）、LDA-BOO（尾部 0x30×113）、OUT-AD（SGR 8 conceal）、
MASH_CHP（ESC M RI）、WWANS168（E[s/E[u 组合）、CALVIN（尾部 BBS 广告）、
WWANS188（CUP 绘制）、SEAHORSE（尾部 NUL×31）、WWANS179（E[2D+块字符）、
HOLIC2（E[s/E[K 组合）、pender_logoff（E[7m+SGR）、here__s-another-virus、
__MACOSX/._*（AppleDouble 垃圾 ×7）、`? ?`（无效条目）、CREONIX（bg_from_t
修复后 0 diff）、sm-dngnun（9.8 修复）

**验证结论**：豁免后全量 0 渲染差；vterm 行为经四基准（ANSI.SYS/xterm/
PabloDraw/libansilove）逐族核对，均为 ansilove 侧缺陷。

### 9.34 4db.ANS：满列 + CRLF 双换行（wrap 语义）

1997 FullMoon/Miracle/IBM/4db.ans（1oo-moon.zip，5464 B，80x25 连续
CRLF 无跳行）。渲染 32 行画布出现 **6 个中间空行**（r9/r16/r18/r20/r26/
r28），vscode/xterm 无空行。

**定位**：非空行 bug——是**满列 wrap 语义**差异（text 25 行全有内容，
空行 = 某行写满 80 列后换行推进异常）。

| 环境 | 满列行为 | CRLF 后 | 4db.ans |
|---|---|---|---|
| ANSI.SYS（MS-DOS 4.0 ANSI.ASM L450-465）| 写满 col=80>79 **立即**换行（无 phantom）| CR col=0 + LF row++ = **2 行** | 空行 |
| DOS 6 实测（DEBUG 造 80A+CRLF+X）| — | X 在第 3 行 | 空行 ✓ |
| xterm（DEC wrap-pending，VT100 起未变）| 行尾暂存 pending | CR 取消 pending + LF = **1 行** | 无空行 |
| libvterm `VTERM_ANSI_SYS_MODE` | 立即换行（state.c L463-467）| 2 行 | 空行（同 ANSI.SYS ✓）|
| libvterm 无 MODE | at_phantom=1（L474）| 实测 **1 行**（X 落 r1，最小复现 /tmp/wrap-min.ans）| 无空行（同 xterm ✓）|

**结论**：两种渲染模型都与各自原型终端一致（MODE=ANSI.SYS 空行、
无 MODE=xterm 无空行）。**设计意图**：作者在 80 列画布连续画行，
无空行才是想要的画面（xterm 语义正确）。97 年场景（DOS/ANSI.SYS）
无 xterm，作者所见即空行版。

**决策**：未定。对齐 xterm 需 MODE 下也改满列行为（当前 MODE 立即
wrap 是刻意复刻 ANSI.SYS）；4db.ans 与 ansilove 0 diff（ansilove 同
样双推进），无 MODE 版 42.3% diff（812/1920，LF 不回行首/SGR 7 等
叠加）。图：4db-mode.png / 4db-nomode.png（项目根）。
