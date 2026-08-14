#!/usr/bin/env bash
# 16colo.rs 整站合集解压脚本。
# 把 mega zip（含 16colo-packs/<年份>/<pack>.zip）解压到 scripts/art/packs/<年份>/，
# 供 telnet_bash_srv.js --art scripts/art/packs 直接加载。
#
# 用法:
#   ./scripts/unzip-packs.sh                 # 默认 ~/16colo-packs.zip
#   ./scripts/unzip-packs.sh /path/x.zip     # 指定 mega zip
#   ./scripts/unzip-packs.sh -n /path/x.zip  # 跳过已存在的文件（增量/续传）
#
# 解压后建议运行 ./scripts/verify-packs.js 核对完整性。
set -euo pipefail

MODE="-o"
ZIP="${HOME}/16colo-packs.zip"
if [ "${1:-}" = "-n" ]; then
	MODE="-n"
	shift
fi
if [ -n "${1:-}" ]; then
	ZIP="$1"
fi

DEST="$(cd "$(dirname "$0")/art/packs" && pwd)"

if [ ! -f "$ZIP" ]; then
	echo "找不到 $ZIP" >&2
	exit 1
fi

echo "解压 $ZIP -> $DEST （模式: $([ "$MODE" = "-n" ] && echo 增量 || echo 覆盖)）"
mkdir -p "$DEST"
# 只解压 .zip 条目（跳过 .rar/.lha 等服务器不支持的格式），保留年份目录
unzip "$MODE" -q "$ZIP" '16colo-packs/*.zip' -d "$DEST"

# 去掉多余的 16colo-packs/ 顶层包装
if [ -d "$DEST/16colo-packs" ]; then
	shopt -s nullglob
	mv "$DEST"/16colo-packs/*/ "$DEST"/ 2>/dev/null || true
	shopt -u nullglob
	rmdir "$DEST/16colo-packs" 2>/dev/null || true
fi

N="$(find "$DEST" -name '*.zip' | wc -l)"
echo "完成: $N 个 zip（建议运行 node scripts/verify-packs.js 核对）"
