# Phase 62 Slice 2V — Browser-Session Idle Expiry and Throttled last_seen

## Status

Implementation, final-head source CI and guarded real-yaVDR runtime acceptance
are fully accepted. The canonical documentation closeout series starts at:

```text
45f1cc78d2c98f6db4d039a5ea7189f51bbcf8e9
```

The final documentation head is covered by the closeout CI reported in the PR
and canonical status files.

Implementation/runtime head:

```text
e84415fadb2587ff744ff8927f1f0113920ece2f
```

Source CI:

```text
VDR-Suite CI #6779
Run ID 30741293079
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30741293079
```

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

An invalid value fails browser-session issuance and presented browser-cookie
requests closed before dispatch. Browser lifecycle routes return:

```text
HTTP 503
browser_session_idle_configuration_invalid
```

Ordinary routes with a valid presented browser credential use the existing
HTTP-503 unavailable-security-persistence path. Anonymous requests and Basic
authentication without a presented browser cookie remain governed by their
existing contracts.

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
calculation. Neither path implements a separate client-side or gate-local idle
clock.

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
  closed before dispatch;
- ordinary routes use the existing HTTP-503 unavailable-persistence response;
- browser logout returns HTTP 503
  `browser_session_activity_unavailable`;
- no session secret, CSRF token, cookie value or Authorization header is stored
  or emitted by the activity path.

## Concurrent-session interaction

When the per-actor concurrency limit from Slice 2U is enabled, idle-expired
browser sessions do not consume a slot. The effective count remains inside the
serialized issuance transaction and receives the same configured idle timeout.

No automatic eviction or revocation is introduced.

## Accountability

Existing authentication-denial accountability records idle expiry through the
secret-free `session_expired` reason. Invalid lifecycle configuration records
`browser_session_idle_configuration_invalid`. Logout activity-store failure
records `browser_session_activity_unavailable`; ordinary routes retain the
existing unavailable-persistence accountability contract.

No new audit read API, export, retention policy, cleanup job or outcome family
is included.

## Focused source verification

The dedicated test and architecture guard cover:

1. default `0` is valid and disables idle expiry and request-time activity writes;
2. `300` and `86400` are valid;
3. empty, signed, spaced, suffixed, overflowing, `1..299` and above-maximum
   values are invalid;
4. schema upgrade adds and backfills `last_seen_at` idempotently;
5. new browser rows initialize `last_seen_at`;
6. a session inside the idle window authenticates;
7. an idle-expired session returns `session_expired` for ordinary GET,
   protected mutation and logout paths;
8. CSRF verification rejects the same idle-expired row;
9. absolute expiry remains authoritative and is never extended;
10. a successful authenticated request updates `last_seen_at` only when at
    least 60 seconds have elapsed;
11. repeated requests inside the throttle interval do not write;
12. disabled idle expiry performs no activity write;
13. invalid configured policy fails presented browser requests closed;
14. a forced activity-update SQL failure returns HTTP 503 before dispatch;
15. idle-expired rows do not consume a Slice-2U concurrency slot;
16. logout and revoked-cookie replay semantics remain unchanged;
17. accountability and all test evidence remain secret-free.

The architecture guard rejects:

- use of `updated_at` as the idle clock;
- per-request unthrottled writes;
- client-owned idle decisions;
- separate cookie and CSRF idle calculations;
- sliding absolute expiry;
- cleanup, deletion, eviction or automatic revocation;
- a non-zero compatibility default;
- idle timeout values outside the bounded contract.

## Real yaVDR acceptance

Guarded runtime acceptance completed successfully on 2026-08-02.

The isolated acceptance:

1. backed up the installed daemon, daemon configuration, frontend loader and
   SQLite state;
2. installed the exact source-CI-approved daemon;
3. applied a temporary idle timeout of `300` seconds through a runtime-only
   systemd drop-in;
4. proved one ordinary GET succeeds before idle expiry;
5. proved one ordinary GET and one protected mutation return HTTP 401
   `session_expired` after idle expiry;
6. proved `last_seen_at` is updated once when due and not rewritten inside the
   60-second throttle interval;
7. proved absolute `expires_at` remains unchanged;
8. proved logout succeeds for a replacement non-idle session and revoked-cookie
   replay returns HTTP 401 `credential_revoked`;
9. verified exact secret-free accountability for idle denials, logout and
   replay;
10. revoked the isolated test lifecycle;
11. restored the original configuration exactly;
12. removed the runtime-only systemd drop-in and idle test environment;
13. verified SQLite quick and foreign-key checks;
14. left the Phase-62 daemon installed and the service active;
15. performed zero VDR domain mutations.

```text
PHASE_62_SLICE_2V_RUNTIME_ACCEPTANCE=PASS

Implementation/runtime head:
e84415fadb2587ff744ff8927f1f0113920ece2f

Source CI:
#6779 / run 30741293079 / all five jobs successful

Installed/running daemon SHA-256:
e0b6f6de08527b6af49d526ca0118b14b6fb85ff3335fc607ca1b531cdee5f60

Loader SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Restored configuration SHA-256:
8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b

Final service PID:
86549

Runtime report SHA-256:
0a961fbc8b51158fd4a16aa24fc9afde7dafa9d5272e986a46ec73880c311f86

Configured idle timeout:
300 seconds

Activity-write interval:
60 seconds

Ordinary GET before idle expiry:
HTTP 200

Ordinary GET after idle expiry:
HTTP 401 session_expired

Protected mutation after idle expiry:
HTTP 401 session_expired

last_seen writes inside accepted interval:
1

Absolute expiry unchanged:
yes

Replacement logout:
HTTP 204

Revoked replacement-cookie replay:
HTTP 401 credential_revoked

Acceptance lifecycle active rows after cleanup:
0

SQLite quick check:
ok

SQLite foreign-key check:
empty

Accountability secret-free:
yes

VDR domain mutations:
0

Final service state:
active

Runtime drop-in:
removed

Idle test environment:
not set
```

Durable secret-free evidence:

```text
/var/backups/vdr-suite-phase62-slice2v-20260802T092139Z-e84415fadb25
```

Runtime report:

```text
/var/backups/vdr-suite-phase62-slice2v-20260802T092139Z-e84415fadb25/runtime-acceptance-report.txt
```

Runtime report SHA-256:

```text
0a961fbc8b51158fd4a16aa24fc9afde7dafa9d5272e986a46ec73880c311f86
```

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

## Acceptance result

The Slice-2V runtime gate is fully satisfied through implementation, focused
tests, architecture checks, final-head source CI and guarded real-yaVDR
acceptance.

Verified:

1. implementation remained within the bounded Slice-2V contract;
2. strict configuration, schema, repository, authenticator and HTTP tests passed;
3. architecture and Make-test inventory checks passed;
4. all five GitHub Actions jobs passed on the final implementation head;
5. guarded real-yaVDR acceptance passed;
6. idle expiry applied consistently to ordinary and mutation paths;
7. throttled `last_seen_at` persistence behaved as specified;
8. absolute expiry was never extended;
9. test lifecycle rows were revoked;
10. original configuration and systemd state were restored;
11. accountability remained secret-free;
12. SQLite integrity checks passed;
13. zero VDR domain mutations occurred;
14. the accepted Phase-62 daemon remains installed and active.

Slice 2V is runtime-accepted. Its documentation closeout is complete when the
final documentation head has all five GitHub Actions jobs green.

## Exact next action

Require all five GitHub Actions jobs for the documentation-only Slice-2V
closeout.

No next Phase-62 implementation slice is selected by this runtime acceptance.
After full closeout CI, perform a fresh post-2V gap analysis and select exactly
one bounded slice. Do not combine that selection with this closeout.
