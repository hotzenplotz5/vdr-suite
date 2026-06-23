# SearchTimer User Workflow Foundation

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Backend Contract](searchtimer-backend-contract.md)
- [SearchTimer Real Payload Validation](searchtimer-real-payload-validation.md)
- [SearchTimer Real VDR Compatibility Report](searchtimer-real-vdr-compatibility-report.md)
- [EPGSearch Native Fuzzy Real-Backend Validation](epgsearch-native-fuzzy-real-backend-validation.md)

---

## Purpose

Phase 50.0 defines the user-facing SearchTimer workflow foundation.

The goal is to turn the validated backend and native fuzzy capability foundation into a coherent operator workflow before adding more automation.

This phase does not add new backend fields blindly.

It defines which workflow semantics are stable enough to expose to clients and operators.

---

## Existing Foundation

The following SearchTimer foundations already exist:

- backend-aware SearchTimer identity
- SearchTimer list route
- SearchTimer preview route
- SearchTimer create route
- SearchTimer update route
- SearchTimer delete route
- RESTfulAPI SearchTimer command executor
- real VDR create, readback, update and delete smoke validation
- native fuzzy capability validation through operator refresh, capability report and persisted restore

This means Phase 50 should focus on workflow shape, stable semantics and user safety.

---

## User Workflow Contract

The stable manual workflow is:

1. List existing SearchTimers.
2. Preview a candidate SearchTimer query against available EPG data.
3. Create a SearchTimer with validated stable fields.
4. Read back the created SearchTimer from the backend.
5. Update an existing SearchTimer through explicit backend-native identity.
6. Delete an existing SearchTimer through explicit backend-native identity.
7. Keep real VDR as the source of truth for SearchTimer state.

The workflow must remain understandable to an operator before later automation is added.

---

## Stable Workflow Inputs

The first stable workflow surface should prefer:

- backend id
- SearchTimer name
- query text
- active flag
- recording directory
- priority and lifetime
- start and stop margins
- VPS flag
- channel constraints
- time constraints
- duration constraints
- day-of-week constraints
- repeat handling
- comparison behavior
- blacklist behavior
- validated native fuzzy mode when backend capability allows it

Advanced epgsearch and Live-style options remain allowed only when they are already validated and documented.

---

## Safety Rules

- Never create or update a real backend SearchTimer without explicit operator intent.
- Preserve backend identity in all user workflow operations.
- Keep VDR and epgsearch as the source of truth after write operations.
- Read back backend state after create and update whenever possible.
- Make unsupported backend capability visible instead of silently downgrading behavior.
- Do not hide normalization done by RESTfulAPI or epgsearch.
- Keep automation out of scope until the manual workflow contract is stable.

---

## API Surface Baseline

Current route baseline:

    GET  /api/searchtimers
    GET  /api/searchtimers/preview
    POST /api/searchtimers
    POST /api/searchtimers/update
    POST /api/searchtimers/delete

The /api/vdr/searchtimers aliases remain compatibility routes.

Later phases may refine the route shape, but Phase 50.0 treats this as the current baseline to document and harden.

---

## Out of Scope for Phase 50.0

- SearchTimer automation loops
- automatic conflict resolution
- profile and permission enforcement
- full Live UI parity
- multi-backend frontend polish
- recommendation engine behavior
- persistent full EPG mirroring

These belong to later milestones after the manual workflow foundation is stable.

---

## Next Implementation Direction

Recommended next phase:

    Phase 50.1 - SearchTimer workflow request model

Purpose:

- define one backend-neutral request model for user workflow operations
- separate user workflow intent from raw RESTfulAPI transport fields
- keep validated backend-specific fields behind mapping boundaries
- prepare controller and client behavior for consistent manual workflow execution

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Development Index](index.md)
