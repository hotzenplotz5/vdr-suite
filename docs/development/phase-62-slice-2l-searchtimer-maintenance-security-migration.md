# Phase 62 Slice 2L — SearchTimer Maintenance Security Migration

## Status

Repository implementation is active on Draft PR #117.

No real yaVDR installation or runtime acceptance has been performed for this
slice yet.

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

## Explicitly Deferred

This slice does not migrate:

```text
POST /api/searchtimers/execute
POST /api/vdr/searchtimers/execute
POST /api/searchtimers/real-test
POST /api/vdr/searchtimers/real-test
POST /api/searchtimers/validate
POST /api/vdr/searchtimers/validate
POST /api/searchtimers/plan
POST /api/vdr/searchtimers/plan
```

Execution has separate operator-confirmation, executor-opt-in and backend-write
policy and therefore remains a separate security class.

Validation and planning are non-mutating POSTs and require a later explicit
safe-POST classification rather than mutation permission.

No generic role administration, native/service credential lifecycle,
idempotency platform, Phase 63+ runtime, PR-ready transition or merge is part of
Slice 2L.

---

## Efficient Acceptance Plan

The repository batch is completed and CI-validated before the user is asked to
run anything on yaVDR.

After one tested installation, the two profiles run consecutively against the
same verified backup and installed fingerprints. Cleanup, grant restoration,
session revocation, resource snapshots and database integrity are verified
after each profile.

PR #117 remains open, Draft and unmerged.
