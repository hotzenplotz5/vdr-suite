# Phase 62 Slice 2L — SearchTimer Maintenance Security Migration

## Status

Slice 2L is complete on Draft PR #117.

Repository implementation, CI, guarded yaVDR installation and mutation-safe
runtime acceptance passed on 2026-08-01 at:

```text
43b516c7e2adb96bfde415abc8c665f77a541643
```

---

## Scope

Slice 2L migrates the existing SearchTimer update and delete aliases as one
bounded maintenance family:

```text
POST /api/searchtimers/update
POST /api/vdr/searchtimers/update
  -> searchtimers.modify@<backend-id>

POST /api/searchtimers/delete
POST /api/vdr/searchtimers/delete
  -> searchtimers.delete@<backend-id>
```

Missing or empty `backendId` resolves to `default`, matching the existing
request parsers.

Query strings remain supported. Trailing-slash variants are not aliases and
remain fail-closed.

---

## Browser Security Contract

For browser-session requests the server performs, in order:

1. browser credential and lifecycle validation;
2. active actor-grant resolution;
3. exact route classification;
4. cookie-bound CSRF validation;
5. exact permission and backend-scope authorization;
6. append-only pre-dispatch accountability;
7. existing controller and SearchTimer service execution.

The frontend injects the active in-memory CSRF token only for the four exact
POST aliases. A caller-provided token cannot override the active token.

GET requests, trailing-slash variants, SearchTimer validation, planning,
preview and execution do not receive maintenance CSRF treatment from this
slice.

---

## Fixed Roles

`role.admin@<backend-id>` expands to:

```text
searchtimers.modify
searchtimers.delete
```

only for the same concrete backend scope.

`role.read-only@<backend-id>` denies both permissions before direct grants or
Admin expansion.

Wildcard role rows remain ineffective for a concrete backend.

---

## Mutation-Safe Runtime Contract

Both runtime profiles use exactly:

```json
{}
```

The existing parsers resolve the backend to `default`. The existing services
then stop before the command executor because `backendNativeId` is absent.

Expected response subset for both update and delete:

```text
HTTP 200
success = false
message = searchtimer backend native id is required
errors = ["backendNativeId is required"]
```

No existing SearchTimer identifier is supplied. No update or delete executor
may be called during routine acceptance.

Profiles:

```text
tools/phase62-runtime-acceptance/slice-2l-searchtimer-update.json
tools/phase62-runtime-acceptance/slice-2l-searchtimer-delete.json
```

---

## Focused Tests

The slice uses small owner-specific tests instead of extending existing
monolithic tests:

```text
core/security/tests/test_searchtimer_maintenance_security.cpp
web/frontend/tests/test_searchtimer_maintenance_security_runtime.js
tools/check_searchtimer_maintenance_security.py
```

They cover:

- all four exact aliases;
- Legacy Basic compatibility;
- missing CSRF denial;
- missing permission denial;
- exact direct grants;
- default backend resolution;
- wrong backend scope;
- query-string normalization;
- trailing-slash fail-closed behavior;
- exact Admin allowance;
- wildcard Admin denial;
- Read-only precedence;
- canonical accountability fields;
- frontend CSRF overwrite protection;
- continued exclusion of SearchTimer execute and safe workflow POSTs.

---

## Real yaVDR Acceptance

The combined Slice 2L/2M batch used one guarded installation and one verified
backup:

```text
Repository head:
43b516c7e2adb96bfde415abc8c665f77a541643

GitHub Actions:
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30703009976

Verified backup:
/var/backups/vdr-suite-phase62-slice2lm-20260801T141451Z-43b516c7e2ad

Installed/running daemon SHA-256:
02a701c3bc8279808b3303c23d19080dc7ccc9c522d2ca28c90b996618456a03

Installed deferred loader SHA-256:
c32999adc0aca8ee815ceebde8982ad50f4c206d93d4864ac949146dc8190bd7
```

Each maintenance profile passed independently:

```text
slice-2l-searchtimer-update:
  tests_passed=29
  runtime_http_requests=27
  accountability_authorized=8
  accountability_csrf=2
  accountability_permission=2
  accountability_read_only=2
  accountability_scope=4

slice-2l-searchtimer-delete:
  tests_passed=29
  runtime_http_requests=27
  accountability_authorized=8
  accountability_csrf=2
  accountability_permission=2
  accountability_read_only=2
  accountability_scope=4
```

The runtime evidence is checksum-protected at:

```text
/var/backups/vdr-suite-phase62-slice2lm-20260801T141451Z-43b516c7e2ad/runtime-acceptance-batch
```

Both profiles preserved SearchTimer resource state, restored all targeted
grants, left unrelated grants untouched, revoked their browser sessions,
denied revoked-cookie replay and kept the daemon PID unchanged.

```text
real_searchtimer_updates=0
real_searchtimer_deletes=0
database_quick_check=ok
database_foreign_key_violations=0
```

---

## Explicitly Deferred

This slice does not migrate:

```text
POST /api/searchtimers/execute
POST /api/vdr/searchtimers/execute
POST /api/searchtimers/real-test
POST /api/vdr/searchtimers/real-test
```

Execution has separate operator-confirmation, executor-opt-in and backend-write
policy and therefore remains a separate security class.

Validation and planning were classified separately as non-mutating POSTs in
Slice 2M.

No generic role administration, native/service credential lifecycle,
idempotency platform, Phase 63+ runtime, PR-ready transition or merge is part of
Slice 2L.

---

## Completion

Repository implementation, focused tests, full CI, guarded installation,
mutation-safe runtime acceptance, cleanup and durable evidence all passed.

PR #117 remains open, Draft and unmerged.
