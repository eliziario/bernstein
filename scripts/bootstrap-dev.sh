#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
ocaml_root="${repo_root}/ocaml"

if ! command -v brew >/dev/null 2>&1; then
  echo "Homebrew is required to install the Bernstein development toolchain." >&2
  exit 1
fi

brew_prefix="$(brew --prefix)"
export PATH="${brew_prefix}/bin:${brew_prefix}/sbin:${PATH}"

brew bundle --file "${repo_root}/Brewfile"

if ! conan profile path default >/dev/null 2>&1; then
  conan profile detect --force
fi

if ! command -v pipx >/dev/null 2>&1; then
  echo "pipx was not installed by Homebrew." >&2
  exit 1
fi

pipx install --force cmakelang
pipx_bin_dir="$(pipx environment --value PIPX_BIN_DIR)"
export PATH="${pipx_bin_dir}:${PATH}"

if ! command -v opam >/dev/null 2>&1; then
  echo "opam is required to create the local OCaml switch." >&2
  exit 1
fi

if ! opam switch list --short --safe | grep -Fqx "${ocaml_root}"; then
  opam switch create "${ocaml_root}" 5.5.0 --yes
fi

opam install --switch="${ocaml_root}" "${ocaml_root}" --deps-only --with-dev-setup --yes
opam install --switch="${ocaml_root}" ocamlformat --yes

if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  pre-commit install --install-hooks
else
  echo "Git hooks will be installed after the Jujutsu/Git repository is initialized."
fi

cat <<'EOF'

Development toolchain installed.

Use just configure, just build, and just test for the C++ project.
Use just reasoner-build for the OCaml reasoner scaffold.
EOF
