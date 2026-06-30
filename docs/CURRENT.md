# VDR-Suite Current State

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Development Documentation](development/index.md)
- [Roadmap](planning/roadmap.md)

---

## Start Here

This is the primary human entry point for the current state of the repository.

Use this file before reading historical phase notes.

---

## Current Verified State

Phase 56 is complete.

Current completed project block:

```text
Phase 56 - Library Boundary, Packaging and Developer Documentation
```

Verified outcomes:

- source groups are split by responsibility
- the transitional `ACTIONS_SRC` aggregate has been removed
- package and install boundaries are documented
- `make install` supports `DESTDIR` staging
- daemon, CLI, docs, data directory and manpages are staged
- manual page skeletons exist and are installed by the staging target
- no public C++ ABI is promised
- no `vdr-suite-dev` package is introduced
- Debian packaging metadata is still deliberately deferred

---

## Current Build and Install Contract

Canonical staging command:

```bash
make install DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr
```

Primary verification command:

```bash
make test-install-staging
```

Current staged runtime layout:

```text
/usr/sbin/vdr-suite-daemon
/usr/bin/vdr-suite-dashboard
/etc/vdr-suite/
/usr/share/doc/vdr-suite/README.md
/usr/share/vdr-suite/
/usr/share/man/man8/vdr-suite-daemon.8
/usr/share/man/man5/vdr-suite.conf.5
/usr/share/man/man1/vdr-suite-dashboard.1
```

---

## Current Runtime Binaries

```text
vdr-suite-daemon
  runtime daemon and HTTP API process

vdr-suite-dashboard
  current command-line dashboard entry point
```

`vdr-suite-dashboard` is currently CLI, not the future web/mobile frontend.

---

## Current Architecture Boundary

Core modules are internal implementation boundaries.

The REST API is the application-facing API boundary.

Rule:

```text
Core modules may not depend on api/rest.
The REST API layer may depend on core modules.
```

Current core areas:

- `core/sqlite`
- `core/recordings`
- `core/vdr`
- `core/http`
- `core/runtime`
- `core/daemon`

Application-facing API layer:

- `api/rest`

---

## Current VDR Backend Direction

The architecture remains multi-backend oriented:

```text
RuntimeConfig
  -> VdrConfig
  -> VdrAdapterFactory
  -> IVdrAdapter
  -> ExternalVdrAdapter / MockVdrAdapter / RestfulApiVdrAdapter
  -> VdrService
  -> REST controllers
```

A future multi-site administration and permission phase should build on this boundary instead of bypassing it.

---

## What Is Historical

Most files under `docs/development/phase-*` are implementation records.

They are useful for traceability, but they are not the best place to start when asking:

```text
What is true now?
```

Use this file first, then follow links to the specific current or historical document you need.

---

## Most Useful Current Links

Project state:

- [Current Project Status](development/current-status.md)
- [Project Status Dashboard](project-status-dashboard.md)
- [Roadmap](planning/roadmap.md)

Phase 56 closure:

- [Phase 56 Completion Audit](development/phase-56.57-completion-audit.md)
- [Install Manifest / Package File Contract](development/phase-56.55-install-manifest-package-file-contract.md)
- [Package Prerequisite Audit](development/phase-56.56-package-prerequisite-audit.md)
- [ADR-0037: Packaging, Install Layout and API Boundary](adr/ADR-0037-packaging-install-api-boundary.md)

Developer entry points:

- [Development Documentation](development/index.md)
- [Build System State](development/build-system-state.md)
- [Architecture Map](development/architecture-map.md)
- [ADR Index](adr/index.md)

---

## Next Work Block

Next work block:

```text
DOC-RESET-1 - Documentation Usability Reset
```

Purpose:

- make the current state easy to find
- separate current truth from historical phase records
- reduce confusion from outdated index snapshots
- add a guard so this entry point stays linked

After that:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
