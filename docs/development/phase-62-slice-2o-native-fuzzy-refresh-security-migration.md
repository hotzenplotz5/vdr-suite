# Phase 62 Slice 2O — Native Fuzzy Operator Refresh Security Migration

## Status

Repository implementation is active on Draft PR #117.

No yaVDR installation or runtime acceptance has been performed for this slice.

---

## Scope

Slice 2O protects the existing Native Fuzzy operator refresh aliases:

```text
POST /api/epgsearch/native-fuzzy/refresh
POST /api/vdr/epgsearch/native-fuzzy/refresh
  -> epgsearch.native-fuzzy.refresh@<backend-id>
```

The backend scope is read from the JSON body field `backendId`. Missing or empty
values resolve to `default`, matching the controller request parser.

Query strings remain supported because route classification uses the normalized
request path. Trailing-slash variants remain fail-closed.

---

## Security Contract

For browser-session requests the server performs:

1. browser credential and lifecycle validation;
2. persisted actor-grant resolution;
3. exact route classification;
4. cookie-bound CSRF validation;
5. exact `epgsearch.native-fuzzy.refresh` authorization for the body backend;
6. append-only pre-dispatch accountability;
7. existing controller and operator-refresh service handling.

There is currently no Webfrontend owner for this operator endpoint. Therefore
Slice 2O adds no global JavaScript fetch wrapper and no route-foreign frontend
test. A browser client must obtain the session CSRF token through the existing
browser-session API and submit it explicitly.

Legacy and Managed Basic compatibility remain supported without browser CSRF.

---

## Fixed Roles

`role.admin@<backend-id>` grants
`epgsearch.native-fuzzy.refresh` only for the same concrete backend.

`role.read-only@<backend-id>` denies the refresh before direct grants or Admin
expansion.

Wildcard role rows remain ineffective for a concrete backend.

---

## Runtime-Safe Acceptance Boundary

Native Fuzzy operator refresh can create a temporary SearchTimer, execute
readback, delete the probe, persist capability evidence and update backend
capabilities. Routine acceptance must not reach any of those steps.

The checked-in profile therefore uses a guaranteed non-registered backend:

```text
phase62-slice2o-missing-backend
```

The static request body is:

```json
{
  "backendId": "phase62-slice2o-missing-backend",
  "probeQuery": "Phase 62 Slice 2O Acceptance",
  "tolerance": 2,
  "keepProbeSearchTimer": false,
  "updateBackendCapabilities": false
}
```

The existing service resolves the backend registry before probe creation. The
expected response is HTTP 404 with:

```text
backendId = phase62-slice2o-missing-backend
backendKnown = false
createAttempted = false
persisted = false
backendCapabilitiesUpdated = false
status = backend-not-found
errors = ["backend not found: phase62-slice2o-missing-backend"]
```

This proves the route reaches the existing service boundary while stopping
before command executor use, SearchTimer creation, repository persistence or
capability mutation.

Profile and adapter:

```text
tools/phase62-runtime-acceptance/slice-2o-native-fuzzy-refresh.json
tools/phase62-runtime-acceptance/static-body-runner.py
```

The adapter reuses the existing runtime harness and only broadens manifest
validation for a bounded static object. It requires:

- a non-empty JSON object;
- serialized size at most 2048 bytes;
- `safeBody.backendId` exactly equal to the authorization `backendId`;
- no `operationId` field.

Existing `{}` profiles remain on the original runner unchanged.

---

## Focused Tests

```text
core/security/tests/test_native_fuzzy_refresh_security.cpp
tools/check_native_fuzzy_refresh_security.py
```

They cover:

- both exact aliases;
- Legacy Basic compatibility;
- browser CSRF enforcement;
- direct permission and exact backend scope;
- default backend resolution;
- query-string normalization;
- trailing-slash fail-closed behavior;
- exact Admin allowance and wildcard Admin denial;
- Read-only precedence;
- canonical, secret-free accountability;
- exclusion from the safe-POST allowlist;
- static-body manifest scope equality and size limits.

---

## Explicitly Deferred

Slice 2O does not migrate:

```text
POST /api/searchtimers/preview/cache/refresh
POST /api/vdr/searchtimers/preview/cache/refresh
POST /api/epg/cache/refresh
POST /api/epgsearch/native-fuzzy/stale-probes/delete
POST /api/vdr/epgsearch/native-fuzzy/stale-probes/delete
```

The two cache families obtain backend scope from query parameters and have
different state and rollback semantics. Global stale-probe deletion is not a
backend-scoped mutation. They require separate contracts.

No real Native Fuzzy probe, SearchTimer creation, capability update, generic
role administration, credential lifecycle, Phase 63+ runtime, PR-ready
transition or merge is included.

PR #117 remains open, Draft and unmerged.
