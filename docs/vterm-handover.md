# vterm 交接文档

> HEAD = `35460ed`。工作区改动已全部 staged，待 review 后 commit。
> 详细实验记录见 `docs/vterm-profile.md`。

## 背景

ESP32-S3 Mac 模拟器内置 vterm 终端（telnet 连 host bash，vendored libvterm = neovim fork v0.3.3）。
本轮解决 `pi -c` 卡 73 秒的问题。

## 数据流

```
pi(host) → node-pty → telnet:2324 → recv(telnet-cli 任务)
        → s_ring[8KB]（批量 push）→ vterm 循环(macplus 任务)
        → vterm_input_write(解析) → 滚动/putglyph → 脏矩形渲染 → 480×640 fb
```

## 最终改动（staged，12 文件 +548/−134）

| 文件 | 改动 |
|---|---|
| `libvterm/src/screen.c` | 滚动环形缓冲（行指针 + O(1) 指针旋转）+ **free_buffer use-after-free 修复** |
| `libvterm/include/vterm.h` | `MAX_CHARS_PER_CELL 6→1`（cell 36→16B；代价：不支持 combining）|
| `vterm_telnet.c/.h` | 批量 push（逐字节→memcpy）+ 阻塞 pop 信号量 |
| `vterm_esp32.c` / `term_render.c/.h` | 删除调试代码（profiler / drain / sb_push 日志 + bb bench）|
| `main.c` | `VTERM_BOOT=0` + macplus 栈 24K→8K |
| `scripts/*` | `--test burst` + `--replay` 模式 + `count-combining.js` |

## 性能结论

| 层 | 速率 |
|---|---|
| parse（push）| **304 KB/s** |
| parse（no-push）| 395 KB/s |
| 网络（批量 push 后）| 646 KB/s |
| host 纯 CPU（gprof）| 69 MB/s |

**瓶颈 = parse，且是 CPU-bound（Xtensa 单发射跑 parser 状态机），不是 PSRAM 写带宽。**
host 72ms vs ESP32 11.4s 的 158× 主要是 CPU 速度差。parse 优化到此为止。

**试过又撤回的**（有数据，详见 profile #10）：
- xRingbuffer 双 ring + IAC 状态机重构 telnet → parse −30% + pi -c 错乱，回退原版。
- D3 erase 快路径 → 零收益。
- 8B cell（颜色量化）→ +10%，不划算。
- 屏幕迁内部 RAM → 2×37.5KB 塞不进 52KB。

## 构建 / 测试

```bash
# host 回归（176/177；1 个 combining 失败是 MAX_CHARS=1 既有代价）
xmake -r build && ./build/linux/x86_64/release/vterm-test

# ESP32：idf-mcp MCP 工具（interrupt → build → flash /dev/ttyACM0 → monitor）
```

## 关键约束（AGENTS.md）

- 禁用 `git reset/checkout/revert/restore`；撤回只用 edit 反操作。
- IDF 操作走 idf-mcp MCP 工具。
- 提交前必须 `git diff --cached` 完整 review + 用户确认。
- 孤儿代码立刻删；注释英文；commit 走 Conventional Commits。

## 未决事项

- 工作区已全部 staged，待 commit。
- telnet host/port 硬编码 `192.168.31.173:2324`（vterm_telnet.c）——待配置化。
- `vterm_telnet.c` 的 `s_fd_mutex` 保护阻塞 send——理论隐患，键盘 <16B 对 2880B 缓冲永不触发，可留待后续。
