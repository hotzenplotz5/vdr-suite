# Phase 9.8 – Local Partial Refresh Validation

## Purpose

Phase 9.8 validates the Phase 9 partial snapshot refresh architecture against a real local VDR/RESTfulAPI setup.

The goal is to prove that a real backend-domain change is detected through change-state polling and executed as a domain-specific snapshot update.

---

## Verified Runtime Path

```text
VDR domain change
    ↓
RESTfulAPI change-state
    ↓
RestfulApiVdrAdapter
    ↓
VdrService
    ↓
PollingService
    ↓
ChangeDetectionService
    ↓
VdrChangeEvent
    ↓
SnapshotRefreshPlanner
    ↓
SnapshotUpdatePlan
    ↓
SnapshotCacheService domain update
    ↓
SnapshotCache
```

---

## Test Target

Optional target:

```text
make test-local-partial-refresh-validation
```

This target is intentionally not part of `make test` because it requires a real local VDR/RESTfulAPI setup and an interactive backend-domain change.

---

## Observed Validation Result

Initial snapshot:

```text
channels: 342
recordings: 973
timers: 0
events: 36272
```

After one real timer-domain change, the generated plan was:

```text
status: no
channels: no
recordings: no
timers: yes
events: no
full: no
```

Updated snapshot:

```text
channels: 342
recordings: 973
timers: 1
events: 36272
```

---

## Result

The validation proves that a real timer-domain change refreshes only the timer domain.

This confirms the core Phase 9 architecture goal:

```text
change-state polling
    ↓
change detection
    ↓
refresh planning
    ↓
partial snapshot refresh
```

---

## Status

Phase 9.8 validates partial refresh behavior against a real local VDR for the timer-domain path.

Milestone tag:

```text
v2.8-phase9-local-partial-refresh-validation
```

Recommended documentation tag:

```text
v2.8.1-phase9-doc-sync
```
