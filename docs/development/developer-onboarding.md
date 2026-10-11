# Developer Onboarding

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)

---

## Purpose

This document gives new contributors a practical entry point into VDR-Suite development.

It explains the current development workflow, the repository structure, the quality gates and the documentation rules that must be respected when changing the project.

---

## Project Principles

VDR-Suite is a modern service-oriented backend around the Video Disk Recorder ecosystem.

The project is designed to remain:

- VDR-centric
- backend-neutral
- daemon-driven
- snapshot-oriented
- capability-aware
- prepared for multi-backend and multi-VDR environments

VDR remains the primary backend domain and source of truth.

VDR-Suite complements VDR. It does not replace it.

---

## Repository Areas

Important top-level areas:

- `core/sqlite` - SQLite persistence foundation
- `core/recordings` - recording, metadata, jobs and actions
- `core/http` - HTTP abstractions and test server support
- `core/vdr` - VDR domain, adapters, snapshots, backend identity and backend registry foundations
- `core/runtime` - runtime logging and diagnostics
- `core/daemon` - daemon runtime wiring
- `api/rest` - REST controllers and routing
- `apps/daemon` - daemon executable entry point
- `mk` - shared Makefile fragments and local test groups
- `docs` - project documentation, architecture documents and ADRs
- `tools` - repository validation and documentation checks

---

## Source-linked area entrypoints

For current code ownership, start with the relevant repository-area README rather than an early Phase-1 sketch:

- [Core C++ domains and runtime owners](../../core/README.md)
- [Database and schema boundaries](../../database/README.md)
- [Runtime staging and packaging assets](../../packaging/README.md)
- [Historical service designs and their modern owners](../../services/README.md)
- [Read-only repository checks and potentially mutating tools](../../tools/README.md)
- [Partial Public-v1 API and checked OpenAPI](../../api/README.md)

These links are documentation entrypoints; do not deploy to a live system from an unverified checkout.

## Development Workflow

Before making recommendations or code changes, inspect the real repository state.

The expected workflow is:

```bash
git pull
git log --oneline -5
git status
```

For local feature validation, use focused test targets first:

```bash
make test-fast
make test-vdr
```

For documentation validation in a verified, isolated source checkout, use `make test-docs`. This checks more than link structure, including a source-scoped partial OpenAPI drift guard; it does not install or deploy anything.

The full regression test is:

```bash
make test
```

The full regression test is also executed by GitHub Actions.

---

## Test Targets

Common test targets:

- `make test-fast` - fast local smoke test
- `make test-vdr` - VDR, snapshot, adapter, backend identity and backend registry tests
- `make test-docs` - documentation structure, local indexes and reachability checks
- `make test-architecture` - architecture validation scripts
- `make test-phase` - phase consistency validation
- `make test` - full regression test

For architecture phases, at least these should be considered mandatory:

```bash
make test-vdr
make test-docs
```

---

## Documentation Rules

Documentation is part of the definition of done for architecture work.

The current `tools/check_docs.py` enforces a top-level `#` heading and resolvable local links for Markdown under `docs/` (with historical relocation exceptions). It does **not** require literal `## Navigation` or `## Back` headings on every page; consistent navigation is still encouraged for current entrypoints.

`tools/check_doc_indexes.py` checks local index coverage, with stricter archive rules. `tools/check_doc_reachability.py` checks the canonical/relevant archive reachability graph starting from the root README. New current-facing documents should be linked from appropriate indexes.

If it is part of a new documentation area, it must also be reachable from the root README through `docs/index.md` and the relevant local index.

The documentation checks are:

```bash
make test-docs
```

The `mk/maintenance-tests.mk` target currently runs:

- `tools/check_docs.py`, `tools/check_doc_indexes.py`, `tools/check_doc_reachability.py`, and `tools/check_doc_entrypoints.py`
- `tools/check_public_v1_openapi.py` and the scoped `tools/tests/test_public_v1_openapi.py` unit tests
- ADR, MU.5 administration, first-admin bootstrap, and completed-phase marker guards.

This is **not** a formal OpenAPI schema-validation pass and does not prove complete route coverage.

---

## Architecture Rules

Architecture changes should be small, testable and backward-compatible.

Important rules:

- Do not introduce classes before checking existing architecture.
- Do not introduce methods before checking existing interfaces.
- Do not bypass adapter boundaries.
- Keep backend-specific code behind backend adapters.
- Keep domain objects backend-neutral.
- Prefer snapshot reads for frontend-facing runtime state.
- Prefer capability checks before exposing backend-specific behavior.
- Prefer small architecture steps over large rewrites.

---

## Backend and Multi-VDR Direction

Implemented backend foundations include identity/registry and backend-scoped snapshot access, plus the enrolled Backend Agent, bounded observation and durable command/result transport, and actor/backend-scoped permissions. See [current architecture](current-architecture-state.md) and [CURRENT](../CURRENT.md) for the boundary between established implementation and current acceptance.

An illustrative multi-backend arrangement is:

```text
BackendRegistry
  ├─ wohnzimmer
  ├─ ferienhaus
  └─ testsystem
```

This diagram is illustrative, not a deployed topology. Runtime health, provider eligibility, actor permissions and per-backend execution must follow the implemented Agent and authorization contracts; do not infer availability from registry entries.

---

## Commit Discipline

For implementation phases, prefer:

1. architecture check
2. focused implementation
3. focused tests
4. documentation update when needed
5. `make test-vdr`
6. `make test-docs`
7. `git diff`
8. commit
9. push
10. verify `git status`

Do not treat a GitHub write as successful unless the resulting commit is visible or the local repository confirms it after `git pull`.

---

## Back

- [Back to Development Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
