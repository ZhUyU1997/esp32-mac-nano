# vterm 渲染性能优化 — 实验总结

## 环境与测量方法

- 硬件：ESP32-S3 + ST7701S（480×640 RGB 面板，PSRAM framebuffer，RGB222 数据总线）
- 负载：`scripts/telnet_bash_srv.js 2324 --test fill` 每 500ms 灌一屏 80×30 文本
- 采样：`VTERM_PROFILE` 每秒打一行 `prof: render N/s avgXus | blit N/s | dmg N ev avg M cells`
- 每个实验 build + flash + 读 prof，结果记录在本文件

## 数据总表（fill 全屏，最坏情况）

| 实验 | render | blit | 总帧成本 | 相对基线 |
|---|---|---|---|---|
| 基线（逐字节 blit + 全量 render） | 165ms | 329ms | 494ms | 1× |
| #1 blit 按 uint32 打包 | 125ms | 329ms | 454ms | 1.1× |
| #2 blit 16×16 分块转置 | 125ms | 48ms | 176ms | 2.8× |
| #3 现场绘制（消除 s_pixels） | 134ms | 0 | 134ms | 3.7× |
| #4 量化色值提升 | 69ms | 0 | 69ms | 7.2× |
| #5 bg memset + 光标判断提出 | 45ms | 0 | 45ms | 11× |
| #6 脏矩形 | 45ms（全屏）/ **<1ms（稀疏）** | 0 | — | — |

## 最终架构

```
libvterm 解析 → 脏矩形 bbox (dmg_add) → 只重画 bbox 内 cell
                                     → 每个 cell: bg memset + glyph 逐像素写入 16×16 栈缓冲（转置）
                                     → flush_cell_fb: 16 字节线性 run 直写旋转后的 fb
（无 s_pixels 中转，无全帧 blit；鼠标指针 save/restore 叠加在 fb 上）
```

---

## #1 blit 按 uint32 打包 — 无效，但定位了真瓶颈

假设：blit 慢在逐字节写 fb。改成一次写 4 像素（对齐 Mac 的 `blit_mac_mono_to_lcd_rgba`）。

```c
/* 改前：逐字节 */
for (int dst_x = 0; dst_x < w; dst_x++) {
    uint8_t c = s_pixels[(size_t)dst_x * src_w + src_col];
    fb[(size_t)dst_y * lcd->width + dst_x] = (uint8_t)(c << 2);
}
/* 改后：uint32 打包 */
for (int dst_x = 0; dst_x < w; dst_x += 4) {
    uint32_t v0 = (uint32_t)(src[(dst_x+0)*src_w] << 2);
    uint32_t v1 = (uint32_t)(src[(dst_x+1)*src_w] << 2) << 8;
    ... v2, v3 ...
    *dst_u32++ = v0 | v1 | v2 | v3;
}
```

结果：blit 不变（329ms），render 顺带 165→125ms。

结论：**blit 是读-bound，不是写-bound** —— 逐列读 s_pixels（stride 640）打穿 32KB data cache，每次读 ~1µs PSRAM 延迟。

---

## #2 blit 16×16 分块转置 — blit 7×

根因（#1 发现）：转置的逐列读（stride 640）cache 抖动。改成 16×16 块，读和写都变线性 16 字节 run。

```c
uint8_t blk[16][16];
for (int by = 0; by < src_h; by += 16) {
    for (int bx = 0; bx < src_w; bx += 16) {
        /* 读 16×16 块（行内线性），转置进栈缓冲 */
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++)
                blk[x][y] = s_pixels[(size_t)(by + y) * src_w + (bx + x)];
        /* 写出去：fb(dst_x=by+y, dst_y=639-bx-x)，16 字节 run + uint32 打包 */
        for (int x = 0; x < 16; x++) {
            uint32_t *row = (uint32_t *)(fb + (size_t)(fb_h - 1 - bx - x) * fb_w + by);
            row[0] = pack4(blk[x][0], blk[x][1], blk[x][2], blk[x][3]);
            row[1] = pack4(blk[x][4], blk[x][5], blk[x][6], blk[x][7]);
            row[2] = pack4(blk[x][8], blk[x][9], blk[x][10], blk[x][11]);
            row[3] = pack4(blk[x][12], blk[x][13], blk[x][14], blk[x][15]);
        }
    }
}
```

结果：blit 329→48ms。

---

## #3 现场绘制（on-the-fly）— 消除 s_pixels 中转

把 render 和 blit 合并：每 cell 渲到 16×16 栈缓冲（转置），直接写旋转后的 fb。s_pixels（300KB PSRAM）只在 wide-glyph 溢出时回退用。

关键：旋转映射 `fb(dst_x, dst_y) = s_pixels(px=dst_x, py=639-dst_y)`，即 cell 内像素 `(gx, gy)` 落到 `s_cell[gx][gy]`，flush 时写 `fb[(fb_h-1-px0-gx)*fb_w + (py0+gy)]`。

```c
/* rp_put：写栈缓冲（转置），不再写 s_pixels */
static inline void rp_put(term_renderer_t *r, int px, int py, uint8_t q)
{
    if (r->fb_out)
        s_cell[px - s_cell_px0][py - s_cell_py0] = (uint8_t)(q << 2);
    else
        r->pixels8[py * r->win_w + px] = q;
}

/* 每 cell 渲完 flush：16 字节线性 run 直写 fb */
static void flush_cell_fb(term_renderer_t *r, int w)
{
    for (int gx = 0; gx < w * TERM_CELL_W; gx++) {
        uint32_t *row = (uint32_t *)(r->fb_out +
            (size_t)(r->fb_h - 1 - s_cell_px0 - gx) * r->fb_w + s_cell_py0);
        row[0] = fb_pack4(s_cell[gx][0..3]);
        row[1] = fb_pack4(s_cell[gx][4..7]);
        row[2] = fb_pack4(s_cell[gx][8..11]);
        row[3] = fb_pack4(s_cell[gx][12..15]);
    }
}
```

结果：blit 彻底消失（0ms）。总帧 176→134ms。s_pixels 的写（render 侧）+ 读（blit 侧）都没了。

---

## #4 量化色值提升 — paint 2.6×

发现：paint 里每个像素都在调 `rgb888_to_64`（6-bit 量化）。同一个 cell 的 on/off 色是常量，量化一次即可。

```c
/* cell_style 末尾：一次量化，随 style 一起缓存 */
*on_q = rgb888_to_64(*on_b);
*off_q = rgb888_to_64(*off_b);
...
s_cache_on_q = *on_q;  /* 缓存，bg/glyph 两个 pass 复用 */

/* rp_put 改收预量化值，逐像素的 rgb888_to_64 消除 */
rp_put(r, px0 + gx, py0 + gy, on_q);   /* 原来是 on_b */
```

结果：paint 111→42ms（render 134→69ms）。逐像素量化是 paint 的大头。

---

## #5 bg memset + 光标判断提出循环 — paint 2.3×

bg 是纯色，128 次逐像素写换成单次 memset（纯色转置是 no-op）；glyph 无光标时的判断提到循环外。

```c
/* bg：fb 模式一次 memset */
if (r->fb_out) {
    memset(s_cell, (int)(off_q << 2), (size_t)w * TERM_CELL_W * TERM_CELL_H);
} else { /* s_pixels 回退路径仍是循环 */ }

/* glyph：无光标时内层循环只剩位判断 */
bool cursor_underline = cursor_here && !cur_block && r->cursor_shape == UNDERLINE;
bool cursor_bar       = cursor_here && !cur_block && r->cursor_shape == BAR_LEFT;
for (gy) {
    if (cursor_underline || cursor_bar) { /* 光标带（少见） */ }
    else for (gx) if ((line >> (7 - gx)) & 1) rp_put(..., on_q);  /* 快路径 */
}
```

结果：paint 42→18ms（render 69→45ms）。flush（fb 写）19ms 是 fill 全屏的硬地板。

---

## #6 脏矩形（dirty-rect）— 交互质变

fill 全屏是 worst-case，真实打字/提示符一次只改几个 cell。累积 damage bbox，只重画 bbox。

```c
/* 累积 bbox（vterm_esp32.c） */
static void dmg_add(int r0, int r1, int c0, int c1) {
    /* clamp + 扩展 s_dmg_r0..c1 */
}

static int vterm_cb_damage(VTermRect rect, void *user) {
    s_dirty = true;
    dmg_add(rect.start_row, rect.end_row - 1, rect.start_col, rect.end_col - 1);
    return 1;
}

static int vterm_cb_movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user) {
    s_renderer.cursor = pos;
    s_dirty = true;
    dmg_add(oldpos.row, oldpos.row, oldpos.col - 1, oldpos.col + 1); /* 块光标是反显，旧/新格都要重画 */
    dmg_add(pos.row,    pos.row,    pos.col    - 1, pos.col    + 1);
    return 1;
}

static int vterm_cb_moverect(VTermRect dest, VTermRect src, void *user) {
    return 0;  /* 不认领滚动 → libvterm 报滚动区 damage */
}

/* 渲染侧（term_render.c）：bbox 有效则只画 bbox */
const bool partial = r->dirty_r0 <= r->dirty_r1;
const int row0 = partial ? r->dirty_r0 : 0;
const int row1 = partial ? r->dirty_r1 : r->rows - 1;
const int col0 = partial ? r->dirty_c0 : 0;
const int col1 = partial ? r->dirty_c1 : r->cols - 1;
for (row = row0; row <= row1; row++)
    for (col = col0; col <= col1; col++) { ... 渲 cell + flush ... }
```

配套修正：指针 restore 提前到渲染前（部分渲染可能不覆盖指针区域）；鼠标选择/scrollback 标记全帧。

结果：bash 提示符（39 格）805µs，单次打字几十 µs；全屏 fill 仍 45ms（bbox=全屏）。

---

## 剩余可优化点（未做）

- **flush 19ms**：fb 写-bound（307K 像素，LCD DMA 争用），只能靠双缓冲（用户已否）或脏矩形列合并。
- **paint 18ms**：`vterm_screen_get_cell`（libvterm 内部 cell 拷贝）+ cell_style + glyph 循环，递减收益。

## #7 pi -c 卡顿定位 —— bench 隔离 + drain 日志

真机 `pi -c` 无响应 73s。用 selftest 里注入基准（免 flash 由 host telnet `--test burst:<kind>:<mb>` 发数据）+ drain 计时逐层隔离：

| 层 | 速率 | 定位 |
|---|---|---|
| 滚动 memmove（PSRAM 115KB 整块）| 6.3 KB/s | 当前瓶颈 |
| 网络（telnet 逐字节 push）| 128 KB/s | 次瓶颈 |
| 纯解析+写 cell（无滚动）| 1.08 MB/s | 上限 |

**根因**：libvterm 屏幕缓冲（30×80×~48B≈115KB）是一整块分配 → 落 PSRAM（内部 RAM 仅剩 38KB）。每次 LF 到底行触发滚动 = `moverect` memmove 29 行 ≈ 111KB PSRAM→PSRAM（6.3ms/次）。pi -c 回放 session 历史 ≈ 1.46MB / 1.1 万行 → 1.1 万次滚动 ≈ 66s。

`parse 73s` 里 99% 是这个 memmove（2026 同步输出期间不渲染，纯滚动成本）。

## #8 滚动环形缓冲（ring）—— 6.3→131 KB/s

改 vendored libvterm `screen.c`：屏幕从“单块扁平数组”改为“行指针数组 + 环形偏移”，`getcell` 用 `(row+offset)` 映射，全屏纵向滚动只转 offset（O(1)）+ 清露出的行。

- `ScreenCell **buffers[2]` + `int buffer_offsets[2]`；`moverect_internal` 全屏竖向 fast path（`dest/src` 全宽且覆盖全屏时转 offset）。
- 部分宽/水平滚动、resize 仍走 memmove 回退（正确性）；resize 前先把旧缓冲指针旋转归零（O(rows)，冷路径）。

结果（host `vterm-test` 177/177 通过）：

| 基准 | 改前 | 改后 |
|---|---|---|
| ascii 滚动 | 445 ms | 50.6 ms |
| sgr 滚动 | 717 ms | 37.2 ms（→131 KB/s）|
| sgr 无滚动（上限）| 447 ms | 455 ms（不变）|

## #9 网络批量 push + getcell 取模消除 —— 128 KB/s→不再瓶颈

- `telnet_push` 从逐字节（每字节一次 mutex）改为批量 memcpy（按非 IAC 连续段推），环满退避 `vTaskDelay(0)`→`vTaskDelay(1)`（100Hz 下原 0 tick=热自旋）。
- `getcell` 的 `(row+offset)%rows`（运行时除法，无硬件除法器很贵）改为 `r>=rows ? r-rows : r`（offset/row 都 < rows，一次减法足够）。
- `erase_internal` 提升 getcell + 每行预建 pen（避免逐 cell 位域 RMW）。

结果：网络等待从 drain 的 ~50% 降到 ~1%（burst 2MB：host 118 KB/s，ESP32 parse 118 KB/s，parse 成为新瓶颈）。

**当前剩余瓶颈**：滚动“清露出的行”= 80 格 × 48B PSRAM 写 ≈ 0.33ms/次 → parse 卡 ~131 KB/s。pi -c：73s → ~12s。要进一步提速需把 erase 改成 memcpy 预建空行（或屏幕迁出 PSRAM，但 115KB 放不进 38KB 内部 RAM）。

## 栈用量待实测（macplus_task 10→24KB）

`vterm_esp32_enter()` 在 **macplus_task 内**调用（main.c:402），非独立任务，栈叠加在 Musashi 之上。10→24KB 是开发时保险值，无实测依据。

**vterm 的栈消耗点：**

| 消费点 | 大小 | 位置 |
|---|---|---|
| `tbuf[1024]` telnet drain 缓冲 | 1 KB | vterm_esp32.c:745 |
| `blk[16][16]` blit 转置缓冲 | 256 B | vterm_esp32.c:403（仅 wide-glyph 溢出回退）|
| `VTermScreenCell cell` 局部 | ~64 B/个 | term_render.c 多处 + libvterm screen/state |
| libvterm 解析链 | 帧深×每帧几十~几百 B | input_write→parser→screen 回调→frame_fb→render_cell |

`buf[512]`（vterm_telnet.c:192）不算 —— 在独立 `telnet-cli` 任务（4KB 栈）。

`CONFIG_FREERTOS_CHECK_STACKOVERFLOW_CANARY=y` 开着，溢出即 panic。

**待办**：进/出 vterm 各打一次 `uxTaskGetStackHighWaterMark`，实测峰值 + 安全余量再 right-size，替代拍脑袋的 24KB。

## #10 host profile 定性：parse = PSRAM 写带宽墙（非 CPU）

host 侧用 gprof 回放同一份 /tmp/pi-session.raw（最小 harness，镜像 ESP32 drain 循环：1024B 块 + 每块 flush_damage）。

| | 4.96MB 回放 parse |
|---|---|
| host（CPU only，x86 -O2）| **72ms**（~69 MB/s）|
| ESP32 | 12.6s（394 KB/s，no-push）|

ESP32 慢 **174×** → 瓶颈是内存（PSRAM），不是 CPU。damage_merge CELL vs SCROLL 在 host 零差别（damage 回调是噪声）。

### gprof 真实计数（纠正 §5 的错误估计）

| 函数 | 真实调用次数 | §5 估的 |
|---|---|---|
| `putglyph` | **3,075,045** | 100 万 |
| `moverect_internal`（滚动）| **39,676** | 1.1 万 |
| `erase_internal` | 39,692 | — |
| `linefeed` | 39,710 | — |

（§5 的"1.46MB / 1.1万行"是更早的小抓包；当前 replay 是 4.96MB / 39.7k 行。）

### 真实 PSRAM 写量（三足鼎立，无单一"大头"）

| 项 | 次数 | 每次 | PSRAM 写 | 读 |
|---|---|---|---|---|
| sb_push | 39,676 | 1.6KB | **63.5MB** | 50.8MB |
| erase | 39,692 | 1.28KB | **50.8MB** | — |
| putglyph | 3,075,045 | 16B | **49MB** | — |

- 无 push ~100MB 写；有 push ~114MB 写 + 51MB 读。
- 三者都是**不可削减的顺序写**（写满 cache line，写 12B 还是 16B 触发的 line 数一样）→ CPU 优化无效、字节级压缩无效。

### 本轮实验结论

| 实验 | 结果 |
|---|---|
| **D1 屏幕迁内部 RAM** | ❌ 屏幕是 2 块（primary+altscreen 各 37.5KB）= 75KB，内部空闲 52KB，且碎片化（最大连续块 ~31KB < 37.5KB）|
| **D3 erase 快路径（预建 blank + splat）** | ❌ 实测 parse 无变化（394 vs 395 KB/s）—— erase 是 PSRAM 写-bound，省 CPU 没用 |
| **D2 pen-dirty elision** | ❌ 逻辑硬伤：每个 cell 都要写自己的 pen，"pen 没变"≠"这个 cell 已有 pen"，放弃 |
| **sb_pushline 关闭** | 304→394 KB/s（+30%），39,676 次调用，~114MB PSRAM 流量 |
| **布局对齐 + memcpy（VTermScreenCell 20B ↔ ScreenCell 16B）** | 可行但只省 CPU 小头，PSRAM 流量不动，放弃 |
| **8B cell（颜色量化 6-bit 打包 + 迁内部 DRAM）** | 436 KB/s（no-push，+10%）/ 318 KB/s（push）。primary 19.2KB 进内部 RAM ✅，altscreen 碎片化没进去（pi 不用 altscreen，无关）。+10% 撑不起代价（bold 测试挂 + 量化复杂度），**已撤回** |
| **xRingbuffer 双 ring + IAC 状态机（vterm_telnet 重构）** | parse 掉 30%（304→204 KB/s）+ pi -c 错乱。回放 8.1s（host 610 KB/s）vs 原版 16.7s（0.30 MB/s 背压正确）→ item 语义破坏背压。**已回退原版**（手搓环字节流才是正确语义）|

### 结论（修正）

**parse 的真正天花板是 CPU（Xtensa 单发射跑 parser 状态机 + 3.15M damage 回调 + 逐 cell 处理），不是 PSRAM 写带宽。** 证据：8B cell 把写量减半 + 迁内部 DRAM，只 +10%（12.6→11.4s）；若真是写带宽墙应省数秒。host 72ms vs ESP32 11.4s 的 158× 差距主要是 CPU 速度差（x86 乱序超标量 vs Xtensa 单发射）。

**完整阶梯**：push+16B+PSRAM = 304 → no-push = 395（sb_push +30%）→ no-push+8B+internal = 436（+10%）。sb_push 是最大杠杆，8B/内部 RAM 收益小。parse 优化到此为止，进一步只能靠结构性改动（均已排除）。

## 其它改动（本 session）

- **free_buffer use-after-free 修复**：环形缓冲改动引入——`free_buffer` 释放 `rowptr[0]`，但滚动后指针已旋转，`rowptr[0]` 不是块基址。host harness 一跑就崩定位到。修法：找行指针最小值作基址。ESP32 从不 `vterm_free` 所以没炸，但 resize 路径会踩。host 177/177 通过。
- **bb[8192] bench 删除**：selftest 里 TEMP bench 的 8KB 静态缓冲，回收 8KB DRAM（dram 43651→~52KB）。
- **栈 hwm 实测**：`stack hwm: 3824/3696 bytes free`（8KB 栈峰值用 ~4KB）→ 8KB 够，上面"待办"已解决。
- **vterm DRAM 占用实测**：VTERM_MODE=0 A/B → ~23KB（s_ring 8K + outbuf/tmpbuf 8K + telnet-cli 栈 4K + 结构 ~3K）。屏幕/s_pixels/s_sb 都在 PSRAM。
- **调试代码删除（收尾）**：整个 VTERM_PROFILE profiler（含 uxTaskGetStackHighWaterMark、PROF_BEGIN/END 宏、prof_bg_us/prof_glyph_us 字段）+ drain/sb_push/sync/key/render 日志 + VTERM_BOOT 直进——全部真删（非 #define 0），二进制 -2KB。
