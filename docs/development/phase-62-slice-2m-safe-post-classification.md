# Phase 62 Slice 2M — Explicit Safe POST Classification

## Status

Repository implementation is active on Draft PR #117.

No real yaVDR installation or runtime acceptance has been performed for this
slice yet.

---

## Purpose

HTTP method alone does not determine whether a request mutates state.

Slice 2M explicitly classifies the existing validation, preview and planning
POST routes that execute no backend write. This allows browser sessions to use
them without weakening the fail-closed boundary for every other POST.

---

## Exact Safe Routes

Recording validation and preview:

```text
POST /api/recordings/actions/validate
POST /api/vdr/recordings/actions/validate
POST /api/recordings/actions/preview
POST /api/vdr/recordings/actions/preview
```

SearchTimer validation and planning:

```text
POST /api/searchtimers/validate
POST /api/vdr/searchtimers/validate
POST /api/searchtimers/plan
POST /api/vdr/searchtimers/plan
```

Query strings are accepted because classification uses the exact normalized
request path.

Trailing-slash variants are not aliases and remain fail-closed.

---

## Security Contract

Each exact safe route requires an authenticated request context.

Accepted authentication mechanisms remain:

- Legacy Basic compatibility identity;
- configured Managed Basic identity;
- valid browser session with normal lifecycle and precedence rules.

A browser session does not need a CSRF token for these routes because they do
not mutate state.

A safe POST:

- does not set `protectedMutation`;
- does not require a mutation permission;
- does not consume Admin or Read-only role expansion;
- does not append an authorization decision event merely for successful safe
  dispatch;
- does not bypass authentication or browser-grant persistence availability.

This is an exact allowlist, not a general rule that POST is safe.

---

## Routes That Remain Fail-Closed

The classification deliberately excludes:

```text
POST /api/searchtimers/execute
POST /api/vdr/searchtimers/execute
POST /api/searchtimers/real-test
POST /api/vdr/searchtimers/real-test
POST /api/searchtimers/preview/cache/refresh
POST /api/vdr/searchtimers/preview/cache/refresh
POST /api/epg/cache/refresh
POST /api/epgsearch/native-fuzzy/refresh
POST /api/vdr/epgsearch/native-fuzzy/refresh
POST /api/epgsearch/native-fuzzy/stale-probes/delete
POST /api/vdr/epgsearch/native-fuzzy/stale-probes/delete
```

Execution, cache refresh and administration have state, operator or backend
write implications and require separate permissions and CSRF contracts.

---

## Focused Tests

The shared browser-gate fixture avoids repeating identity, SQLite and browser
credential setup:

```text
core/security/tests/SecurityHttpGateBrowserTestFixture.h
```

The focused runtime-independent gate test is:

```text
core/security/tests/test_safe_post_security.cpp
```

The architecture guard is:

```text
tools/check_safe_post_security.py
```

Together they prove:

- all eight exact aliases;
- Legacy Basic allowance;
- browser-session allowance without CSRF or grants;
- non-mutation classification;
- no successful-dispatch authorization evidence;
- query-string normalization;
- trailing-slash denial;
- continued fail-closed treatment of execute, real-test, refresh and
  administration routes;
- frontend mutation wrappers still do not inject CSRF into safe POSTs.

---

## Efficient Acceptance Boundary

Slice 2M is installed together with Slice 2L after the combined GitHub batch is
fully CI-green.

The real yaVDR pass will use one guarded installation and one browser session.
It will exercise all eight exact safe routes without CSRF, verify the excluded
routes remain fail-closed and confirm resource snapshots, service state and
SQLite integrity remain unchanged.

No real Recording or SearchTimer mutation is part of this acceptance.

PR #117 remains open, Draft and unmerged.
