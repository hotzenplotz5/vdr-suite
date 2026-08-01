# Phase 62 Slice 2U — Concurrent Browser-Session Limit

## Status

Active bounded Phase-62 implementation contract.

Parent closeout head:

```text
3cd36e6d99102638bc753ec3c01bce2e5cacd835
```

PR #117 remains open, Draft and unmerged.

## Purpose

Slice 2U adds a configurable upper bound for simultaneously effective browser
sessions per actor. It prevents unbounded session accumulation without silently
revoking or displacing an existing session.

The slice is intentionally narrower than idle expiry, refresh, cleanup,
retention or session administration.

## Configuration

The daemon reads:

```text
VDR_SUITE_BROWSER_SESSION_MAX_ACTIVE_PER_ACTOR
```

Contract:

```text
0     unlimited compatibility default
1..64 maximum effective active browser sessions per actor
```

Parsing is strict:

- decimal digits only;
- no sign;
- no leading or trailing whitespace;
- no suffix;
- no overflow;
- values above 64 are invalid.

An invalid configured value fails browser-session issuance closed:

```text
HTTP 503
browser_session_limit_configuration_invalid
```

The default remains `0`, so installation alone does not change existing
session behaviour.

## Effective-session count

The limit counts only browser sessions that are effective at request time.
A row counts only when all of the following remain true:

- the browser verifier row belongs to the actor, is active, unrevoked and
  unexpired;
- the canonical actor exists, is active and unrevoked;
- the device exists, belongs to the same actor, is active and unrevoked;
- the canonical session exists, belongs to the same actor and device, is
  active, unrevoked and unexpired;
- the browser credential exists, belongs to the same actor, is active,
  unrevoked and unexpired;
- the issuing credential exists, belongs to the same actor, is active,
  unrevoked and unexpired.

A raw browser row whose issuer has become invalid does not consume a slot. This
preserves the effective-lifecycle semantics accepted in Slice 2T.

A repository read or SQL failure is not interpreted as zero. Issuance fails
closed through the existing generic issuance failure path.

## Atomic issuance policy

The effective count and all browser-session inserts execute within the existing
serialized `BEGIN IMMEDIATE` issuance transaction.

For a configured limit `N`:

```text
count < N  -> issue one new browser session
count >= N -> create nothing and return LimitReached
```

The limit check occurs after actor, device and issuing-credential lifecycle
validation and before any session, credential or verifier row is inserted.

The service exposes a bounded issuance result distinction:

```text
Issued
LimitReached
Failed
```

The existing optional-return compatibility method remains available and maps
all non-issued results to an empty optional.

## HTTP and accountability contract

When the limit is reached:

```text
HTTP 409
browser_session_limit_reached
```

The response is `no-store`, contains the request ID, emits no session cookie and
contains no secret.

The operation outcome is appended as:

```text
event_type: operation.failed
permission: session.issue.self
action: browser.session.issue
reason_code: browser_session_limit_reached
outcome: failed
```

If outcome persistence fails, the existing fail-closed accountability response
remains authoritative.

An invalid limit configuration records:

```text
reason_code: browser_session_limit_configuration_invalid
```

Existing active sessions are never mutated by either denial.

## Required source verification

Focused tests must prove:

1. the default unlimited value allows multiple sessions;
2. strict values `0`, `1` and `64` are accepted;
3. malformed, signed, spaced, overflowing and above-maximum values are invalid;
4. the first session at limit `1` succeeds;
5. the second session returns `LimitReached` without any new lifecycle row;
6. the first session remains usable and unchanged;
7. revoking the first session frees the slot;
8. expired browser rows do not consume a slot;
9. a revoked issuer with a raw active browser row does not consume a slot;
10. another actor is counted independently;
11. repository count failure fails closed;
12. HTTP maps only `LimitReached` to 409;
13. invalid configuration maps to 503;
14. accountability is complete and secret-free;
15. the check and inserts remain in the serialized issuance transaction.

The architecture guard must reject:

- counting only raw `active` browser rows;
- counting without canonical actor/device/session/browser-credential and issuer
  lifecycle joins;
- counting outside the issuance transaction;
- automatic eviction or revocation;
- a non-zero compatibility default;
- values above the bounded maximum.

## Planned real-yaVDR acceptance

After full source CI, guarded runtime acceptance will:

1. back up runtime files, daemon configuration and SQLite state;
2. install the exact accepted daemon;
3. temporarily configure the per-actor maximum to `1`;
4. issue one isolated test browser session successfully;
5. deny a second session for the same actor with HTTP 409;
6. prove no second browser/session/credential lifecycle was inserted;
7. prove the first session can perform a harmless ordinary GET;
8. revoke the first test lifecycle;
9. issue a replacement session successfully;
10. revoke all test lifecycle rows;
11. restore the original daemon configuration;
12. verify database integrity, service health, secret-free evidence and zero VDR
    domain mutations.

No production session or credential may be revoked to make room for the test.

## Explicitly deferred

Slice 2U does not add:

- idle timeout or `last_seen`;
- sliding expiry or refresh;
- expired-session cleanup;
- retention policy;
- automatic eviction;
- oldest-session selection;
- per-device limits;
- session listing or administration APIs;
- logout-all-devices;
- identity, credential, grant or role administration;
- native or service credentials;
- audit query/export;
- Android or Android TV work;
- Phase 63-67 runtime.

## Acceptance gate

Slice 2U is not accepted runtime until:

1. the implementation remains within this boundary;
2. focused configuration, repository, issuance and HTTP tests pass;
3. architecture and Make-test inventory checks pass;
4. all five GitHub Actions jobs pass on the final stabilization head;
5. guarded real-yaVDR acceptance succeeds;
6. test lifecycle rows are revoked and configuration is restored;
7. documentation-only closeout CI passes.
