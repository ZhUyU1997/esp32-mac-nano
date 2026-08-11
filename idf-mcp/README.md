# idf-mcp

ESP-IDF 项目控制台，由 MCP agent 完全驱动、人类只读观看。

agent 通过 MCP 执行 `idf.py` 命令（flash / monitor / build / 任意命令）并随时中断；
pty 输出实时流到你的终端——你可以看到 agent 的一切操作。**你不需要输入**，
但保留 Ctrl-C 紧急刹车：有命令在跑时转发中断，空闲时退出程序。

```
agent ──MCP──► idf-mcp ──pty──► bash（IDF 项目目录）
                     │
                     └──► 你的终端（只读实时显示 + 紧急刹车）
```

## 用法

项目根目录一键启动（自动探测并激活 IDF 环境）：

```bash
cd /home/yzhu/esp32-mini-mac && pnpm idf-mcp
```

或直接运行：

```bash
cd idf-mcp && pnpm start
```

**IDF 环境激活**：程序通过 bash `--rcfile` 在 shell 初始化时自动
`source` 激活脚本，无需手动 source。默认探测
`~/.espressif/tools/activate_idf_v5.5.4.sh`；也可显式指定：

```bash
node idf-mcp.ts --activate /path/to/activate.sh   # 指定脚本
node idf-mcp.ts --activate ""                     # 禁用探测
```

不带参数则不激活（沿用继承的环境）。

启动后 MCP server 监听 `http://127.0.0.1:8765/mcp`。

## MCP 工具

| 工具 | 说明 |
|---|---|
| `idf_execute(command, timeoutMs?, tail)` | **阻塞**执行任意命令，完成后返回 `{status, exit, totalLines}` + tail 输出（`tail` 必填，1-100 行）；超时（`timeoutMs`，默认 600s）返回 `{status:"running"}`，命令继续跑 |
| `idf_build(extra?, timeoutMs?, tail?)` | **阻塞** `idf.py build`，语义同上 |
| `idf_flash(port?, timeoutMs?, tail?)` | **阻塞** `idf.py flash`，语义同上 |
| `idf_monitor(port?)` | **异步**（日志流永不结束）：立即返回 started，agent 读实时日志、`idf_interrupt` 停止 |
| `idf_interrupt()` | 终止前台命令：monitor 发退出键 Ctrl-]（esp-idf-monitor 在 raw mode 下把 Ctrl-C 转发给芯片、不退出），其它命令发 Ctrl-C（SIGINT） |
| `idf_read_output(tail?, offset?, filter?, level?, clear?)` | 只读**上一次执行命令**的输出（新命令开始即清空，天然不混读；monitor 运行中读实时日志）；固定每页 100 行，offset 翻页；filter 正则 / level 级别过滤；返回 `{text, totalLines, nextOffset, hasMore}` |
| `idf_log_stats()` | 日志统计：总行数、字节数、I/W/E 级别计数 |
| `idf_status()` | 项目目录 / 运行状态（running/lastCmd）/ mock 标志 |

**资源**：`idf://instructions` —— 给 agent 的任务说明（谁来驱动、用什么工具）。

**命令完成检测**：bash 的 `PROMPT_COMMAND` 在每次回提示符时输出 OSC 标题哨兵
`MCP_CMD_END:<exit>`（屏幕不可见），`idf_status` 据此自动从 running 回到 idle，
即使命令被 Ctrl-C 中断（交互 bash 会放弃命令行，故不用分号拼接）。

**动态提示符**：rcfile 的 `PROMPT_COMMAND` 读 node 写的状态文件（`/tmp/idf-mcp-state-<pid>`），
空闲时显示 `[idf-mcp] ~/dir [idle]`（绿色），agent 正在跑命令时显示
`[idf-mcp] ~/dir ▶ <cmd>`（黄色）——无尾随 `$`，终端是 watch-only。
提示符只在命令结束后渲染（bash 行为），运行中的 ▶ 同时反映在标题栏。

## 用户侧行为

- **只读**：终端关 echo，stdin 不转发，你无法输入
- **紧急刹车**：Ctrl-C → 命令运行中则转发中断（等价 agent `idf_interrupt`）；
  空闲则退出程序
- **标题栏**：命令运行时显示 `▶ <cmd>`，结束恢复

## Mock 模式（测试，不碰真实硬件/idf.py）

```bash
MCP_IDF_MOCK=1 node idf-mcp.ts
```

`idf_flash` → `mock/mock-flash.mjs`（假 esptool 进度），
`idf_monitor` → `mock/mock-monitor.mjs`（假串口日志，SIGINT 退出）。
工具流程与真实一致，可安全验证 execute/interrupt/状态机。

```bash
node client-test.mjs   # 冒烟测试：flash → monitor → interrupt → execute → interrupt
```

## 配置 AI agent

**Claude Code**（`.mcp.json`）：

```json
{
  "mcpServers": {
    "idf-mcp": {
      "type": "http",
      "url": "http://127.0.0.1:8765/mcp"
    }
  }
}
```

## 环境变量

| 变量 | 默认 | 说明 |
|---|---|---|
| `MCP_IDF_PORT` | `8765` | MCP HTTP 端口 |
| `MCP_IDF_HOST` | `127.0.0.1` | 监听地址 |
| `MCP_IDF_BUFFER` | `16777216` | ring buffer 字节上限（默认 16 MiB） |
| `MCP_IDF_PROJECT` | cwd | 项目目录 |
| `MCP_IDF_MOCK` | — | `1` 启用 mock 模式 |

## 实现说明

- `node-pty`：bash 跑在独立 pty（自有进程组），输出直显 + ring buffer
- 用户 Ctrl-C 到不了 bash 的进程组 → node 收到后转发中断进 pty（monitor 用退出键 Ctrl-]，其余命令 Ctrl-C，与 agent `idf_interrupt` 同路径）
- `@modelcontextprotocol/sdk` Streamable HTTP，每客户端连接一个独立 `McpServer` 实例

## 安全与健壮性

- **并发限制**：有命令运行时，新的 execute/build/flash/monitor 注入会被拒绝（需先 `idf_interrupt`）——单个 pty 无法区分多命令的完成哨兵
- **参数校验**：`idf_flash`/`idf_monitor` 的 `port` 只允许串口路径字符，`idf_build` 的 `extra` 只允许 flag 字符——拒绝 shell 元字符注入
- **完成哨兵**：匹配完整 OSC 序列 `\x1b]0;MCP_CMD_END:<exit>\x07`，命令输出里的普通文本 `MCP_CMD_END` 不会误判；跨 onData 分块也安全
- **行截断**：单行超 4096 字节截断并标记 `…[truncated]`，防超长行撑爆响应
- 仅监听 `127.0.0.1`（`MCP_IDF_HOST` 可改）；无认证，本机进程可调用，勿暴露到公网
