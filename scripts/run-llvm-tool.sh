#!/usr/bin/env bash
set -euo pipefail

if [[ $# -eq 0 ]]; then
  echo "usage: run-llvm-tool.sh <tool> [arguments...]" >&2
  exit 2
fi

tool="$1"
shift

if command -v "${tool}" >/dev/null 2>&1; then
  exec "${tool}" "$@"
fi

if command -v brew >/dev/null 2>&1; then
  llvm_tool="$(brew --prefix llvm)/bin/${tool}"
  if [[ -x "${llvm_tool}" ]]; then
    exec "${llvm_tool}" "$@"
  fi
fi

echo "Unable to find LLVM tool: ${tool}" >&2
exit 1
