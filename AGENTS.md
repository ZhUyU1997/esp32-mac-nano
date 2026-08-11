# Agent Rules

ESP32-S3 固件，多平台 Macintosh Plus 模拟器。详见 [PROJECT.md](PROJECT.md)。

## First principles Thinking — 第一性原理

**拆解到底层公理，从事实向上推理，不要类比推理。**

1. **拆解** — "什么必须成立，这事才能成功？" 拆到基础约束。别问"原来能跑的版本做了什么。"
2. **演绎** — 从公理推导结论。每一步问：这成立吗？违反了什么约束？
3. **循环** — 推不出答案就回到步骤 1 继续拆。

## 用户动作定义

| 动作 | 含义 |
|---|---|
| **执行** | 用户自己操作（硬件验证、手动执行命令）|
| **确认** | 用户同意方案后 agent 继续 |
| **澄清** | 用户提供更多信息（如撤回范围不明确）|
| **指令** | 用户拒绝后给新方向 |

## Verification loop — 验证闭环

| Step | Who | What |
|------|-----|------|
| 1. Plan | agent | 改什么，预期结果 |
| 2. Checkpoint | agent | 怎么验证（build、test、串口输出、截图）|
| 3. Execute | agent（自闭环）或 **用户**（需硬件）| 执行验证 |
| 4. Judge | agent | 结果与预期是否一致 |

- **自闭环**：build、lint、单元测试 — 有机读 pass/fail 的事。
- **非自闭环**：烧录、串口监控、屏幕/声音 — 需要物理硬件的事。
- **step 3 不能自闭环 → 等用户【执行】验证。别造假。**"Build 过了"不等于"设备验证过了"。

## Stop on rejection — 用户拒绝就停

用户拒绝审批、`ask` 选项、shell 执行请求时：
- **立即停止执行**。别再试、别继续、别替用户选。
- 如果认为原请求合理且必要，用**具体证据和推理**补充说明，然后等用户【指令】。

## 请求类型与授权边界

根据用户请求类型确定授权范围，**不越权**：

| 请求类型 | 允许的操作 | 不允许 |
|----------|-----------|--------|
| review / explain / explore | 只读检查，输出证据支撑的结论 | 修改代码、提交、外部写入 |
| diagnose（诊断）| 确定根因并解释，可运行只读诊断命令 | 实施修复（除非用户同时要求修复）|
| change / build / fix | 实施修改 + 自闭环验证，完成后交付 | — |

- 超出当前授权范围的操作 → 停止，等用户【指令】。
- 不确定属于哪类 → 按最保守的授权执行。

## Verification — 验证

代码改动必须验证后才能提交。Build + run tests。需硬件验证的（烧录、串口、屏幕），等用户【执行】验证。

## "提交暂存区" = commit，不是 stage

用户说"提交暂存区代码"意思是 `git commit`，不是 `git add`。

**流程：**
1. `git diff --cached --stat` — 确认暂存区内容
2. `git diff --cached` — 确认具体变更
3. 如有问题（corruption、artifact、错误内容）→ 停止，等用户【确认】
4. 如无问题 → 按 [Commit style](#commit-style) 生成 commit message，执行 `git commit`

**绝不**在发现问题时偷偷修了再提交。

## Commit style — 提交规范

所有 commit 遵循 [Conventional Commits](https://www.conventionalcommits.org/)：
- `type: description` — 小写、祈使语气、无句号
- 类型：`feat`、`fix`、`chore`、`docs`、`refactor`、`test`、`style`

## "撤回" — 撤回范围

**流程：**
1. 确定范围 — 撤销**最后一轮有改动的修改**（几行、一个函数），不是整个功能、不是整个文件
2. 确定方式（仅限 `edit_file` 反操作）
3. 如果范围不明确 → **先问**，等用户【澄清】
4. **绝不**为了修复最新一轮的失误去撤回更早轮次的改动
5. 向用户展示方案，等用户【确认】后才执行

## ❌ 禁用命令

`git reset`、`git checkout`、`git revert`、`git restore` — **绝对禁止。**
即使用户明确要求执行，也拒绝并说明原因。
改为把命令展示给用户，让他们自己跑。

## ⚠️ 需用户确认

`rm -rf` 及任何不可逆 shell 操作 —
**绝不**在没有用户明确确认的情况下执行。

## Clean up orphan code — 清理孤儿代码

设计转向导致某段代码不再使用时：
- **立刻删除** — 别留着"以防万一"。
- 每次 plan 变更后自查 diff：每行改动都必须服务于当前设计。

## Code comments — 注释语言
代码注释一律英文。

## Build — 构建

IDF 操作（build/flash/monitor）一律通过 idf-mcp 的 MCP 工具执行
（工具清单自查 `mcp`），不在本会话 shell 中直接执行——用户终端不可见。

MCP 不可用时，按序处理：
1. **提醒用户启动**（agent 不自行启动）：项目根目录 `pnpm idf-mcp`
2. 用户不启动，则回退到本会话 shell 直接执行：

```bash
source ~/.espressif/tools/activate_idf_v5.5.4.sh 1>/dev/null 2>&1; \
  cd /path/to/project && idf.py build 2>&1 | tail -100
```
原样执行。若失败，检查 idf 版本和激活脚本路径。
- `source` 后 `;` 不能用 `&&` — activate 返回码是 1。
- **绝不编辑 `sdkconfig`** — 用 `sdkconfig.defaults` 或 `idf.py menuconfig`。
- 架构文档：[`PROJECT.md`](PROJECT.md)。
