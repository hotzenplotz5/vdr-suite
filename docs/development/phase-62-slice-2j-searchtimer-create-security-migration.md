# Phase 62 Slice 2J — SearchTimer Create Security Migration

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Phase 62 Slice 2I](phase-62-slice-2i-recording-execution-security-migration.md)

---

## Status

Implementation and targeted local tests are complete but uncommitted.

Runtime acceptance remains pending.

Runtime acceptance must not create a real SearchTimer.

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

## Runtime Acceptance Plan

Runtime acceptance is deferred until:

1. the complete local regression suite passes;
2. the changes are reviewed, committed, and pushed;
3. CI is green;
4. the new runtime is installed after a verified backup.

No authorized successful SearchTimer create request may be used during routine
acceptance.

Safe evidence includes:

- missing-CSRF rejection;
- valid-CSRF permission denial;
- backend-scope denial;
- another request proven to stop before the command executor.

A successful mutation requires a separately approved disposable and reversible
test contract.

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

Slice 2J is complete only after implementation, tests, documentation, commit,
push, green CI, verified runtime installation, mutation-safe runtime acceptance,
and final cleanup verification.

PR #117 must remain open, Draft, and unmerged.
