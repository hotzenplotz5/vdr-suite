# VDR-Suite New Chat Handoff

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Current State](CURRENT.md)
- [Roadmap](planning/roadmap.md)
- [ADR Index](adr/index.md)

---

## Purpose

This file is the short handoff that a new chat should read first.

It is intentionally compact and points to the current source documents instead of repeating the whole phase history.

---

## Required First Reading

A new chat should start with these files in this order:

1. [Current State](CURRENT.md)
2. [Roadmap](planning/roadmap.md)
3. [Phase Map](planning/phase-map.md)
4. [ADR Index](adr/index.md)
5. [Completed Phases](development/completed-phases.md)

Detailed phase notes should be opened only when a specific historical detail is needed.

---

## Current Repository Truth

Current completed project block:

```text
Phase 56 - Library Boundary, Packaging and Developer Documentation
```

Next implementation focus:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

---

## Phase 56 Result

Phase 56 is complete.

The important result is the install and boundary contract before real packaging.

Verified Phase 56 outcomes:

- source groups are split by responsibility
- the transitional recording-action aggregate is removed
- core/API boundary is documented
- REST API remains the application-facing boundary
- `make install DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr` stages installable files
- daemon, CLI, documentation, data directory and manpages are staged
- no public C++ ABI is promised
- no `vdr-suite-dev` package is introduced
- Debian packaging metadata is still deferred

---

## Phase 57 Direction

Phase 57 must start from the existing multi-backend architecture.

The goal is multi-site backend administration and permissions:

- support multiple VDR backend sites
- model backend-specific capabilities and permissions
- support read-only secondary-site mode
- keep operation policy explicit and guarded
- prepare frontend-visible backend and permission state

---

## Current Architecture Boundary

Current simplified architecture chain:

```text
RuntimeConfig
  -> VdrConfig
  -> VdrAdapterFactory
  -> IVdrAdapter
  -> ExternalVdrAdapter / MockVdrAdapter / RestfulApiVdrAdapter
  -> VdrService
  -> REST controllers
```

Boundary rule:

```text
Core modules may not depend on api/rest.
The REST API layer may depend on core modules.
```

---

## Documentation Rules

Use [Current State](CURRENT.md) for current truth.

Use [Roadmap](planning/roadmap.md) for planned direction.

Use [Phase Map](planning/phase-map.md) for compact phase-range coverage.

Use [Completed Phases](development/completed-phases.md) for milestone history.

Use `docs/development/phase-*` files only as historical implementation records.

The old `docs/roadmap/` directory is a forward-roadmap archive, not the current roadmap.

---

## Required Verification Before Declaring Work Complete

For documentation-only or guardrail-only changes:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

For install-layout changes:

```bash
make test-install-staging
make test-docs
make test-phase
make test-phase-map-coverage
```

For runtime-sensitive changes, also run the applicable runtime, daemon and real VDR acceptance checks.

GitHub Actions must be green before a pushed phase or reset block is considered complete.

---

## Current Known Documentation Cleanup

DOC-RESET-1 created the current-state entry point.

DOC-RESET-2 aligns roadmap, ADR index and completed-phase markers.

DOC-RESET-3 should add stronger documentation tooling checks for:

- current entry points
- ADR duplicate active numbers
- completed-phase marker alignment

---

## Back

- [Back to Current State](CURRENT.md)
- [Back to Documentation Index](index.md)
- [Back to README](../README.md)
