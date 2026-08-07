#!/usr/bin/env bash
# Sync files from one HFS disk image to another using hfsutils.
# Directory layout on the destination matches the source; missing folders are created.
# Data is staged through a Unix temp file (HFS -> Unix -> HFS), using MacBinary (-m).
set -euo pipefail

usage() {
  echo "Usage: $0 [OPTIONS] SOURCE.img DEST.img [--] [DIR ...]" >&2
  echo "  Copy files from SOURCE HFS volume to DEST, preserving paths." >&2
  echo "  Directories that do not exist on DEST are created (hmkdir)." >&2
  echo "  With no DIR arguments, the whole volume is synced." >&2
  echo "  With DIR(s), only those subtrees are synced (HFS path from volume root)." >&2
  echo "Options:" >&2
  echo "  -d, --dir PATH    Sync only this directory (repeatable). Slashes become ':'." >&2
  echo "  -n, --dry-run     Print actions only" >&2
  echo "  -v, --verbose     Print each directory and file as it is processed" >&2
  echo "  -a, --all         Include invisible Finder items (default: on)" >&2
  echo "  --visible-only    Only copy visible names (hls without -a)" >&2
  echo "  -h, --help        Show this help" >&2
}

DRY_RUN=0
VERBOSE=0
INCLUDE_ALL=1
ONLY_DIRS=()

while [ $# -gt 0 ]; do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    -n|--dry-run)
      DRY_RUN=1
      shift
      ;;
    -v|--verbose)
      VERBOSE=1
      shift
      ;;
    -a|--all)
      INCLUDE_ALL=1
      shift
      ;;
    --visible-only)
      INCLUDE_ALL=0
      shift
      ;;
    -d|--dir)
      if [ $# -lt 2 ]; then
        echo "$0: --dir requires a path" >&2
        exit 1
      fi
      ONLY_DIRS+=("$2")
      shift 2
      ;;
    -*)
      echo "Unknown option: $1" >&2
      usage
      exit 1
      ;;
    *)
      break
      ;;
  esac
done

if [ $# -lt 2 ]; then
  usage
  exit 1
fi

SRC_ARG=$1
DST_ARG=$2
shift 2

while [ $# -gt 0 ]; do
  if [ "$1" = "--" ]; then
    shift
    continue
  fi
  ONLY_DIRS+=("$1")
  shift
done

command -v readlink >/dev/null || { echo "readlink is required" >&2; exit 1; }
SRC_IMAGE=$(readlink -f "$SRC_ARG")
DST_IMAGE=$(readlink -f "$DST_ARG")

if [ ! -f "$SRC_IMAGE" ] || [ ! -f "$DST_IMAGE" ]; then
  echo "SOURCE and DEST must be existing regular files." >&2
  exit 1
fi

if [ "$SRC_IMAGE" = "$DST_IMAGE" ]; then
  echo "SOURCE and DEST must be different files." >&2
  exit 1
fi

for cmd in hmount humount hls hcd hcopy hmkdir hdel; do
  command -v "$cmd" >/dev/null || { echo "Missing hfsutils command: $cmd" >&2; exit 1; }
done

hfs_list() {
  if [ "$INCLUDE_ALL" -eq 1 ]; then
    hls -1a 2>/dev/null
  else
    hls -1 2>/dev/null
  fi
}

is_hfs_dir() {
  local n=$1
  [ "$(hls -ld -- "$n" 2>/dev/null | awk '{print $1}')" = "d" ]
}

# Move SOURCE cwd to colon path from volume root (empty = root).
navigate_src() {
  local rel=$1
  hvol "$SRC_IMAGE" >/dev/null 2>&1
  hcd
  if [ -n "$rel" ]; then
    hcd ":$rel" || return 1
  fi
  return 0
}

# Create each path segment on DEST (colon-separated relative path, no leading colon).
ensure_dir_dst() {
  local path=$1
  hvol "$DST_IMAGE" >/dev/null 2>&1
  hcd
  [ -z "$path" ] && return 0
  local -a parts
  IFS=':' read -ra parts <<< "$path"
  local p
  for p in "${parts[@]}"; do
    [ -z "$p" ] && continue
    if ! is_hfs_dir "$p"; then
      hmkdir "$p"
    fi
    hcd "$p"
  done
}

# Remove a file on DEST if it exists (same name in current cwd after ensure_dir_dst).
remove_dest_file_if_exists() {
  local base=$1
  local t
  t=$(hls -ld -- "$base" 2>/dev/null | awk '{print $1}' || true)
  case "$t" in
    f|fi) hdel "$base" ;;
    d)
      echo "hfs-sync: destination is a directory, not overwriting: $base" >&2
      return 1
      ;;
  esac
  return 0
}

# Copy one file given by colon path from root, e.g. Applications:MacPaint 1.5
copy_file() {
  local full=$1
  local TMP
  TMP=$(mktemp)
  local parent base

  if [[ "$full" == *:* ]]; then
    parent="${full%:*}"
    base="${full##*:}"
  else
    parent=""
    base="$full"
  fi

  hvol "$SRC_IMAGE" >/dev/null 2>&1
  hcd
  hcopy -m ":$full" "$TMP"

  ensure_dir_dst "$parent"
  if ! remove_dest_file_if_exists "$base"; then
    rm -f "$TMP"
    return 1
  fi
  hcopy -m "$TMP" ":$base"
  rm -f "$TMP"
}

sync_dir() {
  local rel=$1

  navigate_src "$rel" || {
    echo "hfs-sync: source path not found: ${rel:-/}" >&2
    exit 1
  }

  local -a entries=()
  mapfile -t entries < <(hfs_list | LC_ALL=C sort -f)

  local name t child full
  for name in "${entries[@]}"; do
    [ -z "$name" ] && continue
    navigate_src "$rel"
    t=$(hls -ld -- "$name" 2>/dev/null | awk '{print $1}')
    if [ "$t" = "d" ]; then
      if [ -n "$rel" ]; then
        child="${rel}:${name}"
      else
        child="$name"
      fi
      if [ "$VERBOSE" -eq 1 ]; then
        echo "dir  $child" >&2
      fi
      if [ "$DRY_RUN" -eq 0 ]; then
        ensure_dir_dst "$child"
      fi
      sync_dir "$child"
    else
      if [ -n "$rel" ]; then
        full="${rel}:${name}"
      else
        full="$name"
      fi
      if [ "$VERBOSE" -eq 1 ]; then
        echo "file $full" >&2
      fi
      if [ "$DRY_RUN" -eq 0 ]; then
        copy_file "$full"
      fi
    fi
  done
}

cleanup() {
  humount "$SRC_IMAGE" 2>/dev/null || true
  humount "$DST_IMAGE" 2>/dev/null || true
}
trap cleanup EXIT

normalize_hfs_rel() {
  local p=$1
  p="${p#:}"
  p="${p%:}"
  p="${p//\//:}"
  printf '%s\n' "$p"
}

declare -a NORM_DIRS=()
for d in "${ONLY_DIRS[@]}"; do
  [ -z "$d" ] && continue
  nv=$(normalize_hfs_rel "$d")
  [ -z "$nv" ] && {
    echo "hfs-sync: empty path after normalization: $d" >&2
    exit 1
  }
  NORM_DIRS+=("$nv")
done

hmount "$SRC_IMAGE" >/dev/null 2>&1
hmount "$DST_IMAGE" >/dev/null 2>&1

if [ ${#NORM_DIRS[@]} -eq 0 ]; then
  sync_dir ""
else
  for d in "${NORM_DIRS[@]}"; do
    sync_dir "$d"
  done
fi
