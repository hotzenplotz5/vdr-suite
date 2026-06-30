# VDR-Suite Current State

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Development Documentation](development/index.md)
- [Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
- [ADR Index](adr/index.md)

---

## Start Here

This is the primary human entry point for the current state of the repository.

Use this file before reading historical phase notes.

For new ChatGPT sessions or context loss, start with [New Chat Handoff](NEW-CHAT-HANDOFF.md).

---

## Current Verified State

Current completed project block:

```text
Phase 56 - Library Boundary, Packaging and Developer Documentation
```

Verified outcomes:

- source groups are split by responsibility
- package and install boundaries are documented
- `make install` supports `DESTDIR` staging
- daemon, CLI, docs, data directory and manpages are staged
- manual page skeletons exist and are installed by the staging target
- no public C++ ABI is promised
- no `vdr-suite-dev` package is introduced
- Debian packaging metadata is still deliberately deferred
- the current-state documentation entry point has been reset

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

---

## Current Runtime Entry Points

```text
/usr/sbin/vdr-suite-daemon
/usr/bin/vdr-suite-dashboard
```

`vdr-suite-dashboard` is currently CLI, not the future web/mobile frontend.

---

## Current Architecture Boundary

The REST API is the application-facing API boundary.

Core modules remain internal implementation boundaries.

```text
Core modules may not depend on api/rest.
The REST API layer may depend on core modules.
```

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

---

## Current Links

- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Completed Phases](development/completed-phases.md)
- [ADR Index](adr/index.md)
- [Phase 56 Completion Audit](development/phase-56.57-completion-audit.md)

---

## Next Work

Next implementation focus:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
