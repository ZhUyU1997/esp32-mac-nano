#!/usr/bin/env bash
# Print an HFS volume tree using hfsutils (hmount / hls / hcd / hpwd / humount).
set -euo pipefail

usage() {
  echo "Usage: $0 [OPTIONS] [HD.IMG]" >&2
  echo "  HD.IMG defaults to <repo>/macintosh/hd.img when run from this repo." >&2
  echo "Options:" >&2
  echo "  -a, --all         Include invisible Finder items (default: on)" >&2
  echo "  --visible-only    Only visible names (hls without -a)" >&2
  echo "  -h, --help        Show this help" >&2
}

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)
IMAGE="${REPO_ROOT}/macintosh/hd.img"
INCLUDE_ALL=1

while [ $# -gt 0 ]; do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    -a|--all)
      INCLUDE_ALL=1
      shift
      ;;
    --visible-only)
      INCLUDE_ALL=0
      shift
      ;;
    -*)
      echo "Unknown option: $1" >&2
      usage
      exit 1
      ;;
    *)
      IMAGE=$1
      shift
      ;;
  esac
done

for cmd in hmount humount hls hcd hpwd; do
  command -v "$cmd" >/dev/null || { echo "Missing hfsutils command: $cmd" >&2; exit 1; }
done

[ -f "$IMAGE" ] || { echo "Not a file: $IMAGE" >&2; exit 1; }

hfs_list() {
  if [ "$INCLUDE_ALL" -eq 1 ]; then
    hls -1a 2>/dev/null
  else
    hls -1 2>/dev/null
  fi
}

# Print tree under current HFS working directory; indent is ASCII branch prefix for this level.
walk() {
  local indent=$1
  local -a entries=()
  mapfile -t entries < <(hfs_list | LC_ALL=C sort -f)
  local n=${#entries[@]}
  local i=0
  local name t here branch rest
  for name in "${entries[@]}"; do
    [ -z "$name" ] && { i=$((i + 1)); continue; }
    if [ "$i" -eq $((n - 1)) ]; then
      branch="└── "
      rest="    "
    else
      branch="├── "
      rest="│   "
    fi
    printf '%s%s%s\n' "$indent" "$branch" "$name"
    t=$(hls -ld -- "$name" 2>/dev/null | awk '{print $1}')
    if [ "$t" = "d" ]; then
      here=$(hpwd)
      hcd "$name" || { i=$((i + 1)); continue; }
      walk "${indent}${rest}"
      hcd "$here" || return 1
    fi
    i=$((i + 1))
  done
}

cleanup() {
  humount 2>/dev/null || true
}
trap cleanup EXIT

hmount "$IMAGE" >/dev/null 2>&1
hcd :

hvol_out=$(hvol 2>/dev/null || true)
vol_name=$(echo "$hvol_out" | sed -n 's/.*Volume name is "\([^"]*\)".*/\1/p' | head -1)
free_line=$(echo "$hvol_out" | grep 'bytes free' | head -1 || true)
if [ -n "$vol_name" ]; then
  echo "Volume: ${vol_name}"
  echo "Image:  ${IMAGE}"
  [ -n "$free_line" ] && echo "$free_line"
  echo
fi

walk ""
