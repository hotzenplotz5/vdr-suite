# Phase 62 Slice 2W — Browser-Session Terminal Retention Cleanup

## Status

Selected as the next bounded Phase 62 slice after the fully accepted Slice 2V.

This document records selection and implementation boundaries only. No Slice-2W
code, installation or runtime mutation is included in this planning commit.

PR #117 remains open, Draft and unmerged.

## Selection result

The post-Slice-2V gap analysis selected exactly one next slice:

```text
Phase 62 Slice 2W
Browser-Session Terminal Retention Cleanup
```

The selection is based on the following repository facts:

- `BrowserSessionCredentialRepository` can create, resolve, count, touch and
  revoke browser-session verifier rows but exposes no physical deletion API;
- `SecurityIdentityRepository` can create, resolve, revoke and adjust expiry for
  canonical sessions and credentials but exposes no physical deletion API;
- the browser verifier owns foreign keys to the canonical browser session,
  browser credential, actor, device and issuing credential;
- Slice 2R established immutable absolute expiry;
- Slice 2T established request-time issuer binding without cascade cleanup;
- Slice 2U established effective-session counting;
- Slice 2V established idle expiry and `last_seen_at` without physical cleanup;
- the remaining cleanup gap is therefore concrete, isolated and directly built
  on the accepted browser lifecycle model.

Broader operation outcomes, Outbox, revisions, idempotency, generic security
administration, native/service credentials and protected audit reads are larger
or cross-cutting and are not selected for Slice 2W.

## Bounded purpose

Slice 2W adds one repository-owned, bounded physical-retention pass for terminal
browser-session lifecycles.

It must reclaim only browser-session-owned rows after a strict retention delay.
It must not change request-time authentication, authorization, CSRF, absolute
expiry, idle-expiry or concurrency-limit semantics.

## Configuration contract

Selected configuration key:

```text
VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS
```

Selected contract:

```text
0                 disabled compatibility default
86400..31536000   enabled retention delay in seconds
```

Parsing requirements:

- unsigned decimal digits only;
- no sign, whitespace, suffix or partial parse;
- overflow rejected;
- non-zero values below one day rejected;
- values above 365 days rejected;
- invalid configured policy prevents the Security Runtime from becoming ready.

The cleanup batch size is a fixed implementation constant:

```text
256 terminal browser lifecycles per startup pass
```

No second environment variable is introduced for batch size.

## Trigger and ownership

The selected integration point is exactly one bounded pass during Security
Runtime initialization:

1. open the security database;
2. ensure accountability, identity and browser-session schemas;
3. parse and validate the complete security configuration;
4. run one Slice-2W cleanup transaction when retention is enabled;
5. continue constructing authenticators, lifecycle services and gates;
6. set `securityReady` only after cleanup succeeds.

Consequences:

- disabled cleanup performs no deletion and preserves compatibility behaviour;
- enabled cleanup failure leaves the existing HTTP security runtime unavailable
  and fail closed;
- no periodic thread, timer, scheduler or request-path opportunistic cleanup is
  introduced;
- a future periodic scheduler, if ever required, is a separate slice.

## Terminal eligibility

A browser lifecycle is eligible only when its own browser verifier has remained
terminal for at least the configured retention delay.

Selected terminal sources:

1. explicit browser revocation:

```text
revoked_at <> ''
revoked_at <= CURRENT_TIMESTAMP - retention
```

2. absolute expiry:

```text
expires_at <= CURRENT_TIMESTAMP - retention
```

3. idle expiry, only when the accepted Slice-2V idle policy is enabled:

```text
last_seen_at <= CURRENT_TIMESTAMP - idle_timeout - retention
```

The deterministic candidate order is oldest terminal time first, then
`token_id`. At most 256 candidates are processed.

Slice 2W does not treat issuer revocation as a cascade-deletion trigger. The
request-time issuer binding from Slice 2T remains effective, while descendant
cleanup based solely on issuer lifecycle remains explicitly deferred.

## Atomic deletion contract

For every selected browser lifecycle, one `BEGIN IMMEDIATE` transaction must:

1. re-evaluate terminal eligibility inside the write transaction;
2. append the required secret-free cleanup accountability event;
3. delete the browser verifier row;
4. delete the matching canonical session only when it still matches the selected
   actor/device and no browser verifier references it;
5. delete the matching canonical credential only when its type is exactly
   `browser-session`, it still belongs to the selected actor and no browser
   verifier references it;
6. preserve the actor, device, issuing credential, grants and all accountability
   rows;
7. commit only when every selected lifecycle is processed successfully.

Any SQL, foreign-key or accountability failure rolls back the complete batch.
Partial cleanup is forbidden.

## Accountability contract

Each deleted browser lifecycle receives one append-only event inside the cleanup
transaction:

```text
event_type=operation.succeeded
classes=security,lifecycle,maintenance
actor_type=system
session_id=<deleted canonical browser session id>
authentication_state=system-maintenance
action=browser.session.cleanup
decision=completed
reason_code=browser_session_retention_elapsed
outcome=deleted
```

The event may contain the non-secret actor, device and session identifiers needed
for accountability. It must not contain:

- session or CSRF secrets;
- verifier hashes;
- cookie values;
- Authorization headers;
- process environments;
- raw configuration content.

If the accountability append fails, the deletion transaction fails and rolls
back.

## Preservation requirements

Slice 2W must preserve:

- all active and effective browser sessions;
- all terminal browser sessions still inside retention;
- absolute-expiry semantics from Slice 2R;
- issuing-credential request-time binding from Slice 2T;
- concurrency counting from Slice 2U;
- idle-expiry and throttled activity semantics from Slice 2V;
- logout and revoked-cookie replay behaviour;
- actor, device, issuer, permission-grant and role rows;
- accountability history;
- public routing and frontend behaviour;
- existing Legacy Basic and Managed Basic compatibility behaviour.

Cleanup must never be used as automatic eviction to make room under the
concurrency limit. Only already-terminal rows beyond retention are eligible.

## Required source tests

Implementation is not complete until focused tests prove:

1. default `0` performs no cleanup;
2. strict parsing accepts `86400` and `31536000`;
3. empty, signed, spaced, suffixed, overflowing, below-minimum and above-maximum
   values are invalid;
4. active sessions are never selected;
5. revoked rows inside retention are preserved;
6. revoked rows beyond retention are removed;
7. absolute-expired rows inside retention are preserved;
8. absolute-expired rows beyond retention are removed;
9. idle-expired rows are considered only when idle timeout is enabled;
10. idle-expired rows inside retention are preserved;
11. idle-expired rows beyond retention are removed;
12. no more than 256 lifecycles are processed;
13. deterministic ordering is stable;
14. browser verifier, canonical session and browser credential cleanup is
    atomic;
15. actor, device, issuer, grants and accountability are preserved;
16. a credential with a type other than `browser-session` is never deleted;
17. a still-referenced session or credential is never deleted;
18. cleanup accountability is exact and secret-free;
19. forced audit failure rolls back the batch;
20. forced SQL or foreign-key failure rolls back the batch;
21. invalid enabled configuration leaves the security runtime unavailable;
22. request-time authentication, logout, replay, concurrency and idle tests remain
    unchanged.

## Required architecture guard

The Slice-2W guard must reject:

- deletion of active or within-retention rows;
- deletion of actors, devices, issuers, grants or accountability;
- `ON DELETE CASCADE` as a substitute for explicit ownership checks;
- cleanup in an HTTP request handler;
- periodic threads or schedulers;
- automatic eviction to satisfy concurrency limits;
- physical cleanup triggered solely by issuer revocation;
- non-atomic browser/session/credential deletion;
- cleanup without accountability;
- secrets or verifier hashes in cleanup evidence;
- an enabled retention value below one day or above 365 days;
- unbounded cleanup.

## Runtime acceptance boundary

A later Slice-2W runtime acceptance must use isolated test lifecycle rows and a
short-lived test database state prepared specifically for the acceptance. It
must prove deletion and preservation counts without deleting any production
actor, device, issuer, grant, role or accountability record.

The runtime pass must include:

- backup and integrity verification;
- exact source/daemon/configuration fingerprints;
- disabled-policy no-op proof;
- enabled bounded cleanup proof;
- inside-retention preservation proof;
- active-session preservation proof;
- exact cleanup accountability proof;
- rollback proof on one forced failure;
- SQLite quick and foreign-key checks;
- zero VDR domain mutations;
- final active service and restored production configuration.

No runtime acceptance is authorized by this selection document alone.

## Explicit exclusions

Slice 2W does not add:

- periodic cleanup scheduling;
- session listing, logout-all or per-session administration APIs;
- HTTP, Webfrontend, Android or Android TV surfaces;
- cleanup driven solely by issuer revocation;
- actor, device, issuer, grant or role deletion;
- generic identity or credential administration;
- automatic concurrency-limit eviction;
- sliding absolute expiry or refresh tokens;
- generic operation outcomes or Outbox infrastructure;
- audit read, export or retention products;
- Phase 63-67 runtime.

## Exact next action

Implement only the selected Slice-2W configuration, repository/service cleanup
transaction, startup integration, focused tests, architecture guard and required
Make-test registration.

Do not combine Slice 2W with a periodic scheduler, HTTP/API administration,
issuer-cascade cleanup, generic security administration, Outbox, Android or
Phase 63-67 work.
