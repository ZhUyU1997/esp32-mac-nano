#!/usr/bin/env bash
set -euo pipefail

mode="apply"
paths=()

while [[ $# -gt 0 ]]; do
	case "$1" in
	--check)
		mode="check"
		shift
		;;
	--apply)
		mode="apply"
		shift
		;;
	-h|--help)
		printf '%s\n' \
			"Usage: scripts/clang-format.sh [--apply|--check] [paths...]" \
			"" \
			"Default: --apply" \
			"" \
			"Notes:" \
			"- Formats tracked C/C++ sources under the provided paths (or main/ by default)" \
			"- Skips third-party code (main/third_party/jsmn, main/core/macplus/cpu/musashi)" \
			"- Skips main/include/common/class.h"
		exit 0
		;;
	*)
		paths+=("$1")
		shift
		;;
	esac
done

repo_root="$(git rev-parse --show-toplevel 2>/dev/null || true)"
if [[ -z "$repo_root" ]]; then
	printf '%s\n' "error: must run inside a git repository" >&2
	exit 2
fi
cd "$repo_root"

clang_format=""
if command -v clang-format >/dev/null 2>&1; then
	clang_format="clang-format"
elif command -v clang-format-14 >/dev/null 2>&1; then
	clang_format="clang-format-14"
fi
if [[ -z "$clang_format" ]]; then
	printf '%s\n' "error: clang-format not found in PATH" >&2
	exit 2
fi

if [[ ${#paths[@]} -eq 0 ]]; then
	paths=("main")
fi

list_files() {
	git ls-files -- "${paths[@]}" | \
		grep -E '\.(c|cc|cpp|cxx|h|hh|hpp|hxx|S)$' || true
}

exclude_re='^(main/third_party/jsmn/|main/core/macplus/cpu/musashi/|main/include/common/class\.h$)'

mapfile -t files < <(list_files | grep -vE "$exclude_re" || true)

if [[ ${#files[@]} -eq 0 ]]; then
	printf '%s\n' "no matching tracked files"
	exit 0
fi

printf '%s\n' "clang-format: $clang_format"
printf '%s\n' "mode: $mode"
printf '%s\n' "files: ${#files[@]}"

if [[ "$mode" == "check" ]]; then
	printf '%s\0' "${files[@]}" | xargs -0 -r "$clang_format" -style=file --dry-run --Werror
	exit 0
fi

printf '%s\0' "${files[@]}" | xargs -0 -r "$clang_format" -style=file -i
