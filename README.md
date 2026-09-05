# Bernstein

Bernstein is an architecture-aware coding agent for teams that want humans to retain ownership of design while delegating investigation, implementation, and verification to a capable agent.

The project uses a colocated Jujutsu/Git repository for product code. The [inception/](inception/) documents are intentionally excluded from that repository and maintained as a nested Mercurial repository.

## Layout

~~~text
apps/bernstein-agent/  First executable: the local coding agent.
apps/bernstein-sdlc/   Reserved for a later SDLC/control-plane executable.
libs/                  C++ interface targets and future implementation libraries.
ocaml/                 Pure architecture and policy reasoner package.
protocol/              C++/OCaml boundary documentation and future schemas.
inception/             Private Mercurial-managed design material.
~~~

## Development

Install the complete development toolchain with one script:

~~~sh
./scripts/bootstrap-dev.sh
~~~

Then run:

~~~sh
just setup
just configure
just build
just test
~~~

just configure resolves the pinned Conan dependencies and generates a CMake/Ninja development build. just quality formats, lints, and tests the C++ project. just reasoner-build validates the OCaml/Dune package once its dependencies are installed.

The initial executable intentionally does no work. This commit establishes the build, dependency, quality, repository, and language-boundary infrastructure only.
