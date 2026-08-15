#!/bin/bash
# tools/term-mode.sh — 判别终端是 ANSI.SYS 语义 还是 DECAWM 语义
#
# 两种语义本身无对错, 只是超宽内容的落点不同:
#   ANSI.SYS 语义 → 折到下一行(GLDIG1.ANS 显示正常, 16colo.rs/libansilove 同款)
#   DECAWM   语义 → 盖掉行尾(GLDIG1.ANS 会错位, VT100/xterm/libvterm 同款)
#
# 用法:
#   bash tools/term-mode.sh             # 当前终端: 肉眼看结果
#   bash tools/term-mode.sh <终端程序>   # 仿真器: 自动判读
set -e
cd "$(dirname "$0")/.."

# 颜色(仅外层 echo 用)。测试内容必须用 \e 文本交给 printf 转 ESC,
# 直接嵌 ESC 字节会经 pty 进入 bash 的 readline, 被当成按键序列吞掉。
RESET=$'\e[0m'
YELLOW=$'\e[1;33m'
CYAN=$'\e[1;36m'
MAGENTA=$'\e[1;35m'

CUF=73   # ESC[73C: 行尾标记之后再往右挪 73 列(超宽), 复现 GLDIG1.ANS 的定位序列

# 手动模式带颜色; 自动模式纯文本(机器按列号判读)
CMD_COLOR="printf '\e[2J\e[H'; printf '测试: 终端是 ANSI.SYS 还是 DECAWM 语义?\n'; printf '\e[79C\e[1;36m#\e[0m\e[${CUF}C\e[1;35mX\e[0m\n'"
CMD_PLAIN="printf '\e[2J\e[H'; printf '测试: 终端是 ANSI.SYS 还是 DECAWM 语义?\n'; printf '\e[79C#\e[${CUF}CX\n'"

if [ $# -eq 0 ]; then
	eval "$CMD_COLOR"
	echo
	echo "${CYAN}════════════════════════════════════════════════${RESET}"
	echo " 上面: 行尾放标记 ${CYAN}#${RESET}, 又写一个 ${MAGENTA}X${RESET}"
	echo " 看【最右边】是哪个:"
	echo "   ${CYAN}# → ANSI.SYS 语义${RESET} (GLDIG1.ANS 显示正常)"
	echo "   ${MAGENTA}X → DECAWM 语义${RESET} (GLDIG1.ANS 会错位)"
	echo "${CYAN}════════════════════════════════════════════════${RESET}"
	exit 0
fi

PTY="$1"
OUT=$($PTY -c "$CMD_PLAIN" 2>/dev/null || true)
echo "$OUT"

LINE=$(grep -E "^ *X" <<< "$OUT" | head -1)
if [ -z "$LINE" ]; then
	echo
	echo "${YELLOW}>>> 测不出来 (没有内容输出)${RESET}"
	exit 1
fi
COL=$(awk '{ print index($0, "X") - 1 }' <<< "$LINE")

echo
echo "${CYAN}════════════════════════════════════════════════${RESET}"
if [ "$COL" -eq "$CUF" ]; then
	echo " ${CYAN}ANSI.SYS 语义${RESET} (GLDIG1.ANS 显示正常, 行尾是 #)"
elif [ "$COL" -eq 79 ] || [ "$COL" -eq 0 ]; then
	echo " ${MAGENTA}DECAWM 语义${RESET} (GLDIG1.ANS 会错位, 行尾是 X)"
else
	echo " ${YELLOW}? 测不出来 (X 在列 $COL)${RESET}"
fi
echo "${CYAN}════════════════════════════════════════════════${RESET}"
