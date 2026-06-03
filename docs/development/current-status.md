# VDR-Suite – Current Project Status

## Project

VDR-Suite

Goal:

Modern service-oriented backend architecture for VDR recordings, metadata management, job processing, dashboard services, JSON export, REST API, Web UI, OSD integration and future integration of VDR-Rectools.

---

## Current Branch

`phase-2-actions`

---

## Current Verified Head

`06667cf`

Fix modular Makefile include conversion.

Documentation split commits after that head:

- `557545e` – Docs: update README after snapshot architecture introduction
- `dc000eb` – Docs: split completed phase history from current status
- `15c7e67` – Docs: split milestone history from current status
- `e668287` – Docs: add snapshot architecture document

---

## Last Completed Development State

Latest verified implementation state before the documentation split:

- Phase 8.69: PollingService interface
- Phase 8.70: PollingService implementation
- Phase 8.72: extract VDR source list into make include
- Phase 8.74: extract VDR test targets into make include
- Phase 8.75: extract HTTP source list into make include
- Phase 8.76: extract daemon source list into make include
- Phase 8.77: extract recording source lists into make include
- Phase 8.78: extract action and job source lists into make include
- Phase 8.79: initial root Makefile include conversion
- Fix: `06667cf` – Fix modular Makefile include conversion

---

## Current Architecture Direction

VDR-Suite is moving from live RESTfulAPI access per API request toward a daemon-owned snapshot architecture.

Current target chain:

```text
RESTfulAPI
    ↓
RestfulApiVdrAdapter
    ↓
IVdrAdapter
    ↓
VdrService
    ↓
VdrSnapshotBuilder
    ↓
PollingService
    ↓
Snapshot Cache
    ↓
API Responses
```

Purpose:

- keep RESTfulAPI behind the adapter boundary
- avoid repeated live RESTfulAPI calls per API request
- prepare daemon-owned refresh cycles
- prepare future multi-VDR and permission-aware designs
- keep API controllers backend-independent

---

## Implemented Snapshot Components

Implemented:

- `VdrSnapshot`
- `VdrSnapshotBuilder`
- `PollingService`

Verified targets:

```bash
make daemon
make test-vdr-snapshot-builder
make test-polling-service
```

---

## Current Known Technical Debt

Duplicate target warnings exist because VDR test targets currently exist in two places:

```text
Makefile
mk/vdr-tests.mk
```

The build works, but the duplicate root Makefile VDR targets must be removed.

---

## Next Planned Phase

### Phase 8.80 – remove duplicate VDR test targets

Scope:

- remove VDR test targets from root `Makefile`
- keep canonical VDR test targets in `mk/vdr-tests.mk`
- keep build functional

Required verification:

```bash
make daemon
make test
```

Commit target:

```text
Phase 8.80: remove duplicate VDR test targets
```

---

## Upcoming Phases

### Phase 8.81 – PollingService integration in DaemonRuntime

Integrate `PollingService` into daemon runtime wiring.

### Phase 8.82 – Snapshot refresh cycle

Introduce periodic snapshot refresh.

### Phase 8.83 – Daemon snapshot cache

Introduce daemon-owned snapshot cache access for future API responses.

---

## Split Documentation

Long-running historical sections were moved out of this status file.

See:

- `docs/development/completed-phases.md`
- `docs/development/milestones.md`
- `docs/architecture/snapshot-architecture.md`

---

## Project Rules

- Architecture first.
- Read existing code before code changes.
- No placeholders.
- No dummy implementations.
- No permanent single-VDR assumption.
- Prefer complete files for local changes.
- Use nano-compatible workflows for local instructions.
- No `cat <<EOF` blocks in local instructions.
- Keep builds working after each small change.
- Run tests before code commits when local build access is available.
