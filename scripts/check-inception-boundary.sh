#!/usr/bin/env bash
set -euo pipefail

if ! git check-ignore -q inception/README.md; then
  echo "inception/ must remain ignored by Git." >&2
  exit 1
fi

if git ls-files --cached -- inception | grep -q .; then
  echo "inception/ must not be tracked by Git." >&2
  exit 1
fi

if command -v jj >/dev/null 2>&1 && jj root >/dev/null 2>&1; then
  if jj --ignore-working-copy file list -r @ 'inception/**' | grep -q .; then
    echo "inception/ must not be tracked by Jujutsu." >&2
    exit 1
  fi
fi
