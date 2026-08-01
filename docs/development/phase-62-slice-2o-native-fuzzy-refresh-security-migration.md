# Phase 62 Slice 2O — Native Fuzzy Operator Refresh Security Migration

## Status

Repository implementation, CI and real yaVDR runtime acceptance are complete.

Accepted code/runtime head:

```text
a4f69e57de3cf8c29e915c7c5f6eb74a0c3fb9bc
```

GitHub Actions:

```text
VDR-Suite CI #6638
Run ID: 30706360256
Result: all five jobs successful
```

PR #117 remains open, Draft and unmerged.

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

## Real yaVDR Runtime Acceptance

Accepted on 2026-08-01 against the installed yaVDR runtime.

```text
repository_head=a4f69e57de3cf8c29e915c7c5f6eb74a0c3fb9bc
service_pid_before_install=64164
service_pid_after_install=65101
service_pid_after_acceptance=65101
installed_daemon_sha256=d0d7457b82ad55e19263efe255949856f28679ef95cde4bb86b755abd95c1a41
installed_loader_sha256=47c78c871c7caefed8eee2e11e14b8fda5edd3bf85a07f3f099ff24b0a88ce51
runtime_report_sha256=951598cbfac66ac86b658097bc9ee3fe05d6d5a72c3b753e37f42c3e74525aa9
```

Acceptance results:

```text
slice=slice-2o-native-fuzzy-refresh
tests_passed=29
tests_failed=0
runtime_http_requests=27
accountability_authorized=8
accountability_csrf=2
accountability_permission=2
accountability_read_only=2
accountability_scope=4
resource_state_unchanged=yes
target_grants_restored=yes
unrelated_grants_untouched=yes
browser_session_revoked=yes
revoked_cookie_replay_denied=yes
accountability_secret_free=yes
database_integrity=yes
service_pid_unchanged=yes
```

The acceptance confirmed:

- the deliberately absent backend remained unknown;
- the Native Fuzzy service stopped before probe creation;
- no SearchTimer mutation occurred;
- no backend capability update occurred;
- no Native Fuzzy persistence write occurred;
- the daemon PID did not change during acceptance;
- temporary grants were restored exactly;
- the acceptance browser session was revoked;
- revoked-cookie replay was denied;
- SQLite quick and foreign-key checks passed;
- backup and evidence checksums passed;
- the repository worktree remained clean.

```text
native_fuzzy_backend_known=false
native_fuzzy_create_attempted=false
real_native_fuzzy_probe_creates=0
real_searchtimer_mutations=0
backend_capability_updates=0
native_fuzzy_persistence_writes=0
```

Runtime backup and durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2o-20260801T163607Z-a4f69e57de3c
/var/backups/vdr-suite-phase62-slice2o-20260801T163607Z-a4f69e57de3c/runtime-acceptance-slice2o
```

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
