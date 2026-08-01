# Phase 62 Slice 2N — SearchTimer Execution Security Migration

## Status

Repository implementation is active on Draft PR #117.

No yaVDR installation or runtime acceptance has been performed for this slice.

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

Routine acceptance must prove SearchTimer resource state unchanged and zero
executor invocation.

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

PR #117 remains open, Draft and unmerged.
