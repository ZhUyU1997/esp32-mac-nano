# XT 终端模拟器（tools/vterm）— 字形/宽度/渲染体系

独立的 XT 时代终端模拟器模式（与 macplus 模拟器无关），host 端 x64 SDL
验证，最终目标 ESP32-S3（480×640 竖屏 60×40）+ Telnet。

## 1. 架构

```
pty/bash ──▶ libvterm（vendored, neovim fork v0.3.3）
                │  解析终端序列、状态机、cell.width（布局宽度）
                ▼
           term_render.c —— 字形渲染（按 cell 分流）
                │
                ▼
           帧缓冲（8×16 点阵格子）──▶ SDL 窗口 / BMP
```

- **libvterm**：vendored（`libvterm/`），负责终端协议、光标、滚动、宽度。
- **term_render**：`tools/vterm/term_render.c`，把 cell 渲染成 8×16 点阵。
- 两个 target：`vterm-test`（测试）、`vterm-sdl`（交互）。

## 2. 字形体系（4 个字形源）

| 字形 | 数据 | 生成 | 字体源 |
|---|---|---|---|
| CP437/ASCII/框线 | `vga8x16.h`（256×16 字节，内嵌）| 静态 | Linux kernel font_8x16（GPL-2.0）|
| 符号表（1125 个）| `symbol_glyphs.h` | `scripts/gen_symbols.py` | `tools/vterm/fonts/unifont.hex` |
| emoji（79 个）| `emoji_glyphs.h` | `scripts/gen_emoji.py` | `tools/vterm/fonts/noto-emoji-mono.ttf` |
| CJK（6886 个）| `unicode_glyph.h` | `scripts/gen-hzk16.py` | `tools/vterm/HZK16` |

字体源放在 `tools/vterm/fonts/`（**不提交 git**——`.gitignore` 的 `fonts/` 规则）。
克隆后需手动放置再重新生成：

```
mkdir -p tools/vterm/fonts
# GNU Unifont（GPL-2.0 WITH FONT EXCEPTION）
wget https://unifoundry.com/pub/unifont/unifont-15.1.05/font-builds/unifont-15.1.05.hex -O tools/vterm/fonts/unifont.hex
# Noto Emoji monochrome（OFL）
# 从 https://github.com/googlefonts/noto-emoji 的 fonts/NotoEmoji-Regular.ttf 获取
```


## 3. 宽度体系（布局宽度 = xterm 标准）

**权威来源**：glibc `wcwidth()`（xterm 链接的 libc，C.UTF-8 locale）。
用 `scripts/wcwprobe.c` 全量测量 1,114,043 个码点，生成三张数据表：

| 表 | 内容 | 生成 |
|---|---|---|
| `libvterm/src/fullwidth.inc` | 双宽区间（126 区间）| `scripts/gen-width-tables.py` |
| `libvterm/src/combining.inc` | 零宽组合字符（345 区间）| 同上 |
| `libvterm/src/false_zero.inc` | Kuhn 表误判例外（10 个）| 同上 |

`vterm_unicode_width()` 只有 4 步，无特判堆积：

```
combining.inc（零宽）→ 0
fullwidth.inc（双宽）→ 2
false_zero.inc（Kuhn 误判）→ 1
fallback mk_wcwidth（Kuhn 经典）→ 1 / -1
```

**验证**：全 Unicode 1,114,043 字符 vs glibc wcwidth **0 mismatch**；
xterm 终端 CPR 实测抽检一致。所有码点宽度与 xterm 完全一致。

## 4. 渲染规则（布局与渲染解耦）

**布局宽度**（光标推进/换行/选择）= xterm 标准（第 3 节）。
**渲染宽度** = 字形源的自然宽度，比例不变：

```
固定高度：全部字形 16px（16 行点阵）
自然宽度：8×16 源 → 8px；16×16 源 → 16px（最宽 16px，16px 截断）
不缩放：字形 = 源原样（unifont/NotoEmoji/HZK16 提取，不做 up2x/shrink）
```

渲染分流（paint_cell）：

```
空白 → 空（宽字形溢出格不覆盖）
CP437 命中 → 8px
CJK（布局 2 格）→ 16×16
emoji → 16×16（布局 1 或 2 都渲染 16px）
符号表 → w=1: 8×16 / w=2: 16×16
未映射 → 占位框（按布局宽度 1 或 2 格）
```

**"画 2 格坐标动 1 格"**：16×16 源符号（☑● 等）布局 1 格（xterm 宽度），
渲染 16px 溢出到下一格；下一格是空格时不覆盖（wide_occ 保护），
☑ 完整显示；连续符号右半互盖（预期行为）。

## 5. 生成 / 验证链路

```
# 字形表
python3 scripts/gen_symbols.py      # unifont → symbol_glyphs.h
python3 scripts/gen_emoji.py        # NotoEmoji → emoji_glyphs.h
python3 scripts/gen-hzk16.py        # HZK16 → unicode_glyph.h（CJK）

# 宽度表（需要 glibc wcwidth 数据）
gcc -O2 -o /tmp/wcwprobe scripts/wcwprobe.c
python3 -c 'print all cps 0..0x10FFFF' ... | /tmp/wcwprobe > w.txt
python3 scripts/gen-width-tables.py w.txt   # 三个 .inc

# 验证
python3 scripts/verify-glyphs.py    # 字形 vs 字体源一致性（1204 条）
xmake run vterm-test                # 176 断言（渲染一致性 + 布局 + 序列）
python3 scripts/gen-char-table.py   # docs/char-table.png（SOURCE vs RENDERED 对照）

# 宽度对比（xterm 标准）
/tmp/cmp3 全 Unicode 0 mismatch
```

## 6. 已知取舍

- 宽度以 glibc 2.35（Unicode 14 级 EAW）为准；换系统需重跑 gen-width-tables。
- C.UTF-8 下 Ambiguous 判 1（xterm 默认）；CJK locale（LANG=zh_CN）会判 2，
  与 xterm 的 locale 行为一致，本项目固定 C.UTF-8 语义。
- CJK 用 HZK16（GB2312 子集 6886 字）；超出 HZK16 的汉字显示占位框。
- emoji 79 个为常见 TUI 图标（lazygit/starship 常用），其余显示占位框；
  补符号只需往 `scripts/gen_emoji.py` 的列表加字符后重新生成。
