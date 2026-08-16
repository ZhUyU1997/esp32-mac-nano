# ANSI art 对比验证工作流程

目标：`vterm-ans` 渲染结果与 **libansilove（对比参考）**一致、**内容完整**
（不滚动/不截断）、符合 **ANSI.SYS（语义基准）**。

## 工具链

| 工具 | 用途 |
|---|---|
| `vterm-ans` | ANSI art → PNG（渲染器）|
| `scripts/test-art.js --compare` | 批量对比（vs ansilove），逐文件 rate + 豁免 |
| `scripts/compare.js one` | 单文件对比 → 项目根 `compare.png`（三列：ansilove \| vterm-ans \| diff map，带行号）|
| `scripts/compare.js list` | 按 `list.txt` 批量跑（解压到临时目录 → test-art.js）|
| `scripts/diff-lib.js` | 共享 diff 实现（test-art 与 compare one 同源）|
| `scripts/art-lib.js` | 共享工具（zip、SAUCE、键盘脚本检测）|
| `docs/ansi-sys-compat.md` | 全部修复记录 |

## 标准循环

```
1. 批量跑      node scripts/compare.js list > compare-output.txt
2. 提取 FAIL   → list.txt（pack/entry）
3. 单个检查    node scripts/compare.js one <pack> <entry>  → compare.png
4. 用户看图    判断差异类型（偏移/颜色/缺失/乱码）
5. 二分定位    按行/字节切前缀渲染对比 → 找触发点
6. 修复        vterm-ans / libvterm / cp437 / sauce / 对比工具
7. 回归        之前修复的文件全部 0.0%（防回退）
8. 记录文档    docs/ansi-sys-compat.md
9. 重跑批量    看 FAIL 降幅 → 回到 3
```

## 决策原则

- **不擅自动手**：豁免/下一个/修复都要**用户确认**。
- **参考基准**：libansilove（对比）+ ANSI.SYS（语义）——**修复优先于对齐**
  （内容完整 > 模仿参考）。
- **豁免需用户确认**：libansilove bug 类（`\x1b[A` 失效、CR 忽略、TAB 越界、
  SAUCE 漏检）、文件损坏类、非 art 类——加进 `test-art.js` 的 `EXEMPT_FILES`。
- **二分是默认定位方法**：不盲目分析，让用户看图判断。

## 已修复问题类别

| 类别 | 内容 |
|---|---|
| ANSI.SYS 语义（宏 `VTERM_ANSI_SYS_MODE`）| 右边界立即 wrap、CUF 钳制、ED2 归位、LF 隐含 CR |
| 滚动（`vterm-ans.c` `scan_max_row`）| CUP row 修正、CUU 不减、CUF wrap 模拟、字符列推进、rows 余量 128 + cap 8192 |
| CP437（`cp437.c`）| C0 区 27 字形、0x7F ■、0x00 空格占位 |
| SAUCE（`sauce.c`）| 字段偏移（version 双布局）、cols 校验 40..200、title 空回退、version 只认 "00" |
| 颜色 | 默认前景 7 号、CGA 调色板（pen.c）、PabloDraw 24-bit（`t` 序列）、ED2 默认背景（screen.c）|
| 豁免（用户确认）| 键盘脚本、损坏序列、SAUCE 宽度差、ansilove 自身失败 |

## 当前状态

- 全量对比上次在 8/15 跑（FAIL 数未更新）；后续修复已验证：
  旧日志前 240 个 FAIL 样本重跑全部转 OK（0 fail）
- `list.txt`（batch 输入）当前不存在，需要时重建
- 豁免名单（`EXEMPT_FILES`，libansilove bug / 损坏 / 非 80 列）：MEM0595 /
  PK-NUCW / AL-DTD / DG-MAKC2 / -------- / US! / cm-MIST /
  sk!n-abstrakt_nfo_fb / sk!n-motiv8_logo_ansi / mz-piece / grx-comp2 /
  grx-comp7 / ldn-vandalism / +l-ds / g80-hmm / ru8_factory
  （`EXCLUDE_FILES`，非 80 列固定设备无法显示）：fool27.zip-file_id /
  mist0918.zip-FILE_ID / impure84.zip-lmn-siouxie / mist0823.zip-MM-ONE
