set shell := ["zsh", "-eu", "-o", "pipefail", "-c"]

default:
  @just --list

setup:
  pre-commit install --install-hooks

configure preset="dev":
  conan install . --lockfile=conan.lock --output-folder build/conan/{{preset}} --build missing -s build_type={{if preset == "ci" { "RelWithDebInfo" } else { "Debug" }}} -s compiler.cppstd=23
  cmake --preset {{preset}} -DCMAKE_TOOLCHAIN_FILE={{invocation_directory()}}/build/conan/{{preset}}/build/{{if preset == "ci" { "RelWithDebInfo" } else { "Debug" }}}/generators/conan_toolchain.cmake

build preset="dev":
  cmake --build --preset {{preset}}

test preset="dev":
  ctest --preset {{preset}}

format:
  find apps libs tests -type f \( -name '*.cpp' -o -name '*.hpp' \) -exec scripts/run-llvm-tool.sh clang-format -i {} +
  find . -type f \( -name CMakeLists.txt -o -name '*.cmake' \) -exec scripts/run-cmake-format.sh -i {} +
  if find ocaml \( -path ocaml/_build -o -path ocaml/_opam \) -prune -o -type f \( -name '*.ml' -o -name '*.mli' \) -print -quit | grep -q .; then find ocaml \( -path ocaml/_build -o -path ocaml/_opam \) -prune -o -type f \( -name '*.ml' -o -name '*.mli' \) -exec opam exec --switch "{{invocation_directory()}}/ocaml" -- ocamlformat --enable-outside-detected-project --inplace {} +; fi

lint:
  scripts/run-llvm-tool.sh clang-tidy -p build/dev apps/bernstein-agent/main.cpp
  cppcheck --enable=warning,style,performance,portability --std=c++23 --error-exitcode=1 apps libs

reasoner-build:
  opam exec --switch "{{invocation_directory()}}/ocaml" -- dune build --root ocaml

quality preset="dev":
  just configure {{preset}}
  just format
  just lint
  just test {{preset}}
