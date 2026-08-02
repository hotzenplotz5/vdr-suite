# VDR-Suite Current State

## Navigation

- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Post-Slice-2W New Chat Prompt](development/phase-62-post-slice-2w-new-chat-prompt.md)
- [Current Project Status](development/current-status.md)
- [Slice 2X — Protected Mutation Response Outcomes](development/phase-62-slice-2x-protected-mutation-response-outcomes.md)
- [Slice 2W Runtime Closeout](development/phase-62-slice-2w-runtime-closeout.md)
- [Slice 2W Contract](development/phase-62-slice-2w-browser-session-retention-cleanup.md)
- [Phase 62 Runtime Evidence through Slice 2V](development/phase-62-runtime-evidence.md)
- [Phase 62 Gap Matrix](planning/phase-62-security-identity-gap-matrix.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [Security and Identity Architecture](architecture/security-identity-foundation.md)
- [ADR Index](adr/index.md)
- [Phase 61 and Performance Closeout](development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](development/post-phase-61-platform-runtime-closeout.md)
- [Completed Phases](development/completed-phases.md)

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Base: main @ cb77ff66e11dca7db2eafa36525762dcde35102d
Active PR: #117
PR state: open, Draft, unmerged, mergeable
Remote branch: phase-62-security-identity-foundation
Local yaVDR branch: phase62-pr117

Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Completed post-phase platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)
Configurable photorealistic VDR Remote (#115)

Next strict runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Current active runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Repository, source CI and real-runtime acceptance complete through:
Slice 2W - Browser-Session Terminal Retention Cleanup

Accepted Slice-2W source/runtime head:
bb8609151313c613d403b88b1b4c3f55453a93e2

Accepted Slice-2W source CI:
VDR-Suite CI #6834
Run ID 30745952119
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30745952119

Runtime acceptance:
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

Next bounded implementation slice:
Slice 2X - Protected Mutation Response Outcomes

Selection state:
contract/documentation only; no production implementation
```

Phase 61 remains complete. Phase 62 remains active and incomplete. Phase 63-67
runtime has not been advanced.

## Accepted security request and lifecycle path

```text
HTTP request
  -> strict browser-cookie precedence when present
  -> otherwise Legacy Basic or optional Managed Basic
  -> persistent actor/device/session/credential resolution
  -> issuing-credential request-time lifecycle binding
  -> immutable absolute lifetime and optional idle effectiveness
  -> exact route classification and scope extraction
  -> cookie-bound CSRF for browser mutations
  -> exact permission and fixed-role authorization
  -> append-only pre-dispatch accountability
  -> browser lifecycle outcome accountability
  -> existing router, backend and domain safety policy
```

Backend read-only, capability and domain policy remain independent from actor
authorization. Frontends do not own authorization decisions.

## Fully accepted Slice 2W

Slice 2W introduced:

```text
VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS
0                 disabled compatibility default
86400..31536000   enabled retention delay
fixed batch size  256
```

Exactly one bounded cleanup pass runs during Security Runtime startup after all
security schemas and configuration have been validated and before
`securityReady`.

Eligibility is limited to the verifier's own explicit revocation, absolute
expiry or idle expiry beyond retention. Candidates are processed oldest terminal
first and then by token ID.

One `BEGIN IMMEDIATE` transaction rechecks eligibility, appends exact secret-free
`browser.session.cleanup` accountability, deletes the verifier and deletes only
its unreferenced canonical browser session and exact-type `browser-session`
credential. Actors, devices, issuing credentials, grants, roles and audit history
are preserved. Any enabled cleanup failure leaves the Security Runtime fail
closed and rolls back the whole batch.

The real yaVDR acceptance proved:

- fresh schema initialization;
- disabled-policy no-op;
- fail-closed rollback after forced accountability failure;
- preservation of active and within-retention lifecycles;
- deletion of old revoked, absolute-expired and idle-expired lifecycles;
- no issuer-only cascade;
- preservation of non-browser credentials and still-referenced canonical rows;
- exact secret-free accountability;
- fixed bounded behavior: 258 candidates and exactly 256 deterministic
  deletions;
- SQLite quick and foreign-key checks;
- unchanged production database, configuration and loader;
- removed runtime systemd override;
- final active accepted daemon;
- zero VDR domain mutations.

Do not repeat Slice-2W runtime acceptance unless a directly relevant daemon,
cleanup, schema, configuration, systemd execution or harness fingerprint changes.

## Cumulative accepted Phase 62 scope

Accepted through Slice 2W:

- canonical actor, device, session, credential, request and correlation context;
- persistent identity, Managed Basic and browser-session verifiers;
- atomic browser-session issue/logout with independent cookie and CSRF secrets;
- strict cookie precedence and ordinary-route browser authentication;
- exact actor grants and fixed exact-scope Admin/Read-only roles;
- memory-only Webfrontend CSRF ownership;
- protected Remote, Timer, Channel Move, Recording, SearchTimer and Native Fuzzy
  mutation families;
- explicit Safe POST classification;
- immutable absolute browser-session lifetime;
- browser issue/revoke outcome accountability;
- request-time issuing-credential lifecycle binding;
- optional per-actor effective browser-session concurrency limits;
- optional browser-session idle expiry with throttled `last_seen_at` persistence;
- bounded terminal browser-session retention cleanup with atomic accountability;
- guarded real-runtime acceptance and rollback tooling.

## Selected Slice 2X — necessity proof

The next slice is selected because it is required by an explicit Phase-62 exit
criterion and because the accepted code has a directly observable gap.

Required result:

```text
every privileged mutation has actor, decision and outcome evidence
```

Current behavior:

- `SecurityHttpGate` persists actor and authorization decision before dispatch;
- `TestHttpServer` then returns the router result without a business-mutation
  outcome event;
- therefore an authorized successful mutation and an authorized returned error
  are indistinguishable in accountability persistence.

Slice 2X adds exactly one post-router result event for already-protected
mutations. It adds no route, permission, role, schema, repository, configuration
or frontend.

A protected audit read endpoint was rejected because no Phase-62 requirement or
failure case proves it necessary.

## Remaining Phase 62 decision boundary

Proven mandatory before closeout:

- Slice 2X protected mutation response outcomes.

Not currently proven necessary:

- protected audit reads, export, filters, redaction or retention;
- generic security administration;
- native/service credential lifecycle before a real client requires it;
- universal revisions, idempotency or durable operation framework;
- transactional Outbox or generic cross-system coupling.

These items must not be implemented unless a binding requirement, concrete code
gap and real failure case are documented first.

After Slice 2X acceptance, perform only compatibility-retirement readiness and
final Phase-62 closeout analysis. Do not assume additional implementation is
needed.

## Operating rules

- Root-level `AGENTS.md` is binding.
- Prefer GitHub-first edits when the connector can safely complete them.
- Continue through already-approved bounded work without artificial pauses.
- Evaluate CI on the final stabilization head.
- Every CI report includes direct link, run number, run ID, exact head, overall
  status and all five job statuses.
- PR #117 remains open, Draft and unmerged.
- Do not mark it Ready, merge, auto-merge, rebase, force-push or rewrite history.
- Do not repeat accepted runtime work merely because the chat changed.
- Select and implement exactly one bounded Phase 62 slice at a time.
- No feature is selected without a requirement-to-code failure proof.
- Do not pull Android, Android TV or Phase 63-67 runtime into Phase 62.

## Exact next action

Complete the Slice-2X selection/handoff documentation and require all five CI
jobs on the final selection head. After that CI is fully green, implement only
[Slice 2X — Protected Mutation Response Outcomes](development/phase-62-slice-2x-protected-mutation-response-outcomes.md).

Do not implement an audit reader, administration API, Outbox, generic operation
framework or native/service lifecycle without a separate necessity proof.

Do not reopen Slice 2W without a changed relevant acceptance fingerprint.
