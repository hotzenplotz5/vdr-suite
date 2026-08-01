# Phase 62 Slice 2N — SearchTimer Execution Security Migration

## Status

Slice 2N is complete on Draft PR #117.

Repository implementation, full CI, guarded yaVDR installation and
mutation-free runtime acceptance passed on 2026-08-01 at:

```text
0c4893a1afa5059af016b6c7336f6dc9a9801c82
```

---

## Scope

Slice 2N protects the existing SearchTimer workflow execution family:

```text
POST /api/searchtimers/execute
POST /api/vdr/searchtimers/execute
POST /api/searchtimers/real-test
POST /api/vdr/searchtimers/real-test
  -> searchtimers.execute@<backend-id>
```

Missing or empty `backendId` resolves to `default` for authorization. Query
strings remain supported. Trailing-slash variants remain fail-closed.

`real-test` performs no real backend mutation, but it exercises execution-policy
and operator-facing workflow paths. It therefore uses the same permission,
CSRF and Read-only boundary as `execute` instead of being classified as a safe
POST.

---

## Browser Security Contract

For browser-session requests the server performs:

1. browser credential and lifecycle validation;
2. persisted actor-grant resolution;
3. exact route classification;
4. cookie-bound CSRF validation;
5. `searchtimers.execute` authorization for the concrete backend;
6. append-only pre-dispatch accountability;
7. existing controller workflow handling.

The frontend injects the active in-memory CSRF token only for the four exact
POST aliases. Caller-provided CSRF values cannot override it.

---

## Fixed Roles

`role.admin@<backend-id>` grants `searchtimers.execute` only for the same
concrete backend.

`role.read-only@<backend-id>` denies the execution family before direct grants
or Admin expansion.

Wildcard Admin rows remain ineffective for a concrete backend.

---

## Mutation-Free Runtime Profile

The runtime profile sends exactly:

```json
{}
```

The existing workflow parser produces an unknown, non-executable plan. Both
`execute` and `real-test` stop at the existing validation boundary before
operator confirmation, executor opt-in, command mapping or executor invocation.

Expected response subset:

```text
HTTP 200
success = false
executed = false
blocked = true
dispatchStage = validation-blocked
operation = unknown
message = workflow plan is not executable
errors = ["workflow plan is not executable"]
```

Profile:

```text
tools/phase62-runtime-acceptance/slice-2n-searchtimer-execution.json
```

---

## Focused Tests

```text
core/security/tests/test_searchtimer_execution_security.cpp
web/frontend/tests/test_searchtimer_execution_security_runtime.js
tools/check_searchtimer_execution_security.py
```

They cover:

- all four exact aliases;
- Legacy Basic compatibility;
- browser CSRF enforcement;
- direct permission and exact backend scope;
- default backend resolution;
- query-string normalization;
- trailing-slash fail-closed behavior;
- exact Admin allowance and wildcard Admin denial;
- Read-only precedence;
- canonical, secret-free accountability;
- frontend token overwrite protection;
- continued exclusion from the safe-POST allowlist.

The previously monolithic generic gate test was reduced to compact smoke
coverage. Route-specific behavior remains in the focused owner tests.

---

## Real yaVDR Acceptance

The guarded installation and acceptance used:

```text
Repository head:
0c4893a1afa5059af016b6c7336f6dc9a9801c82

GitHub Actions:
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30704777834

Verified backup:
/var/backups/vdr-suite-phase62-slice2n-20260801T150252Z-0c4893a1afa5

Installed/running daemon SHA-256:
3ecb47b33013ea0bdb1eb066ef0b1be7d89c80ebe1f5e9d0f47f7ed4fd798aa7

Installed deferred loader SHA-256:
47c78c871c7caefed8eee2e11e14b8fda5edd3bf85a07f3f099ff24b0a88ce51
```

The runtime profile passed:

```text
tests_passed=51
tests_failed=0
runtime_http_requests=49
accountability_authorized=16
accountability_csrf=4
accountability_permission=4
accountability_read_only=4
accountability_scope=8
```

The checksum-protected evidence is stored at:

```text
/var/backups/vdr-suite-phase62-slice2n-20260801T150252Z-0c4893a1afa5/runtime-acceptance-slice2n
```

Acceptance proved:

```text
resource_state_unchanged=yes
target_grants_restored=yes
unrelated_grants_untouched=yes
browser_session_revoked=yes
revoked_cookie_replay_denied=yes
accountability_secret_free=yes
database_integrity=yes
service_pid_unchanged=yes
execution_dispatch_stage=validation-blocked
real_searchtimer_mutations=0
real_searchtimer_executor_dispatches=0
```

The service restarted once for the guarded installation, then retained PID
`64164` throughout acceptance. Database quick-check, foreign-key validation,
installed fingerprints, backup checksums and evidence checksums all passed.

---

## Explicitly Deferred

This slice does not migrate cache refresh or administrative routes:

```text
POST /api/searchtimers/preview/cache/refresh
POST /api/vdr/searchtimers/preview/cache/refresh
POST /api/epg/cache/refresh
POST /api/epgsearch/native-fuzzy/refresh
POST /api/vdr/epgsearch/native-fuzzy/refresh
POST /api/epgsearch/native-fuzzy/stale-probes/delete
POST /api/vdr/epgsearch/native-fuzzy/stale-probes/delete
```

No production SearchTimer execution is enabled by Slice 2N. Existing operator
confirmation, executor opt-in, production policy, backend-write allowlist,
readback verification and kill-switch rules remain unchanged.

No generic administration, credential lifecycle, Phase 63+ runtime, PR-ready
transition or merge is included.

---

## Completion

Repository implementation, focused tests, full CI, guarded installation,
mutation-free real-runtime acceptance, cleanup and durable evidence all passed.

PR #117 remains open, Draft and unmerged.
