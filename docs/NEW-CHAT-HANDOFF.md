# VDR-Suite New Chat Handoff

## Purpose

This is the canonical entry point for every new VDR-Suite chat. Read it before
repeating repository-wide analysis or real-runtime acceptance. A new chat alone
is not a reason to start over.

Trust completed items marked **VERIFIED** unless a directly relevant repository,
binary, configuration, database, systemd execution, routing or behaviour
fingerprint changed.

## Canonical reading

- [Ready-to-copy continuation prompt](development/phase-62-post-slice-2w-new-chat-prompt.md)
- [Current project truth](CURRENT.md)
- [Current project status](development/current-status.md)
- [Slice 2X selected contract](development/phase-62-slice-2x-protected-accountability-event-read.md)
- [Slice 2W runtime closeout](development/phase-62-slice-2w-runtime-closeout.md)
- [Slice 2W accepted contract](development/phase-62-slice-2w-browser-session-retention-cleanup.md)
- [Phase 62 runtime evidence through Slice 2V](development/phase-62-runtime-evidence.md)
- [Phase 62 gap matrix](planning/phase-62-security-identity-gap-matrix.md)
- [Security and identity architecture](architecture/security-identity-foundation.md)
- [Strict roadmap](planning/roadmap.md)
- [Phase map](planning/phase-map.md)
- [Parity and frontend gap roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture audit gap matrix](planning/architecture-audit-gap-matrix.md)
- [Completed phases](development/completed-phases.md)
- [Phase 61 closeout](development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 runtime closeout](development/post-phase-61-platform-runtime-closeout.md)
- [ADR index](adr/index.md)
- [Agent workflow rules](../AGENTS.md)

Current State, Current Status, the Slice-2W closeout and the Slice-2X selection
contract are the newest Phase-62 authorities. The cumulative runtime-evidence
document remains authoritative for older accepted slices through Slice 2V.

## Stable project position

```text
Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Historical umbrella track:
Phase 58 - Frontend and Live Parity

Completed platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)
Configurable photorealistic VDR Remote (#115)

Current active phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Phase 62 state:
active and incomplete

Repository, source CI and real-runtime acceptance complete through:
Slice 2W - Browser-Session Terminal Retention Cleanup

Selected next bounded slice:
Slice 2X - Protected Accountability Event Read

Slice-2X state:
selection and contract only; no production implementation

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

Selected next implementation slice:
Slice 2X - Protected Accountability Event Read

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

The accepted daemon remains installed and active. The production database,
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

Terminal eligibility is limited to the browser verifier's own explicit
revocation, absolute expiry or idle expiry beyond retention. Issuer revocation
alone is not a cleanup trigger.

One `BEGIN IMMEDIATE` transaction rechecks eligibility, appends exact secret-free
`browser.session.cleanup` accountability, deletes the verifier and deletes only
its unreferenced canonical browser session and exact-type `browser-session`
credential. Actor, device, issuing credential, grants, roles and accountability
history are preserved. Any enabled-policy failure rolls back the complete batch
and leaves the Security Runtime fail closed.

The real-runtime pass proved fresh schema initialization, disabled-policy no-op,
fail-closed HTTP 503 and full rollback, active/within-retention preservation,
old terminal lifecycle deletion, no issuer-only cascade, foreign credential and
re-referenced canonical-row preservation, exact secret-free events, 258 eligible
lifecycles with exactly 256 deterministic deletions, SQLite integrity, unchanged
production state, removed runtime override, final active daemon and zero VDR
domain mutations.

Do not repeat Slice-2W runtime acceptance merely because a chat changes. Repeat
only after a directly relevant daemon, cleanup implementation, schema,
configuration, systemd execution or acceptance-harness fingerprint changes.

## Fresh post-Slice-2W gap analysis

**COMPLETED:** exactly one next bounded slice is selected:

```text
Phase 62 Slice 2X
Protected Accountability Event Read
```

The selection compared all remaining candidates. Slice 2X was chosen because it
closes a concrete security-observability gap using the already accepted
append-only repository and exact authorization model, changes no VDR domain
state, has a small owner set and can withhold all data when audit-of-audit fails.

The following candidates remain deferred:

- generic mutation outcomes and stronger transaction coupling/Outbox;
- common revisions, idempotency and durable operation lifecycle;
- actor, identity, credential, grant and role administration;
- native/service credential lifecycle;
- audit export, configurable redaction and retention;
- compatibility retirement.

## Selected Slice 2X contract

Exact HTTP surface:

```text
GET /api/security/accountability/events
GET /api/security/accountability/events?limit=<1..100>
default limit 50
newest first by recorded_at DESC, event_id DESC
```

Exact authorization:

```text
security.audit.read@*
```

Rules:

- anonymous, invalid, expired or revoked identity state is denied;
- missing or wrong-scope grants are denied;
- direct exact `security.audit.read@*` grants are accepted;
- exact global `role.admin@*` explicitly grants the read;
- non-global admin scope and `role.read-only` do not grant it;
- Legacy Basic compatibility cannot bypass this sensitive-read authorization;
- browser GET does not require CSRF because the route is read-only.

The HTTP response is a fixed no-store projection of already secret-free
accountability fields. It contains no request/response bodies, HTTP headers,
cookies, Authorization values, passwords, hashes, session/CSRF secrets, raw
configuration or process environment.

The existing pre-dispatch authorization event remains mandatory. The bounded
SELECT and one exact `operation.succeeded` read outcome execute in one local
`BEGIN IMMEDIATE` transaction. Query, outcome append or commit failure returns no
rows. This does not implement generic mutation Outbox semantics.

No configuration variable is added.

## Slice 2X exclusions

- audit export, download or streaming;
- pagination, offsets, filters, arbitrary time ranges or search;
- configurable redaction, retention, deletion, compaction or archival;
- frontend audit viewer;
- security administration;
- generic mutation outcomes, Outbox or cross-domain coupling;
- revisions, `If-Match`, idempotency or durable operation replay;
- native/service credential enrollment, rotation or revocation;
- compatibility retirement;
- Android, Android TV or Phase 63-67 runtime.

## Slice 2X tests and guard

The documented implementation must add focused coverage for:

- idempotent schema/index creation and deterministic bounded ordering;
- successful empty results distinct from repository failure;
- exact direct/admin authorization and all deny paths;
- Legacy Basic compatibility non-bypass;
- atomic bounded read plus mandatory success outcome;
- forced SELECT/outcome/commit failure with no rows returned;
- exact query grammar and fixed JSON field allowlist;
- no-store and request/correlation headers;
- browser GET without CSRF only after authorization;
- no general `ApiRouter`, frontend, export or retention ownership.

A dedicated architecture guard must enforce the exact sensitive GET, maximum
100-row bound, repository-only SQL, explicit global role mapping, fixed
secret-free serializer and all exclusions.

## Slice 2X runtime boundary

No runtime acceptance is needed for the documentation-only selection. After
implementation and fully green source CI, a guarded isolated-database acceptance
must prove the exact 401/403/200 matrix, deterministic `limit=2`, invalid-limit
400s, fixed no-store projection, exact authorization/read-outcome evidence,
forced failure 503 with no rows, SQLite integrity, unchanged production
database/configuration/loader, removed temporary grants/override, active final
daemon and zero VDR domain mutations.

Do not repeat Slice 2W scenarios unless a Slice-2W fingerprint changes.

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

The post-Slice-2W gap analysis is complete. Do not select another slice or reopen
the comparison while Slice 2X remains the active bounded workstream.

## Selection gate and exact next action

1. Complete the canonical Slice-2X selection/status/gap/handoff documentation.
2. Require all five GitHub Actions jobs on the final selection head:
   `docs-check`, `make-test-audit`, `frontend-regression-test`,
   `fast-regression-test`, `packaging-regression-test`.
3. Stop if any required job fails and fix only the demonstrated documentation or
   guard problem.
4. After and only after the fully green selection gate, implement exactly the
   bounded Slice-2X contract.
5. Do not start export, retention, generic outcomes, Outbox, revisions,
   idempotency, administration, credential lifecycle, Android, Android TV or
   Phase 63-67 runtime.

## Maintenance rule

Update this file whenever repository, PR, runtime, routing, selected-slice or
next-action truth changes. Preserve durable non-secret evidence and keep the next
permitted action explicit.