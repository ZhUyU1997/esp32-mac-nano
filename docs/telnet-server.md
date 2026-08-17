# Telnet 服务器 & vterm 关键命令

ESP32 vterm 终端模式通过 telnet 连到 host 上的 `tools/art/telnet_srv.js`。
本文档汇总服务器各模式、素材工具和 vterm 端交互按键。

## 服务器模式（tools/art/telnet_srv.js）

```bash
# 1. 交互 bash（默认）：ESP32 键盘直连 host shell
node tools/art/telnet_srv.js

# 2. ASCII-art 画廊：连接自动显示第一张，回车切下一张/下一个包
node tools/art/telnet_srv.js --art scripts/art/packs
#    --art 可重复（合并多个 pack）：--art dirA --art file.zip
#    支持：目录（递归扫描 .zip/.ans/.ice/.txt）、单个 .zip、单个文件

# 2b. 90 年代拨号模拟：--baud 限制发送速度，作品像当年调制解调器一样逐行刷出
#     （8N1 串口 10bit/字节：2400≈240B/s, 9600≈960B/s, 14400≈1440B/s）
node tools/art/telnet_srv.js --baud 2400 --art scripts/art/packs
#    --baud 对 --file 同样生效；回车可中断当前流切下一张

# 2c. 自动播放：每张显示完等待 N 秒自动切下一张（与 --baud 组合时，
#     节流流式完成之后才开始计时；回车仍可手动切换/中断）
node tools/art/telnet_srv.js --auto 8 --art scripts/art/packs
node tools/art/telnet_srv.js --auto 3 --baud 9600 --art scripts/art/packs

# 3. 单文件测试模式：连接发送一个文件，回车重发（改文件后按回车即可重看）
node tools/art/telnet_srv.js --file tools/art/cp437-test.txt

# 4. 基准测试（vterm 渲染/blit 性能分析）
node tools/art/telnet_srv.js --test fill     # 每 500ms 清屏+全屏文本
node tools/art/telnet_srv.js --test scroll   # 每 50ms 一行
node tools/art/telnet_srv.js --test burst:sgr:5   # 灌 5 MiB SGR 序列测吞吐

# 5. 回放模式：连接时把捕获的原始终端流快速发送（A/B 对比）
node tools/art/telnet_srv.js --replay capture.bin
```

端口默认 2324，可用第一个位置参数改：`node tools/art/telnet_srv.js 2325 ...`

**画廊/文件模式自动处理**：CP437→UTF-8 转码（UTF-8 启发式，合法 UTF-8 不转）、SAUCE/COMNT 尾巴剥离、>80 列作品跳过（vterm 只有 80 列）、>30 行提示滚屏。zip 纯内存解压（Node zlib），无外部依赖；大合集惰性加载（一次只解压当前包）。

## 素材工具

**art 遍历/检测共享库**（`tools/art/art-lib.js`，telnet server 与测试脚本共用）：
zip 解压（内存 inflate）、目录扫描、SAUCE 剥离/列数、CP437→UTF-8、宽度检测。
宽度用**光标模拟**（CUP/CUF/CUB/保存恢复，跟踪每行最右列）——流式数字节对
定位型 art 会高估/低估（ABYSS 数成 157 实际 77、AKO 数成 53 实际 115）。

```bash
# 解压 16colo.rs 整站合集（JS 版，ZIP64 支持，9GB mega 流式读取）
# 默认 ~/16colo-packs.zip → scripts/art/packs/<年份>/，解压后自动验证并修复：
#   截断包（缺目录）→ 流式恢复重打包；method 6/1（老 PKZIP）→ unzip 回退重打包
node tools/art/unzip-packs.js             # 覆盖模式
node tools/art/unzip-packs.js -n          # 增量（跳过已有）
node tools/art/unzip-packs.js path.zip    # 指定 mega zip
node tools/art/unzip-packs.js --repair-only  # 只修复不重新解压

# 完整性核对（ZIP64 支持）：对比合集中央目录 vs 实际文件
node tools/art/verify-packs.js

# 批量转换测试：把所有 .ans/.ice 过一遍 vterm-ans（host 工具）
node tools/art/test-art.js                # 全部 packs（~7 万文件）
node tools/art/test-art.js --extract     # 只做解压测试（条目 inflate 验证）
node tools/art/test-art.js --render      # 只做 vterm-ans 渲染测试
node tools/art/test-art.js --count       # 统计文件数/总字节 + 90s 拨号传输时间
node tools/art/test-art.js --quality     # 渲染时额外解码 PNG，检测占位框
node tools/art/test-art.js pack.zip      # 指定 zip/目录/文件
node tools/art/test-art.js --width 0     # 不限宽度（默认跳过 >80 列）
node tools/art/test-art.js --width 120   # 自定义宽度阈值
node tools/art/test-art.js --log run.log # 逐文件结果留档
# 默认 extract+render 组合跑；逐文件结果直接打 stdout（OK/FAIL/SKIP+尺寸+SAUCE）
```

素材存 `scripts/art/packs/`（gitignore，不入库）；`scripts/art/README.md` 有详细说明。

## vterm 端交互按键（ESP32）

| 按键 | 作用 |
|---|---|
| **回车** | 画廊：下一张/下一个包；`--file`：重发文件 |
| **Shift+PageUp / PageDown** | 滚动回看 scrollback |
| **F10 / F12** | 退出 vterm 模式 |
| **鼠标滚轮** | 非 mouse 协议时滚动 scrollback |
| **左键拖选** | 文本选区（xterm 流式几何）；滚轮/拖选时光标不闪烁、选区随滚动同步 |
| **鼠标协议程序**（htop/vim）| 自动进入 mouse 模式，滚轮变按钮 4/5 |

## 渲染验证

`tools/art/cp437-test.txt` 覆盖半色调渐变、双线框、拉丁重音、希腊字母、符号 —
`node tools/art/telnet_srv.js --file tools/art/cp437-test.txt` 连上即可核对。

## host 端开发工具（tools/vterm/）

```bash
xmake run vterm-sdl        # 交互窗口（pty bash）
xmake run vterm-test       # 回归套件
```
