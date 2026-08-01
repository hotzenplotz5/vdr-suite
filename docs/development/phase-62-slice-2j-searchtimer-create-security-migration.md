# Phase 62 Slice 2J — SearchTimer Create Security Migration

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Phase 62 Slice 2I](phase-62-slice-2i-recording-execution-security-migration.md)

---

## Status

Implementation, local regression tests, commit, push, CI and real-runtime
acceptance are complete.

The accepted repository and runtime commit is:

```text
7a3c8a1a3e0e6902b6ec0fea8a48bd69428c93e4
```

Runtime acceptance created no real SearchTimer.

---

## Purpose

Slice 2J migrates ordinary SearchTimer creation into the Phase 62 identity,
authorization, accountability, and browser-session CSRF contract.

Only SearchTimer creation belongs to this slice.

---

## Protected Routes

The exact protected POST aliases are:

```text
POST /api/searchtimers
POST /api/vdr/searchtimers
```

Query strings are accepted because authorization uses the normalized path.

Trailing-slash variants are not aliases and remain unmigrated:

```text
POST /api/searchtimers/
POST /api/vdr/searchtimers/
```

They fail closed for browser-session and enforced security contexts.

---

## Authorization Contract

Both routes require:

```text
searchtimers.create@<backendId>
```

The authorization permission and action are:

```text
searchtimers.create
```

`backendId` is read from the JSON body. When it is absent or empty, the gate
uses `default`, matching the existing SearchTimer create request parser.

A permission grant for another backend does not authorize the request.

---

## Role Contract

The fixed administrator role grants `searchtimers.create`.

The fixed read-only role retains precedence over administrator and direct
mutation permissions. SearchTimer creation by such an actor is rejected with:

```text
role_read_only
```

Managed credentials without the permission receive `permission_denied`.

Legacy Basic compatibility remains available while the deployment runs in
legacy compatibility mode.

---

## Browser-Session CSRF Contract

Browser-session creation requests require:

1. an authenticated browser-session cookie;
2. a valid `X-CSRF-Token`.

Missing or invalid CSRF evidence is rejected before dispatch with:

```text
csrf_validation_failed
```

The deferred frontend loader adds the current in-memory CSRF token only to the
two exact POST aliases.

It does not add the header to:

- GET requests;
- trailing-slash variants;
- SearchTimer update;
- other SearchTimer workflows.

A caller-provided CSRF value cannot override the current session token.

---

## Accountability Contract

Authorization records use:

```text
permission = searchtimers.create
action     = searchtimers.create
backendId  = resolved request backend
```

Allowed authorization produces `dispatch_authorized`.

CSRF, permission, role, and backend-scope denials produce `dispatch_denied`
with the corresponding reason code.

Authorization headers, cookies, session secrets, and CSRF secrets must never
be written to accountability fields.

---

## Local Test Coverage

The security HTTP gate test covers:

- both exact aliases;
- Legacy Basic compatibility;
- browser CSRF rejection;
- permission denial and success;
- `default` backend fallback;
- backend-scope denial;
- query strings;
- trailing-slash fail-closed behaviour;
- administrator access;
- read-only precedence;
- enforced mode;
- accountability evidence.

The SearchTimer frontend runtime test covers:

- both exact aliases;
- query strings and absolute URLs;
- replacement of caller-provided CSRF values;
- exclusion of GET, update, and trailing-slash requests.

The targeted C++ test is:

```text
make test-security-http-gate
```

---

## Runtime Acceptance

Real yaVDR runtime acceptance completed successfully on 2026-08-01.

All authorized SearchTimer-create requests used exactly:

```json
{}
```

The parser resolved `backendId` to `default`. The request then stopped at the
existing name validation before the command executor. The accepted response was:

```text
HTTP 200
success = false
message = searchtimer name is required
errors = ["name is required"]
```

The append-only evidence verified:

```text
searchtimer_accountability_events = 17
csrf_denied                      = 2
permission_denied                = 2
backend_scope_denied             = 4
read_only_denied                 = 2
dispatch_authorized              = 7
trailing_slash_fail_closed       = 2
unauthenticated_denials          = 2
real_searchtimer_creates         = 0
```

Both exact aliases, query-string variants, exact administrator scope, read-only
precedence and wildcard-role non-effectiveness passed.

The acceptance browser session was revoked, revoked-cookie replay was denied,
target grants matched the verified backup state, SQLite integrity passed and no
authorization, cookie or CSRF secret appeared in accountability fields.

---

## Out of Scope

Slice 2J does not migrate:

- SearchTimer update;
- SearchTimer delete;
- SearchTimer execute;
- SearchTimer validation or planning;
- SearchTimer real-test;
- preview or preview-cache refresh;
- EPG or native-fuzzy operational state;
- generic security administration;
- Legacy Basic retirement.

---

## Completion Boundary

Slice 2J completed implementation, tests, documentation, commit, push, green
CI, verified runtime installation, mutation-safe runtime acceptance and final
cleanup verification.

PR #117 remains open, Draft and unmerged.
