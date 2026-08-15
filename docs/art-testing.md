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
