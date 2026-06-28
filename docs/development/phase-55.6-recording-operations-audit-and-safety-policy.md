# Phase 55.6 - Recording Operations Audit and Safety Policy

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Phase Map](../planning/phase-map.md)
- [Roadmap](../planning/roadmap.md)
- [Real Recording Action End-to-End Validation](real-recording-action-e2e-validation.md)
- [Real Recording Action Regression Audit](real-recording-action-regression-audit.md)

---

## Purpose

Phase 55.6 audits recording operations before expanding any destructive or mutating real-backend behaviour.

The goal is not to add new mutation paths. The goal is to make the existing recording action safety state explicit and define which gates must remain mandatory before future recording-operation implementation continues.

---

## Current Code Surface

Current recording action request state:

- recording actions are backend-scoped through `backendId`
- recording actions carry a `recordingId`
- action requests default to `dryRun = true`
- additional backend-native parameters are carried in `parameters`

Current supported action enum includes:

```text
Check
Repair
Shrink
Cut
Pes2Ts
Rename
Move
Delete
MetadataRefresh
Unknown
```

Current implemented RESTfulAPI mutation executor surface:

```text
Move
Rename
Delete
```

Current RESTfulAPI request endpoints:

```text
POST /recordings/move.json
POST /recordings/delete.json
```

---

## Existing Safety Foundation

The existing safety model already has important defaults:

- dry-run is the request default
- real execution is disabled unless explicitly allowed
- read-only backend policy blocks mutation
- missing backend capability blocks execution
- missing permission blocks execution
- unavailable backend blocks execution
- unsupported action blocks execution
- recording-in-use can block execution when supplied by context

The backend policy defaults are important:

- `readOnlyPolicy(...)` sets `readOnly = true` and `executionAllowed = false`
- `restfulApiMutationPolicy(...)` sets `executionAllowed = true`, but must remain an explicit policy choice

---

## Real-VDR Evidence Already Recorded

Existing documentation already records real VDR validation for:

```text
Recording Delete
Recording Rename
Recording Move
```

The real validation established two important identity rules:

- VDR/RESTfulAPI recording actions require backend-native recording identity.
- VDR-Suite must treat `backendId + backendNativeId` as the long-term routing identity for mutating recording operations.

The real validation also established a frontend rule:

- clients must reload recording lists after mutating recording actions because recording ids and list positions can change.

---

## Safety Policy for Phase 55.6

### Allowed by default

The following remain allowed by default:

- read-only recording listing
- read-only recording query
- dry-run action validation
- dry-run action planning
- request preview generation
- safety-policy evaluation
- documentation and guardrail checks

### Not allowed by default

The following remain blocked by default:

- real recording move
- real recording rename
- real recording delete
- automatic destructive real-VDR regression
- frontend-triggered mutation without explicit backend policy
- mutation on read-only secondary backends

### Required before any real mutation

Any real recording mutation must require all of the following:

```text
backendId present
backendNativeId or validated backend-native recording path present
action supported by backend capability policy
permission present
backend not read-only
executionAllowed true
dryRun false explicitly requested
operator/test gate present for real helper execution
post-action recording list refresh/readback planned
```

For destructive helper execution against a real VDR, the earlier test-recording marker rule remains mandatory:

```text
VDR-SUITE-TEST
```

---

## Audit Finding

Phase 55.6 confirms that the project already has a useful recording-action safety foundation, but it is not yet a product-grade recording-operations policy.

The remaining gap is not basic move/rename/delete execution. The remaining gap is policy completeness:

- clear separation of safe read-only APIs from mutating APIs
- explicit backend policy selection in daemon/API wiring
- explicit frontend-visible mutation availability
- stable post-mutation refresh/readback expectations
- read-only secondary-site enforcement
- no automatic destructive regression without a safe fixture strategy

---

## Follow-up Implementation Direction

Next implementation work should be narrow and safety-first:

1. expose or document recording operation safety state for clients
2. keep mutating execution behind explicit backend policy
3. require backend-native identity for RESTfulAPI mutations
4. require post-mutation recording refresh/readback semantics
5. keep destructive real-VDR helper execution opt-in and marker-restricted

No production default should silently open real recording mutation.

---

## Verification

Documentation and guardrail verification for this phase:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
make test-real-vdr-acceptance-manifest
make test-daemon-runtime-shutdown-resets
make test-http-listener-bind-failure-handling
```

Runtime/API changes in later implementation phases must additionally run the applicable daemon and real VDR acceptance checks.

---

## Back

- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
