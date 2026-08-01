# Phase 62 Slice 2S — Browser-Session Lifecycle Outcome Accountability

## Status

Repository implementation is prepared. Full CI and guarded real yaVDR runtime
acceptance remain pending.

PR #117 remains open, Draft and unmerged.

---

## Why this slice follows Slice 2R

Slice 2R completed the bounded absolute browser-session lifetime contract. A
fresh post-2R gap review compared the remaining lifecycle, administration and
accountability work.

A browser-session idle timeout is not a small follow-up because it requires a
`last_seen` persistence model, request-time writes and concurrency policy.
Expired-session cleanup is a destructive retention policy. Concurrent-session
limits require race-safe issuance semantics and richer failure reporting.

The smallest independent gap is narrower: the existing browser lifecycle gate
already records pre-dispatch accountability, but the actual issue/revoke result
is not recorded after the HTTP service runs.

---

## Exact Scope

Slice 2S adds outcome events only for the two existing lifecycle operations:

```text
POST /api/security/browser-sessions
POST /api/security/browser-sessions/logout
```

It does not add, remove or reclassify any route.

The existing gate events remain unchanged:

```text
authorization.allowed / authorization.denied
dispatch_authorized / dispatch_denied
```

The HTTP lifecycle service adds one post-operation event for every dispatch that
reaches an actual issue or revoke attempt:

```text
operation.succeeded
operation.failed
```

Canonical fields:

```text
issue permission: session.issue.self
issue action:     browser.session.issue
revoke permission: session.revoke.self
revoke action:     browser.session.revoke
backend scope:      *
decision:           allowed
```

Outcome reason codes:

```text
browser_session_issued
browser_session_issuance_failed
browser_session_lifetime_configuration_invalid
browser_session_revoked
browser_session_revocation_failed
```

The final `outcome` value is exactly `succeeded` or `failed`.

Authentication and CSRF denials remain gate-owned. They never reach the HTTP
service and therefore receive no duplicate operation outcome.

---

## Fail-Closed and Compensation Contract

### Login

A successful session is not delivered until its `operation.succeeded` event is
persisted.

If issuance succeeds but the outcome append fails:

1. the new browser session, canonical session and browser credential are subject
   to compensating revocation through the existing transactional lifecycle
   service;
2. the one-time cookie and CSRF secrets are wiped;
3. the response is HTTP 503 `accountability_unavailable`;
4. the response contains no `Set-Cookie` session credential.

The generated credential therefore cannot become an unaccounted client-visible
session.

If lifetime validation or issuance itself fails, `operation.failed` is appended
before the existing failure response is returned. Failure to persist that event
also returns `accountability_unavailable`.

### Logout

Successful revocation remains authoritative before the success outcome is
appended.

If revocation succeeds but the outcome append fails:

1. the browser credential, canonical session and canonical credential remain
   revoked;
2. the response is HTTP 503 `accountability_unavailable`;
3. the response still includes an expired `Set-Cookie` value with `Max-Age=0`
   so the client removes its unusable credential.

If revocation itself fails, the service appends `operation.failed`. It does not
clear a potentially still-active cookie unless revocation actually succeeded.

---

## Secret and Context Boundary

Outcome records contain only the established non-secret context:

- actor, device and session identifiers;
- authentication state;
- permission and global scope;
- request and correlation identifiers;
- canonical action, decision, reason and outcome.

They never contain:

- Authorization headers or decoded credentials;
- cookie values or cookie fragments;
- CSRF tokens;
- verifier hashes;
- response bodies;
- plaintext secrets.

For successful login, the event session identifier is the newly created browser
session rather than the issuing Basic compatibility session. Failure events
before creation retain the authenticated source context.

---

## Verification Contract

The focused HTTP-service test covers:

- successful issue outcome with the newly created session ID;
- successful revoke outcome;
- invalid-lifetime failure outcome;
- issuance failure outcome;
- revocation failure outcome;
- canonical permissions, actions, global scope and outcome values;
- no cookie or CSRF secret in any event;
- forced accountability append failure after successful issuance;
- compensating revocation of all three new lifecycle rows;
- HTTP 503 without a session cookie on blocked login outcome;
- forced accountability append failure after successful revocation;
- revoked lifecycle rows remaining revoked;
- expired cookie delivery and replay denial after blocked logout outcome.

A dedicated static checker enforces the service, server wiring, tests,
documentation and Make integration.

---

## Planned Real-Runtime Acceptance

Real-runtime acceptance may run only after all five PR CI jobs are green.

The guarded pass will:

1. verify exact branch/head, clean worktree and installed fingerprints;
2. back up the installed runtime and SQLite database;
3. install the CI-accepted daemon;
4. issue one bounded browser session;
5. prove the existing pre-dispatch event and new `operation.succeeded` issue
   event use the same request context;
6. deny a missing-CSRF logout before service dispatch and prove no duplicate
   operation event was created;
7. perform valid logout and prove the new successful revoke outcome;
8. deny revoked-cookie replay;
9. prove all acceptance events are secret-free;
10. prove the test session and credential remain revoked, database integrity is
    valid and the service remains active.

The routine runtime pass will not force an accountability storage failure on the
production database. Append-failure compensation is covered by the focused
in-memory SQLite test.

---

## Explicitly Deferred

Slice 2S does not implement:

- outcome events for business mutations outside browser-session lifecycle;
- a transactional outbox or universal commit/event atomicity;
- event retries, queues or delivery infrastructure;
- idle timeout or `last_seen` persistence;
- sliding expiry or refresh;
- expired-session cleanup or retention;
- concurrent-session limits;
- protected audit query/export/retention;
- identity, credential, grant or role administration;
- generic roles;
- Android clients;
- Phase 63-67 runtime;
- PR Ready transition or merge.

## Acceptance Gate

Slice 2S is not complete until:

1. the atomic repository diff remains within this lifecycle-outcome boundary;
2. the focused and architecture tests pass;
3. all five PR CI jobs pass;
4. guarded real yaVDR issue/logout outcome acceptance succeeds;
5. the runtime test session is revoked and durable evidence is documented.
