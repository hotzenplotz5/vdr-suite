# Phase 62 Security and Identity Gap Matrix

Status: active Phase 62 planning and implementation matrix

```text
Repository baseline:
cb77ff66e11dca7db2eafa36525762dcde35102d

Accepted runtime slices:
Slice 1 through Slice 2W

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

Runtime report SHA-256:
e0fbe1689b2f48e75bb4ae6836b227d7da92e08d53b009ac1c2cb371a36c74ea

Durable evidence:
/var/backups/vdr-suite-phase62-slice2w-20260802T114239Z-bb8609151313

Next bounded implementation slice:
not yet selected

PR #117:
open, Draft, unmerged
```

A component is not accepted installed runtime until it is connected, covered by
the complete CI graph and validated on the real yaVDR system. Code-head evidence
alone is insufficient.

## Gap matrix

| Security area | Current accepted state | Remaining gap | Candidate later work |
|---|---|---|---|
| Actor/device model | Canonical persistent actor, device, session and credential context | Protected enrollment and administration | Bounded lifecycle-administration slice |
| Authentication | Legacy Basic, optional Managed Basic and browser sessions; strict cookie precedence; issuer binding, absolute expiry, idle expiry and terminal retention accepted | Native/service mechanisms and compatibility retirement | Native/service credential lifecycle or retirement slice |
| Browser sessions | Atomic issue/logout, independent secrets, persistence, absolute expiry, replay denial, outcomes, issuer binding, concurrency limit, idle expiry and bounded terminal cleanup accepted | Listing, logout-all and protected administration remain absent | Later separate administration design |
| Browser-session retention | One bounded startup pass deletes only old terminal verifiers and unreferenced canonical browser rows with exact accountability | No periodic scheduler or operator-facing administration | Closed for Slice 2W; scheduling/admin remain separate optional gaps |
| Browser-session idle expiry | `last_seen_at`, strict optional idle policy, shared cookie/CSRF effectiveness and 60-second write throttle accepted | None for request-time idle effectiveness | Preserve accepted contract |
| Concurrent browser sessions | Optional `0..64` effective-session limit with atomic deny-new semantics accepted | No automatic eviction | Preserve deny-new semantics unless a future slice explicitly selects eviction |
| Issuing credential lineage | Issuer revalidated on every browser request; terminal cleanup does not cascade solely from issuer state | No descendant lifecycle administration | Later explicit lifecycle-administration design |
| Grants and scopes | Exact actor grants and fixed scopes accepted | Protected grant administration | Later bounded administration design |
| Fixed roles | Exact-scope Admin and Read-only accepted | Generic persisted roles and assignments | Later role-administration design |
| CSRF | Enforced for accepted browser mutations with memory-only frontend token | Future owners require explicit contracts | Preserve in every future route slice |
| Central authorization | All registered central business and administrative POST families classified | No remaining product POST migration gap | No further route-migration slice |
| Browser lifecycle outcomes | Issue, revoke and cleanup outcomes accepted | Other operation outcomes and stronger coupling | Bounded broader-outcomes slice candidate |
| Accountability | Pre-dispatch, lifecycle outcomes, concurrency/idle denials and cleanup writes accepted and secret-free | Protected reads, export, redaction and audit retention | Bounded audit-product slice candidate |
| Revisions/idempotency | Domain-specific partial mechanisms only | Common preconditions, idempotency and durable operation lifecycle | Bounded operation-lifecycle slice candidate |
| Administration | No general security-management API | Protected identity, credential, grant and role operations | One or more later bounded administration slices |
| Native/service clients | Core model is transport-neutral | Enrollment, rotation and revocation contracts | Bounded native/service credential slice candidate |
| Compatibility retirement | Legacy compatibility remains transitional | Retirement criteria, migration and operational tooling | Near final Phase 62 closeout |

## Closed Slice 2W contract

```text
VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS
0                 disabled compatibility default
86400..31536000   enabled retention delay in seconds
fixed batch size  256
```

Accepted trigger:

- one bounded pass during Security Runtime initialization;
- after all security schemas and complete configuration validation;
- before `securityReady`;
- no scheduler, background thread or request-path cleanup.

Accepted eligibility:

- explicit browser revocation older than retention;
- absolute expiry older than retention;
- idle expiry older than retention when idle policy is enabled;
- deterministic oldest-terminal-first order and then token ID;
- at most 256 browser lifecycles.

Accepted atomic ownership:

1. re-evaluate eligibility inside `BEGIN IMMEDIATE`;
2. append secret-free `browser.session.cleanup` accountability;
3. delete the browser verifier;
4. delete its canonical session only when unreferenced;
5. delete its credential only when it is type `browser-session` and
   unreferenced;
6. preserve actor, device, issuer, grants, roles and accountability;
7. roll back the complete batch on any failure.

Accepted real-runtime proof:

- disabled policy no-op;
- full rollback and HTTP 503 fail-closed behavior after forced accountability
  failure;
- active and within-retention preservation;
- old revoked, absolute-expired and idle-expired deletion;
- no issuer-only cleanup;
- non-browser credential and re-referenced canonical-row preservation;
- exact secret-free cleanup events;
- 258 candidates with exactly 256 deterministic deletions;
- SQLite integrity, unchanged production database/configuration/loader, removed
  runtime override, active final daemon and zero VDR domain mutations.

## Phase 62 dependency order

1. Identity and authorization foundation — accepted.
2. Persistent lifecycle, browser sessions, exact grants and fixed roles — accepted.
3. Business and administrative POST migration — accepted through Slice 2Q.
4. Absolute browser-session lifetime — Slice 2R accepted.
5. Browser issue/revoke outcome accountability — Slice 2S accepted.
6. Issuing-credential lifecycle binding — Slice 2T accepted.
7. Concurrent effective browser-session limit — Slice 2U accepted.
8. Browser-session idle expiry and throttled activity — Slice 2V accepted.
9. Browser-session terminal retention cleanup — Slice 2W accepted.
10. Fresh post-Slice-2W gap analysis and one bounded selection — next action.
11. Common revisions, idempotency and durable operation lifecycle — open.
12. Broader outcomes, coupling/Outbox and protected audit reads — open.
13. Protected identity, credential, grant and generic-role administration — open.
14. Native/service credential lifecycle — open.
15. Compatibility retirement readiness and final Phase 62 closeout — open.

## Selection constraints for the next slice

The next selection must:

- close one concrete remaining repository gap;
- have a small coherent owner set;
- preserve all accepted browser lifecycle behavior;
- define exact fail-closed and accountability semantics;
- include focused source tests, an architecture guard and a bounded real-runtime
  acceptance path;
- avoid combining administration, operation lifecycle, audit product and
  native/service credential work into one slice;
- avoid Android, Android TV and Phase 63-67 runtime.

## Exact next action

Perform one fresh post-Slice-2W gap analysis. Compare the concrete remaining
candidates by dependency order, risk, owner set, source testability and safe
runtime acceptance. Select and document exactly one smallest coherent next
Phase-62 slice, update the canonical handoff and require all five CI jobs before
implementation.

Do not reopen Slice 2W without a changed relevant acceptance fingerprint.