# VDR-Suite New Chat Handoff

## Purpose

This is the canonical entry point for every new VDR-Suite chat. Read it before
repeating repository-wide analysis or real-runtime acceptance. A new chat alone
is not a reason to start over.

Trust completed items marked **VERIFIED** unless a directly relevant repository,
binary, configuration, database, systemd execution, routing or behaviour
fingerprint changed.

## Canonical reading

- [Ready-to-copy post-Slice-2W prompt](development/phase-62-post-slice-2w-new-chat-prompt.md)
- [Current project truth](CURRENT.md)
- [Current project status](development/current-status.md)
- [Slice 2W runtime closeout](development/phase-62-slice-2w-runtime-closeout.md)
- [Slice 2W accepted contract](development/phase-62-slice-2w-browser-session-retention-cleanup.md)
- [Phase 62 runtime evidence through Slice 2V](development/phase-62-runtime-evidence.md)
- [Phase 62 gap matrix](planning/phase-62-security-identity-gap-matrix.md)
- [Security and identity architecture](architecture/security-identity-foundation.md)
- [Strict roadmap](planning/roadmap.md)
- [Completed phases](development/completed-phases.md)
- [Agent workflow rules](../AGENTS.md)

The post-Slice-2W prompt is the explicit continuation entry point. The Slice-2W
closeout, Current State and Current Status are the newest Phase-62 runtime
authorities. The cumulative runtime-evidence document remains authoritative for
older accepted slices through Slice 2V.

## Stable project position

```text
Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Current active phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Phase 62 state:
active and incomplete

Repository, source CI and real-runtime acceptance complete through:
Slice 2W - Browser-Session Terminal Retention Cleanup

Phase 63-67 runtime:
not advanced
```

## Active workstream

```text
Repository: hotzenplotz5/vdr-suite
Checkout: /home/yavdr/vdr-suite-phase62
Local branch: phase62-pr117
Remote branch: phase-62-security-identity-foundation
Pull request: #117
Base: main @ cb77ff66e11dca7db2eafa36525762dcde35102d
PR state: open, Draft, unmerged, mergeable

Accepted Slice-2W source/runtime head:
bb8609151313c613d403b88b1b4c3f55453a93e2

Accepted Slice-2W source CI:
VDR-Suite CI #6834
Run ID 30745952119
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30745952119

Runtime acceptance:
PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE=PASS

Next bounded implementation slice:
not yet selected

Continuation prompt:
docs/development/phase-62-post-slice-2w-new-chat-prompt.md
```

PR #117 must remain open and Draft. Do not mark it Ready, merge it, enable
auto-merge, rebase, force-push, rewrite branch history or mutate Base, title,
body, reviewers or other review/merge metadata without explicit approval.

PR #118 is the separate paused TVScraper bugfix workstream. Do not mix its code,
daemon fingerprint or planning into Phase 62 commits or evidence.

## Installed real-runtime truth

**VERIFIED on 2026-08-02 after successful Slice-2W acceptance:**

```text
Daemon unit:
vdr-suite-daemon.service

Installed executable:
/usr/sbin/vdr-suite-daemon

Installed/running daemon SHA-256:
7775804306bf70eca6ef23474605467381162cfc9d5b874cdb187840ca8bc571

Installed deferred-runtime-loader.js SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Daemon configuration SHA-256:
8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b

Runtime evidence:
/var/backups/vdr-suite-phase62-slice2w-20260802T114239Z-bb8609151313

Runtime report SHA-256:
e0fbe1689b2f48e75bb4ae6836b227d7da92e08d53b009ac1c2cb371a36c74ea

Final service PID at acceptance:
89965
```

The PID and service timestamps are volatile. The accepted head, binary, loader,
configuration, report and evidence fingerprints are durable repetition gates.

The accepted new daemon remains installed and active. The production database,
daemon configuration and deferred loader were unchanged by the isolated
acceptance. The runtime-only systemd drop-in was removed from the final unit.

## Fully accepted Slice 2W

**VERIFIED:**

```text
VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS
0                 disabled compatibility default
86400..31536000   enabled retention delay
fixed batch size  256
```

Exactly one bounded cleanup pass runs during Security Runtime initialization
after all security schemas and configuration are validated and before
`securityReady`.

Terminal eligibility is limited to the verifier's own explicit revocation,
absolute expiry or idle expiry beyond retention. Issuer revocation alone is not
a cleanup trigger.

One `BEGIN IMMEDIATE` transaction rechecks eligibility, appends exact secret-free
`browser.session.cleanup` accountability, deletes the verifier and deletes only
its unreferenced canonical browser session and exact-type `browser-session`
credential. Actor, device, issuing credential, grants, roles and accountability
history are preserved. Any enabled-policy failure rolls back the complete batch
and leaves the Security Runtime fail closed.

The real-runtime pass proved:

- fresh security-schema initialization;
- disabled-policy no-op;
- fail-closed HTTP 503 and full rollback after forced accountability failure;
- active and within-retention lifecycle preservation;
- old revoked, absolute-expired and idle-expired lifecycle deletion;
- no issuer-only cascade;
- non-browser credential preservation;
- preservation of a canonical session/credential re-referenced inside the
  cleanup transaction;
- exact secret-free cleanup accountability;
- 258 eligible lifecycles with exactly 256 deterministic deletions;
- SQLite quick and foreign-key checks;
- unchanged production database, configuration and loader;
- removed runtime override;
- final active accepted daemon;
- zero VDR domain mutations.

Do not repeat Slice-2W runtime acceptance merely because a chat changes. Repeat
only after a directly relevant daemon, cleanup implementation, schema,
configuration, systemd execution or acceptance-harness fingerprint changes.

## Harness stabilization truth

Two failed attempts are diagnostic history, not accepted runtime evidence:

1. A fixture attempted to violate the real verifier table's unique session and
   credential constraints. The fixture was corrected to reproduce the intended
   re-reference condition using an `AFTER DELETE` trigger.
2. The systemd watcher observed the short pre-`execve()` `Type=simple` start
   window. It was hardened to wait for `MainPID=0` after stop and for a stable
   PID executing the actual ExecStart path after start.

Both failures rolled back automatically, removed the runtime override, restored
the old daemon/configuration and restarted the service. Only the final PASS on
head `bb860915...` and CI #6834 is accepted.

## Credential and secret restrictions

The Managed-Basic plaintext password used for earlier acceptance is unavailable.
Do not rotate or reprovision credentials implicitly.

Never print, store or commit:

- Authorization headers;
- plaintext passwords or password hashes;
- cookies or complete cookie values;
- CSRF tokens;
- raw session or verifier secrets;
- login response bodies containing secrets;
- process environments.

## Anti-loop boundary

Do not repeat completed repository analysis, public-origin work, route migration,
Slice-2R lifetime, Slice-2S outcomes, Slice-2T issuer binding, Slice-2U
concurrency, Slice-2V idle expiry or Slice-2W terminal retention unless a
directly relevant accepted fingerprint or contract changes.

## Remaining Phase 62 gaps

Still open after Slice 2W:

- broader operation outcomes and stronger transaction coupling;
- common revisions, idempotency and durable operation lifecycle;
- protected actor, identity, credential, grant and role administration;
- native/service credential enrollment, rotation and revocation;
- protected audit reads, export, redaction and retention;
- compatibility retirement and final Phase 62 closeout.

No next implementation slice is selected yet.

## Exact next action

1. Verify the current remote head and latest completed CI.
2. Perform one fresh, bounded post-Slice-2W gap analysis using the current gap
   matrix, architecture and roadmap.
3. Compare only concrete smallest coherent candidates and their owners,
   dependencies, tests and runtime boundaries.
4. Select exactly one next Phase-62 slice.
5. Document its configuration/contract, tests, architecture guard, runtime
   acceptance and explicit exclusions.
6. Update Current State, Current Status, Handoff and Gap Matrix.
7. Require all five CI jobs on the final selection/handoff head.
8. Implement only after that selection CI is fully green.

Do not combine multiple remaining security themes. Do not advance Android,
Android TV or Phase 63-67 runtime.

## Maintenance rule

Update this file whenever repository, PR, runtime, routing or next-action truth
changes. Preserve durable non-secret evidence and keep the next permitted action
explicit.