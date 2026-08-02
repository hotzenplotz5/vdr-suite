# Phase 62 Slice 2U — Concurrent Browser-Session Limit

## Status

Implementation, source CI, guarded real-yaVDR runtime and documentation closeout
are fully accepted.

Implementation/runtime head:

```text
16ff04a4ba371aad32fc4a38bf82f9c0529c532d
```

Source CI:

```text
VDR-Suite CI #6690
Run ID 30723297375
All five jobs successful
```

Documentation closeout commit:

```text
4747d725664d4c382d17d3b19fa2776f48ba437b
```

Final shared closeout and workflow head:

```text
d00fc5045a136d87323fbc13fb1bfc1030f7d3b5
```

Final closeout CI:

```text
VDR-Suite CI #6693
Run ID 30733265772
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30733265772
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

## Real yaVDR acceptance

Guarded runtime acceptance completed successfully on 2026-08-02.

The acceptance:

1. backed up runtime files, daemon configuration and SQLite state;
2. installed the exact source-CI-approved daemon;
3. configured an isolated acceptance actor with a temporary limit of `1`;
4. issued the first browser session with HTTP 200;
5. denied a second same-actor session with HTTP 409
   `browser_session_limit_reached`;
6. proved no second browser/session/credential lifecycle was inserted;
7. proved the first session remained usable through an ordinary GET;
8. logged out the first session and released its slot;
9. issued and logged out a replacement session;
10. denied replay of the revoked replacement cookie with HTTP 401;
11. revoked all isolated acceptance lifecycle rows and the source identity;
12. restored the original daemon configuration;
13. verified database integrity, active service state, secret-free evidence and
    zero VDR domain mutations.

No production session or credential was evicted or revoked.

```text
Implementation/runtime head:
16ff04a4ba371aad32fc4a38bf82f9c0529c532d

Source CI:
#6690 / run 30723297375 / all five jobs successful

Installed/running daemon SHA-256:
0e3ec0d57f4471804824247f712c2457015cc22ac9576df60d8d77ed8ddb3134

Loader SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Final service PID:
79316

Runtime report SHA-256:
7c33b06d07beff6b17bad82153ff4a0f1e7c1f5c8d8f972406f8b0b9160f4c89

Configured limit:
1

First session issue:
HTTP 200

Second same-actor issue:
HTTP 409 browser_session_limit_reached
No second lifecycle row created

Existing first session ordinary GET:
HTTP 200

First logout:
HTTP 204
Slot released

Replacement session issue:
HTTP 200

Replacement logout:
HTTP 204

Revoked replacement-cookie replay:
HTTP 401 credential_revoked

Successful issue outcomes:
2

Limit-reached failed outcomes:
1

Successful revoke outcomes:
2

Acceptance lifecycle and source identity:
revoked

Original daemon configuration:
restored

SQLite quick check:
ok

SQLite foreign-key check:
empty

VDR domain mutations:
0

Service state:
active

Automatic rollback:
not required
```

Durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2u-20260802T041910Z-16ff04a4ba37/runtime-acceptance-slice2u
```

Runtime report:

```text
/var/backups/vdr-suite-phase62-slice2u-20260802T041910Z-16ff04a4ba37/runtime-acceptance-slice2u/runtime-acceptance-report.txt
```

Runtime report SHA-256:

```text
7c33b06d07beff6b17bad82153ff4a0f1e7c1f5c8d8f972406f8b0b9160f4c89
```

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

## Acceptance result

The Slice-2U acceptance gate is fully satisfied through implementation, focused
tests, architecture checks, final-head source CI, guarded real-yaVDR acceptance
and successful documentation closeout CI.

Verified:

1. implementation remained within the bounded Slice-2U contract;
2. configuration, repository, issuance and HTTP tests passed;
3. architecture and Make-test inventory checks passed;
4. all five source GitHub Actions jobs passed on the final implementation head;
5. guarded real-yaVDR acceptance passed;
6. test lifecycle rows and the isolated source identity were revoked;
7. original configuration was restored;
8. accountability remained secret-free;
9. SQLite integrity checks passed;
10. zero VDR domain mutations occurred;
11. closeout commit `4747d725664d4c382d17d3b19fa2776f48ba437b`
    is covered by final shared CI head
    `d00fc5045a136d87323fbc13fb1bfc1030f7d3b5`;
12. VDR-Suite CI #6693, run `30733265772`, passed all five jobs.

Slice 2U is closed. Its installed daemon, loader and runtime-evidence
fingerprints remain authoritative and unchanged. The separately selected next
workstream is Slice 2V; it must not reopen or repeat Slice-2U runtime acceptance.
