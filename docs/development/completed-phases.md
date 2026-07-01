# VDR-Suite Completed Phases

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases Archive](completed-phases/README.md)
- [Completed Phases Latest Marker](completed-phases-latest.md)

---

## Purpose

This file is the compact entry point for completed implementation phases.

It provides a milestone-oriented overview and links detailed historical ranges to the completed phases archive.

Current status belongs to:

- [Current Project Status](current-status.md)

Future planning belongs to:

- [Roadmap](../planning/roadmap.md)
- [Milestones](../planning/milestones.md)

Detailed historical archives belong to:

- [Completed Phases Archive](completed-phases/README.md)

The latest completed implementation marker is tracked independently in:

- [Completed Phases Latest Marker](completed-phases-latest.md)

---

## Completed Milestones Overview

### Library Boundary, Packaging and Developer Documentation

Status: Completed.

Completed phases:
- Phase 57 - Multi-Site Backend Administration and Permissions.
- Phase 57.9 - Completion Audit.
- Phase 56 - Library Boundary, Packaging and Developer Documentation.
- Phase 56.57 - Phase 56 Completion Audit.

Key outcomes:
- Core/API boundaries are documented, with REST as the application-facing API boundary.
- Source groups are split by responsibility.
- The transitional `ACTIONS_SRC` aggregate has been removed.
- `make install DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr` stages runtime, CLI, docs, data directory and manpages.
- Initial daemon, config and CLI manpages exist and are installed by the staging target.
- Install manifest ownership and package prerequisites are documented.
- No public C++ ABI or `vdr-suite-dev` package is introduced.

---

### Recording Operations Audit and Safety Policy

Status: Completed.

Purpose:
- Audit live recording operations before expanding destructive real-backend recording actions.
- Preserve dry-run, read-only and opt-in safety as the default behaviour.
- Record guardrail coverage for recording mutation safety policy.

Completed phases:
- Phase 55.6 - Recording operations audit and safety policy.

Key outcomes:
- Real recording move, rename and delete paths remain explicitly gated.
- Destructive real VDR recording probes remain opt-in and marker-restricted.
- Recording mutation safety policy is documented and wired into local test groups.
- Phase 56 is now completed as the packaging and library-boundary block.
