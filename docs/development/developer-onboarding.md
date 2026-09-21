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

For documentation validation, use:

```bash
make test-docs
```

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

Every documentation page must provide:

- a top-level `#` heading
- a `## Navigation` section
- a `## Back` section

New documentation must be linked from the local directory index.

If it is part of a new documentation area, it must also be reachable from the root README through `docs/index.md` and the relevant local index.

The documentation checks are:

```bash
make test-docs
```

This runs:

- `tools/check_docs.py`
- `tools/check_doc_indexes.py`
- `tools/check_doc_reachability.py`

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

Current backend-related foundations include:

- backend identity
- backend-aware snapshot read metadata
- backend-aware snapshot access lookup
- `BackendNode`
- `BackendRegistry`

The long-term direction is to support multiple VDR backends, for example:

```text
BackendRegistry
  ├─ wohnzimmer
  ├─ ferienhaus
  └─ testsystem
```

Runtime integration, permissions, health and multi-backend polling must be introduced incrementally.

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
