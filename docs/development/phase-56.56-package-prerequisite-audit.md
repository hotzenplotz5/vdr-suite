# Phase 56.56 - Package Prerequisite Audit

## Navigation

- [Development Index](index.md)
- [Phase 56.55 - Install Manifest / Package File Contract](phase-56.55-install-manifest-package-file-contract.md)
- [ADR-0037: Packaging, Install Layout and API Boundary](../adr/ADR-0037-packaging-install-api-boundary.md)

---

## Status

Implemented for Phase 56 packaging-readiness work.

---

## Purpose

Phase 56.56 records the package prerequisite audit for the current install staging contract.

This audit separates build prerequisites, runtime prerequisites, optional integration prerequisites and explicitly deferred Debian packaging work.

---

## Build Prerequisites

Current source builds require:

- GNU make
- a C++17-capable compiler
- sqlite3 command-line tool for database preparation and test data loading
- SQLite development library and headers for linking with `-lsqlite3`
- Python 3 for repository validation tools
- standard POSIX install utility

These are build-time requirements for the current Makefile workflow.

---

## Runtime Prerequisites

The currently staged runtime artifacts require:

- Linux or compatible POSIX runtime environment
- SQLite runtime library
- readable configuration directory at `/etc/vdr-suite/`
- runtime access to the configured database path
- network access to configured VDR backends when real backend integration is enabled

---

## Optional VDR Integration Prerequisites

Real backend integration depends on the selected backend mode.

For RESTfulAPI-backed VDR access, the target VDR host needs:

- a reachable VDR instance
- vdr-plugin-restfulapi or a compatible RESTfulAPI endpoint
- matching endpoint behavior for channels, timers, recordings, events and status

For test or mock modes, no real VDR backend is required.

---

## Not Yet Introduced

Phase 56.56 does not introduce:

- `debian/control`
- `debian/rules`
- maintainer scripts
- systemd service files
- package dependency metadata
- final package names as Debian control stanzas
- automatic daemon activation
- public C++ ABI packaging
- `vdr-suite-dev`

---

## Packaging Boundary

The packaging boundary remains:

- install into `DESTDIR`
- do not write directly to the live host during package staging tests
- keep headers internal
- expose application behavior through the REST API, not through a public C++ ABI
- keep `vdr-suite-dev` out of Phase 56

---

## Current Verification Commands

The current validation set remains:

```bash
make test-install-staging
make test-docs
make test-phase
make test-phase-map-coverage
```

`make test-install-staging` verifies staged runtime, CLI, docs, data directory and manpage paths.

---

## Follow-Up

The final Phase 56 step is:

- Phase 56.57 - Phase 56 Completion Audit

After Phase 56 is complete, the documentation structure should be simplified so current state and historical phase records are easier to distinguish.

---

## Back

- [Back to Development Index](index.md)
