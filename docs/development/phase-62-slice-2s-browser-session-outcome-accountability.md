# Phase 62 Slice 2S — Browser-Session Lifecycle Outcome Accountability

## Status

Repository implementation, full source/runtime CI and guarded real yaVDR
runtime acceptance are complete.

Accepted implementation/runtime head:

```text
c128867bfbf4ce10bcf7dc23d14652e5f5324c83
```

Accepted source/runtime CI:

```text
VDR-Suite CI #6663
Run ID 30717721595
All five jobs successful
```

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

The smallest independent gap was narrower: the existing browser lifecycle gate
already recorded pre-dispatch accountability, but the actual issue/revoke result
was not recorded after the HTTP service ran.

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
issue permission:  session.issue.self
issue action:      browser.session.issue
revoke permission: session.revoke.self
revoke action:     browser.session.revoke
backend scope:     *
decision:          allowed
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

All five jobs in CI #6663 passed, including the complete architecture/C++
regression graph, packaging, frontend contracts and full daemon build.

---

## Accepted Real-Runtime Acceptance

The guarded yaVDR pass installed the CI-accepted daemon and performed one
complete browser lifecycle without contacting or mutating VDR domain state.

It verified:

1. exact repository head, clean worktree and matching build/installed/running
   daemon fingerprints;
2. one successful browser-session issue;
3. exactly one gate-owned `authorization.allowed` event and one
   `operation.succeeded` issue event with matching actor, device, request and
   correlation context;
4. ordinary browser-authenticated GET access;
5. missing-CSRF logout denial with exactly one gate denial event and zero
   operation events;
6. one valid logout with exactly one gate allow event and one
   `operation.succeeded` revoke event;
7. atomic lifecycle revocation and revoked-cookie replay denial;
8. secret-free acceptance accountability;
9. SQLite quick/foreign-key integrity and an active unchanged daemon PID.

Accepted runtime summary:

```text
service_pid_after_install=69610
service_pid_after_acceptance=69610
runtime_http_requests=5
login_accountability_events=2
missing_csrf_accountability_events=1
logout_accountability_events=2
lifecycle_accountability_events=5
operation_succeeded_events=2
missing_csrf_operation_events=0
login_dispatch_authorized=yes
login_outcome_succeeded=yes
ordinary_browser_get=yes
missing_csrf_denied=yes
logout_dispatch_authorized=yes
logout_outcome_succeeded=yes
logout_succeeded=yes
session_revoked=yes
credential_revoked=yes
revoked_cookie_replay_denied=yes
accountability_secret_free=yes
database_integrity=yes
service_state=active
automatic_rollback=not-required
```

Installed fingerprints:

```text
daemon_sha256=682cfc76738454f57daff0831fe7a01786f57abf42cf16c2fa9c2ac16309a07a
loader_sha256=3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
runtime_report_sha256=9ca22c30db9e22decb8e4f74d0204b82d53bb58c344cebdd95d4bae0893a5421
database_before_sha256=12356c390c4c852bf59b1a9636e27738332ab71f836dcb01ef46984a39dc7e0f
database_after_sha256=2153b347d97ce1148a1efdbc3628c4f9652346e82b27d0baeae50c38172e5378
```

The before/after database snapshots intentionally differ because the test
browser-session lifecycle rows and acceptance accountability events remain as
revoked, durable evidence. The session and credential are inactive, have
revocation timestamps and cannot be replayed.

Durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2s-20260801T210333Z-c128867bfbf4/runtime-acceptance-slice2s
```

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

All Slice-2S gates are complete:

1. the atomic repository diff remained within the lifecycle-outcome boundary;
2. focused, architecture and complete regression tests passed;
3. all five source/runtime CI jobs passed;
4. guarded real yaVDR issue/logout outcome acceptance succeeded;
5. the runtime test session and credential are revoked and durable evidence is
   preserved.

No next Phase-62 implementation slice is selected by this closeout.
