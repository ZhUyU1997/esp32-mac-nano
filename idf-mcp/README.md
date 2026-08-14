# idf-mcp

ESP-IDF 项目控制台：一个**按键驱动的 TUI** + 一个 **MCP server**，人和 agent 共用同一个
「当前命令」槽位。

```
┌──────────────────────────────────────────────────────────────┐
│ [idf-mcp] esp32-mini-mac  ● idle                            │ ← 状态栏（持久）
├──────────────────────────────────────────────────────────────┤
│ [Build] [Flash] [Flash+Mon] [Monitor] [Reboot] [Stop] [Clear] [Quit] │ ← 按钮栏
├──────────────────────────────────────────────────────────────┤
│  I (30)  boot: ESP-IDF v5.5.4 ...                            │ ← 输出区（占满剩余屏幕）
│  I (120) app_main: Mac Plus starting...                      │    build/flash/串口日志
│  ...                                                         │    实时流到这里
└──────────────────────────────────────────────────────────────┘
```

- 顶部两行固定：状态栏（idle / ▶ 命令 秒数 / ✓·✗ 命令 exit N）+ 按钮栏。
- 下方全部空间是输出区：颜色、`\r` 进度条、滚动都保留，build/flash/monitor 输出实时显示。
- **无持久 bash**：每条命令是独立的 pty 子进程（退出码精确，无需完成哨兵）。
- IDF 环境在启动时一次性捕获（`source activate; env -0`）。

## 用法

```bash
cd /home/yzhu/esp32-mini-mac && pnpm idf-mcp     # 或 cd idf-mcp && pnpm start
```

`--activate <script>` 指定 IDF 激活脚本，`--activate ""` 禁用探测（默认探测
`~/.espressif/tools/activate_idf_v5.5.4.sh`）。

MCP server 监听 `http://127.0.0.1:8765/mcp`。

## 按钮

| 按钮 | 动作 | 子进程 |
|---|---|---|
| **Build** | 编译 | `idf.py build`（阻塞）|
| **Flash** | 烧录 | `idf.py flash`（阻塞）|
| **Flash+Mon** | 烧录后直接看串口，**只复位一次** | `idf.py flash monitor --no-reset`（异步）|
| **Monitor** | 看串口日志（启动即复位芯片）| `idf.py monitor`（异步）|
| **Reboot** | 重启芯片 | monitor 运行中→发 `Ctrl-T Ctrl-R`；空闲→开 monitor |
| **Stop** | 停止当前命令 | monitor→`Ctrl-]`；其它→`Ctrl-C`(SIGINT) |
| **Clear** | 清空输出区 | — |
| **Quit** | 退出程序 | 先杀掉运行中的子进程 |

交互：**鼠标 hover 高亮 + 左键点击**，或键盘 `←/→` 移动 + `Enter` 触发。
**输出区拖拽选中、`y` 复制**（OSC52，终端支持则直接进剪贴板；否则回退 clip.exe/xclip/pbcopy）。
禁用按钮置灰不可触发。`Ctrl-C` = Stop（有命令运行时）/ 退出（空闲时）。

## MCP 工具

| 工具 | 说明 |
|---|---|
| `idf_execute(command, timeoutMs?, tail?)` | **阻塞**任意命令（`bash -c`），超时返回 `{status:"running"}`，命令继续跑 |
| `idf_build(extra?, timeoutMs?, tail?)` | **阻塞** `idf.py build` |
| `idf_flash(port?, timeoutMs?, tail?)` | **阻塞** `idf.py flash` |
| `idf_flash_monitor(port?)` | **异步** `idf.py flash monitor --no-reset`（烧录+看日志，单复位）|
| `idf_monitor(port?)` | **异步** `idf.py monitor`（启动即复位）|
| `idf_reboot(port?)` | monitor 运行中→`Ctrl-T Ctrl-R`；空闲→开 monitor（启动复位）|
| `idf_interrupt()` | 终止当前命令（monitor→`Ctrl-]`，其它→`Ctrl-C`）|
| `idf_read_output(tail?, offset?, filter?, level?, clear?)` | 读当前命令输出（分页/过滤/级别）|
| `idf_log_stats()` | 日志统计（I/W/E 计数）|
| `idf_status()` | 运行状态 / 项目目录 / mock 标志 |

资源 `idf://instructions`：给 agent 的任务说明。

## Mock 模式（测试，不碰真实硬件）

```bash
MCP_IDF_MOCK=1 node idf-mcp.ts   # 或 pnpm start:mock
```

`flash`/`monitor`/`flash_monitor` 走 mock 脚本；`idf_build` 仍是真 `idf.py build`（未 mock）。
`node client-test.mjs` 冒烟测试：flash → monitor → reboot → interrupt → flash_monitor → execute 超时 → reboot 空闲。

## 环境变量

| 变量 | 默认 | 说明 |
|---|---|---|
| `MCP_IDF_PORT` | `8765` | MCP HTTP 端口 |
| `MCP_IDF_HOST` | `127.0.0.1` | 监听地址 |
| `MCP_IDF_BUFFER` | `16777216` | ring buffer 字节上限 |
| `MCP_IDF_PROJECT` | cwd | 项目目录 |
| `MCP_IDF_SERIAL_PORT` | — | 串口路径（不设则 idf.py 自动探测）|
| `MCP_IDF_MOCK` | — | `1` 启用 mock |

## 实现说明

- **单子进程槽位**：同一时刻只有一个子进程；人按按钮和 agent 调 MCP 工具打到同一个状态机，
  忙时拒绝新命令（`a command is already running`）。
- **无持久 bash**：命令直接 `pty.spawn`（`idf.py` / `bash -c`），`exit` 事件给精确退出码；
  被信号打断的按 `128+signal` 计（Ctrl-C = 130）。
- **输出双路**：子进程输出（a）解析成显示行喂给 React log（`\r` 进度条原地覆盖、
  ESP-IDF I/W/E 级别着色），（b）剥 ANSI 后进 ring buffer 供 MCP 读取。
- **TUI 框架**：[Dye](https://github.com/andrewjsauer/dye)（Ink 的现代 fork）：内置鼠标 hover/click、
  文本选中 + OSC52 剪贴板、React 组件；终端复原由框架处理。

## 安全

- 仅监听 `127.0.0.1`（`MCP_IDF_HOST` 可改）；无认证，勿暴露公网。
- `port` 只允许串口路径字符，`idf_build` 的 `extra` 只允许 flag 字符，拒绝 shell 元字符注入。
- 单行超 4096 字节截断并标记 `…[truncated]`。
