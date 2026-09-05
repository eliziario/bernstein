#!/usr/bin/env bash
set -euo pipefail

if command -v cmake-format >/dev/null 2>&1; then
  exec cmake-format "$@"
fi

if command -v pipx >/dev/null 2>&1; then
  pipx_bin_dir="$(pipx environment --value PIPX_BIN_DIR)"
  if [[ -x "${pipx_bin_dir}/cmake-format" ]]; then
    exec "${pipx_bin_dir}/cmake-format" "$@"
  fi
fi

echo "Unable to find cmake-format. Run ./scripts/bootstrap-dev.sh first." >&2
exit 1
