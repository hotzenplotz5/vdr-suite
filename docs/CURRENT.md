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
Phase 57 - Multi-Site Backend Administration and Permissions
```

Verified outcomes:

- backend access modes for read-write and read-only sites
- backend registry JSON permission hints
- recording action backend access handling
- timer action backend access coverage
- SearchTimer backend access coverage
- frontend-visible backend permission state

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
- [Phase 57 Completion Audit](development/phase-57.9-completion-audit.md)

---

## Next Work

Next implementation focus:

```text
Phase 58 - Frontend and Live Parity
```

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
