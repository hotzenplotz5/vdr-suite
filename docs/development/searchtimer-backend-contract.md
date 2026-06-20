# SearchTimer Backend Contract

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Completed Phases](completed-phases.md)
- [Roadmap](../planning/roadmap.md)

## Back

- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Completed Phases](completed-phases.md)

---

---

## Purpose

This document records the backend contract for SearchTimer integration.

Phase 47.14 documents the contract before expanding the SearchTimer domain model.

The contract keeps SearchTimer work aligned with the VDR-Suite architecture rule:

- Prefer backend-neutral boundaries.
- Keep VDR and RESTfulAPI as backend sources of truth.
- Avoid modelling backend-specific data blindly.

---

## Current Implemented Runtime Chain

    /api/searchtimers
        -> ApiRouter
        -> SearchTimerController
        -> ISearchTimerDataSource
        -> RestfulApiSearchTimerAdapter
        -> RESTfulAPI /searchtimers.json

The route is already wired to a daemon-side backend data source.

The daemon creates a SearchTimer adapter from the per-backend runtime context.

---

## Current Domain Model Scope

The current SearchTimer foundation models the minimal stable contract:

    backendId
    backendNativeId
    name
    query
    state

This is sufficient for:

- listing SearchTimers
- backend-aware identity
- filtering by backend
- filtering by active or inactive state
- filtering by text
- limit and offset pagination
- JSON serialization for client-facing API responses

---

## RESTfulAPI Mapping Scope

The current RESTfulAPI mapper reads SearchTimer objects from the backend response and maps:

    id                  -> backendNativeId
    search              -> name
    search              -> query
    use_as_searchtimer  -> state

The backend id is supplied by the runtime adapter and is not inferred from the backend payload.

---

## Known Backend Contract Gap

epgsearch and Live-style SearchTimers can contain more fields than the current VDR-Suite model.

Potential future fields include:

    directory
    priority
    lifetime
    channel filter
    channel group
    search mode
    case sensitivity
    start margin
    stop margin
    repeat handling
    compare title
    compare subtitle
    compare description
    blacklists
    recordings keep policy

These fields must not be added blindly.

They should be added only after validating real RESTfulAPI and epgsearch payloads on yaVDR.

---

## Contract Decision

Phase 47.14 keeps the SearchTimer domain intentionally minimal.

The current model is the stable list/query contract.

Future phases may expand it after real backend payload validation.

---

## Follow-up Implementation Direction

Recommended next steps:

    Phase 47.15 - SearchTimer real payload validation
    Phase 47.15 - Capture and validate real RESTfulAPI SearchTimer payloads before domain expansion
    Phase 47.16 - SearchTimer domain model expansion
    Phase 47.17 - SearchTimer mapper expansion
    Phase 47.18 - SearchTimer JSON contract expansion

---

## Verification

This document is valid when these checks pass:

    make test-docs
    make test-phase
