# VDR-Suite Build and Documentation Test State

## Navigation

- [README](../../README.md)
- [Developer Onboarding](developer-onboarding.md)
- [Current Architecture](current-architecture-state.md)
- [Current State](../CURRENT.md)
- [Development Index](index.md)

## Source of truth

The top-level [Makefile](../../Makefile) composes C++ source groups and targets through many `mk/*.mk` fragments; it is not a single, independently maintained list of fixed modules. [`mk/common.mk`](../../mk/common.mk) sets the C++17 build flags, `.build` output default and shared compiler variables. [`mk/local-test-groups.mk`](../../mk/local-test-groups.mk) composes test groups.

The former Phase-10.21.1 snapshot is retained as historical context; use the current Makefile and included fragments to establish exact target dependencies before running commands.

## Current documentation checks

[`mk/maintenance-tests.mk`](../../mk/maintenance-tests.mk) defines `make test-docs`. Its recipe includes:

1. `tools/check_docs.py` for document headings and links in `docs/`.
2. `tools/check_public_v1_openapi.py` plus focused `tools/tests/test_public_v1_openapi.py` unit tests, guarding only the documented Public-v1 subset.
3. `tools/check_doc_indexes.py`, `tools/check_doc_reachability.py` and `tools/check_doc_entrypoints.py`.
4. `tools/check_adr_index.py`, `tools/check_mu5_administration_architecture.py`, `tools/check_p2_first_admin_bootstrap_boundary.py` and `tools/check_completed_phase_markers.py`.

These are source-tree quality checks, **not** proof that the full HTTP surface has an OpenAPI schema or that a formal external OpenAPI validator, live runtime or installed TV client passed acceptance.

The existing [GitHub CI workflow](../../.github/workflows/ci.yml) defines a `docs-check` job that invokes `make -j2 --output-sync=target test-docs test-phase` on configured events. Its presence is not evidence that a run succeeded for this documentation branch.

## Build and execution boundaries

- `make daemon` is a source build target, not deployment or live-service acceptance.
- `make test-architecture` and `make test-phase` have distinct validation responsibilities in the included makefile.
- `make clean` and test database setup may affect local build/test artifacts; review their actual recipes and output destinations before running them, especially on a real yaVDR host.
- Installation staging, deployment and packaging are separate contracts: [packaging](../../packaging/README.md) and [developer onboarding](developer-onboarding.md).
- There is no CI or local-test pass claimed by this text-only documentation reconciliation.

## Back

- [Development Index](index.md)
- [Documentation Index](../index.md)
