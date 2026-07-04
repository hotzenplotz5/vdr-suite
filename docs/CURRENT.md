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

---

## Current Verified State

Current completed major project block:

- Phase 57 - Multi-Site Backend Administration and Permissions

Previous completed major project block:

- Phase 56 - Library Boundary, Packaging and Developer Documentation

Latest completed implementation slice:

- Phase 58.90b - Stable Channel Sorter

Verified Phase 57 outcomes:

- backend access modes for read-write and read-only sites
- backend registry JSON permission hints
- recording action backend access handling
- timer action backend access coverage
- SearchTimer backend access coverage
- frontend-visible backend permission state

Verified Phase 58.90b slice outcomes:

- guarded backend channel move API
- stable frontend channel sorting module
- desktop and touch pointer-drag support
- handle-only drag so normal list scrolling remains usable
- no focus-restore experiment in the stable state

---

## Current Build and Install Contract

Canonical staging command:

- make install DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr

Primary verification command:

- make test-install-staging

---

## Current Runtime Entry Points

- /usr/sbin/vdr-suite-daemon
- /usr/bin/vdr-suite-dashboard

`vdr-suite-dashboard` is currently CLI, not the future web/mobile frontend.

The web frontend is currently a Phase 58 foundation with stable channel sorting and selected VDR views.

---

## Current Architecture Boundary

The REST API is the application-facing API boundary.

Core modules remain internal implementation boundaries.

- Core modules may not depend on api/rest.
- The REST API layer may depend on core modules.

---

## Current VDR Backend Direction

The architecture remains multi-backend oriented:

- RuntimeConfig
- VdrConfig
- VdrAdapterFactory
- IVdrAdapter
- ExternalVdrAdapter / MockVdrAdapter / RestfulApiVdrAdapter
- VdrService
- REST controllers

---

## Current Links

- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Completed Phases](development/completed-phases.md)
- [ADR Index](adr/index.md)

---

## Next Work

Next implementation focus:

- Phase 58 - Frontend and Live Parity

Immediate follow-up:

- continue frontend/live-parity consolidation after the stable channel sorter
- do not re-introduce the broken post-move focus-restore experiment without a separate guarded implementation and test path

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
