# Phase 62 Slice 2V — Browser-Session Idle Expiry and Throttled last_seen

## Status

Selected after the completed Slice-2U closeout and fresh post-2U gap analysis.
Implementation and acceptance are pending.

PR #117 remains open, Draft and unmerged.

## Purpose

Slice 2V adds an optional server-side idle timeout for browser sessions and one
explicit throttled browser-activity timestamp. It closes only the request-time
idle-expiry gap.

The existing absolute `expires_at` lifetime remains the hard upper bound and is
never extended by activity.

## Configuration

The daemon reads:

```text
VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS
```

Contract:

```text
0          disabled compatibility default
300..86400 enabled idle timeout in seconds
```

Parsing is strict:

- decimal digits only;
- no sign;
- no leading or trailing whitespace;
- no suffix;
- no overflow;
- values `1..299` and values above `86400` are invalid.

An invalid configured value fails browser-session issuance and presented
browser-cookie authentication closed:

```text
HTTP 503
browser_session_idle_configuration_invalid
```

Anonymous requests and Basic authentication without a presented browser cookie
remain governed by their existing contracts.

## Persistence contract

The browser-session verifier table gains exactly one additive column:

```text
last_seen_at TEXT NOT NULL
```

Migration rules:

- schema creation is idempotent;
- existing rows are backfilled from their immutable `created_at` value;
- newly issued rows initialize `last_seen_at` at insertion time;
- `updated_at` is not reused as an activity clock because revocation, expiry and
  other lifecycle writes also modify it;
- no row is deleted, revoked or otherwise displaced by the migration.

## Request-time effectiveness

With timeout `T > 0`, a browser session is idle-expired when:

```text
last_seen_at <= CURRENT_TIMESTAMP - T seconds
```

A session is effective only when all previously accepted actor, device,
canonical session, browser credential, issuing credential and absolute-expiry
checks pass and the browser row is not idle-expired.

Idle expiry:

- maps to the existing HTTP 401 `session_expired` contract;
- applies to ordinary GET requests;
- applies before protected mutation dispatch;
- applies to logout and CSRF verification;
- never refreshes or extends absolute `expires_at`;
- does not physically revoke, delete or clean up the row.

Cookie authentication and CSRF verification use the same repository-owned idle
calculation. Neither path may implement a separate client-side or gate-local
idle clock.

## Throttled activity writes

A successfully verified browser credential updates `last_seen_at` at most once
per fixed 60-second interval.

Rules:

- the write occurs only when idle timeout is enabled;
- invalid, unknown, revoked, absolute-expired or idle-expired credentials do not
  update activity;
- successful credential authentication counts as activity even when a later
  CSRF, permission, backend or domain decision denies the request;
- repeated requests inside the 60-second write interval perform no update;
- a repository error while attempting an activity update fails the request
  closed with HTTP 503 `browser_session_activity_unavailable`;
- no session secret, CSRF token, cookie value or Authorization header is stored
  or emitted by the activity path.

## Concurrent-session interaction

When the per-actor concurrency limit from Slice 2U is enabled, idle-expired
browser sessions do not consume a slot. The effective count remains inside the
serialized issuance transaction and receives the same configured idle timeout.

No automatic eviction or revocation is introduced.

## Accountability

Existing authentication-denial accountability records idle expiry through the
secret-free `session_expired` reason. Invalid configuration and activity-store
unavailability use their explicit reason codes.

No new audit read API, export, retention policy, cleanup job or outcome family
is included.

## Required focused verification

Source tests must prove:

1. default `0` is valid and disables idle expiry and request-time activity writes;
2. `300` and `86400` are valid;
3. empty, signed, spaced, suffixed, overflowing, `1..299` and above-maximum
   values are invalid;
4. schema upgrade adds and backfills `last_seen_at` idempotently;
5. new browser rows initialize `last_seen_at`;
6. a session inside the idle window authenticates;
7. an idle-expired session returns `session_expired` for ordinary GET and
   protected mutation paths;
8. CSRF verification rejects the same idle-expired row;
9. absolute expiry remains authoritative and is never extended;
10. a successful authenticated request updates `last_seen_at` only when at
    least 60 seconds have elapsed;
11. repeated requests inside the throttle interval do not write;
12. invalid, revoked and expired credentials do not write;
13. an activity update repository failure maps to HTTP 503 and prevents dispatch;
14. idle-expired rows do not consume a Slice-2U concurrency slot;
15. logout and revoked-cookie replay semantics remain unchanged;
16. accountability and all test evidence remain secret-free.

The architecture guard must reject:

- use of `updated_at` as the idle clock;
- per-request unthrottled writes;
- client-owned idle decisions;
- separate cookie and CSRF idle calculations;
- sliding absolute expiry;
- cleanup, deletion, eviction or automatic revocation;
- a non-zero compatibility default;
- idle timeout values outside the bounded contract.

## Real yaVDR acceptance gate

A new guarded real-yaVDR acceptance is required because this slice changes the
daemon and browser-session schema. It is permitted only after all five source CI
jobs pass on the final stabilization head.

The runtime pass must use an isolated test identity and prove at minimum:

- configured idle timeout is applied;
- ordinary GET succeeds before idle expiry;
- ordinary GET and mutation fail closed after idle expiry;
- no domain mutation occurs;
- absolute expiry is unchanged;
- throttled `last_seen_at` changes at most once in the accepted interval;
- logout/replay behaviour remains valid for a non-idle replacement session;
- original configuration is restored;
- test lifecycle is revoked;
- SQLite quick and foreign-key checks pass;
- service remains active;
- evidence is secret-free.

## Explicitly excluded

Slice 2V does not add:

- physical cleanup or retention;
- automatic session eviction or revocation;
- session listing, logout-all or administration UI/API;
- sliding absolute expiry or cookie refresh;
- general actor, identity, credential, grant or role administration;
- native/service credential lifecycle;
- generic operation outcomes, Outbox, revisions or idempotency;
- protected audit reads or export;
- Android or Android TV work;
- Phase 63-67 runtime.
