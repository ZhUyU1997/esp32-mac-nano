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
- **滚动**：moverect 返回 0 → 滚动=整屏重画（45ms）；真正的滚动只复制 fb 区域可进一步加速，但复杂。
