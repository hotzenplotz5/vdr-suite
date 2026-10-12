# Build Requirements — source-backed entrypoint

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Developer Onboarding](development/developer-onboarding.md)
- [Isolated Developer Quickstart](development/quickstart.md)
- [Build-System State](development/build-system-state.md)
- [Packaging Status](../packaging/README.md)

## Current source of truth

The current root [Makefile](../Makefile) includes the build rules under [`mk/`](../mk/README.md); [`mk/common.mk`](../mk/common.mk) configures `g++`, C++17 compilation, `pkg-config`-based SQLite linking and local `.build/` artifacts. Actual library and tool requirements differ by target; do **not** treat the early short list below as a complete dependency manifest or infer a tested distribution support matrix from it.

The [Developer Quickstart](development/quickstart.md) gives a bounded example of installing the CI-derived packages in a **disposable** Ubuntu development environment; it is not a prescription to upgrade a running VDR host.

The [GitHub CI workflow](../.github/workflows/ci.yml) installs target-specific dependencies for its Ubuntu runner, including C/C++ build tools, SQLite, crypto, curl and FFmpeg development/runtime packages for the fast regression job. This is evidence of that CI job's build environment, **not** proof of general package compatibility or the installed yaVDR host.

For install staging and current Debian packaging readiness, use [packaging](../packaging/README.md) and [build-state](development/build-system-state.md). Verify exact Make targets and their prerequisites in the checked-out commit before any local build; this page authorizes no deployment.

## Initial planning requirements (historical, not certified minimums)

The original build sketch listed Debian 13+, Ubuntu 24.04+, yaVDR NG, GCC 13+, C++17, Git, Make and SQLite, with CMake/`pkg-config`/Doxygen envisaged for later. These were **planning statements**; no fresh distribution or version compatibility assessment was established by this GitHub-only documentation review.

## Back

- [Documentation Index](index.md)
- [README](../README.md)
