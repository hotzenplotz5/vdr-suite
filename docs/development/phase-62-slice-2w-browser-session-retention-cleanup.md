# Phase 62 Slice 2W — Browser-Session Terminal Retention Cleanup

## Status

**Fully implemented, source-verified and accepted on the real yaVDR runtime.**

The complete runtime closeout is recorded in
[Phase 62 Slice 2W Runtime Closeout](phase-62-slice-2w-runtime-closeout.md).

PR #117 remains open, Draft and unmerged. Phase 62 remains active and
incomplete. Phase 63-67 runtime has not been advanced.

## Purpose

Slice 2W closes the physical-retention gap for old terminal browser-session
lifecycles. It adds one repository-owned, bounded cleanup pass without changing
request-time authentication, authorization, CSRF, absolute expiry, idle expiry
or concurrent-session semantics.

## Accepted configuration

```text
VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS
0                 disabled compatibility default
86400..31536000   enabled retention delay in seconds
fixed batch size  256
```

Parsing is strict unsigned decimal. Signs, whitespace, suffixes, partial parses,
overflow, non-zero values below one day and values above 365 days are rejected.
Invalid enabled configuration keeps the Security Runtime unavailable.

## Trigger and ownership

Exactly one bounded pass runs during Security Runtime initialization:

1. open the security database;
2. ensure accountability, identity and browser-session schemas;
3. parse and validate the complete security configuration;
4. run the cleanup transaction when retention is enabled;
5. construct the remaining authenticators, lifecycle services and gates;
6. set `securityReady` only after cleanup succeeds.

There is no periodic timer, scheduler, background thread or request-path
cleanup.

## Terminal eligibility

A browser verifier is eligible only when its own lifecycle has remained terminal
for at least the configured retention delay.

Accepted terminal sources:

- explicit revocation via `revoked_at`;
- absolute expiry via `expires_at`;
- idle expiry via `last_seen_at` when the Slice-2V idle policy is enabled.

Candidates are ordered by oldest terminal time first and then by `token_id`.
At most 256 candidates are processed per startup pass.

Issuer revocation alone is not a cleanup trigger. Slice-2T request-time issuer
binding remains unchanged.

## Atomic deletion contract

One `BEGIN IMMEDIATE` transaction:

1. rechecks eligibility inside the write transaction;
2. appends exact secret-free cleanup accountability;
3. deletes the browser verifier;
4. deletes the matching canonical session only when it still matches the
   selected actor/device and no browser verifier references it;
5. deletes the matching canonical credential only when it belongs to the
   selected actor, has type exactly `browser-session` and is unreferenced;
6. preserves actor, device, issuing credential, grants, roles and accountability
   history;
7. commits only after the whole selected batch succeeds.

Any SQL, foreign-key or accountability failure rolls back the complete batch.
Partial cleanup is forbidden.

## Accountability contract

Each deleted verifier receives one event:

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

The event may contain the non-secret actor, device and session identifiers
needed for accountability. It never contains session/CSRF secrets, verifier
hashes, cookie values, Authorization headers, process environments or raw
configuration.

## Accepted tests and guards

Source tests and the architecture guard prove:

- default `0` is a no-op;
- strict lower/upper bounds and invalid-value rejection;
- active and within-retention rows are preserved;
- old revoked, absolute-expired and idle-expired rows are eligible;
- idle eligibility is disabled when idle policy is disabled;
- deterministic ordering and the fixed 256-row bound;
- atomic verifier/session/credential cleanup;
- actor, device, issuer, grants, roles and accountability preservation;
- non-`browser-session` credentials are not deleted;
- referenced canonical sessions and credentials are not deleted;
- exact secret-free cleanup accountability;
- complete rollback after forced accountability or SQL failure;
- enabled cleanup failure leaves the Security Runtime fail closed;
- no HTTP handler, scheduler, issuer cascade or concurrency eviction is added.

## Accepted source and runtime evidence

```text
Accepted source/runtime head:
bb8609151313c613d403b88b1b4c3f55453a93e2

VDR-Suite CI #6834
Run ID: 30745952119
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30745952119

PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE=PASS

Installed/running daemon SHA-256:
7775804306bf70eca6ef23474605467381162cfc9d5b874cdb187840ca8bc571

Installed deferred-runtime-loader.js SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Daemon configuration SHA-256:
8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b

Runtime report SHA-256:
e0fbe1689b2f48e75bb4ae6836b227d7da92e08d53b009ac1c2cb371a36c74ea

Durable evidence:
/var/backups/vdr-suite-phase62-slice2w-20260802T114239Z-bb8609151313
```

The isolated runtime pass proved fresh schema initialization, disabled no-op,
fail-closed rollback after forced accountability failure, all enabled
preservation/deletion boundaries, exact secret-free audit events, 258 candidates
with exactly 256 deterministic deletions, SQLite integrity, unchanged production
database/configuration/loader, removed runtime override, final active daemon and
zero VDR domain mutations.

Do not repeat the acceptance unless a directly relevant daemon, cleanup, schema,
configuration, systemd execution or harness fingerprint changes.

## Explicit exclusions

Slice 2W does not add:

- periodic cleanup scheduling;
- session listing, logout-all or per-session administration API/UI;
- cleanup driven solely by issuer revocation;
- actor, device, issuer, grant, role or accountability deletion;
- generic identity or credential administration;
- automatic concurrency-limit eviction;
- sliding absolute expiry or refresh tokens;
- generic operation outcomes or Outbox infrastructure;
- audit read, export, redaction or retention products;
- Android, Android TV or Phase 63-67 runtime.

## Exact next action

Perform one fresh post-Slice-2W gap analysis and select exactly one smallest
coherent next Phase-62 slice. Document its contract and require fully green
selection CI before implementation.

Do not reopen Slice 2W without a changed relevant acceptance fingerprint.