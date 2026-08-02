# Phase 62 Security and Identity Gap Matrix

Status: active Phase 62 planning and implementation matrix

```text
Repository baseline:
cb77ff66e11dca7db2eafa36525762dcde35102d

Accepted runtime slices:
Slice 1 through Slice 2V

Accepted Slice-2V implementation/runtime head:
e84415fadb2587ff744ff8927f1f0113920ece2f

Accepted Slice-2V source CI:
VDR-Suite CI #6779
Run ID 30741293079
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30741293079

Accepted Slice-2V closeout head:
cf31b2b67f73f12718601ced5468a59a1183adcb

Accepted Slice-2V closeout CI:
VDR-Suite CI #6799
Run ID 30742295881
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30742295881

Selected next bounded slice:
Slice 2W - Browser-Session Terminal Retention Cleanup

Slice-2W state:
selection documented; implementation not started

PR #117:
open, Draft, unmerged
```

A component is not accepted installed runtime until it is connected, covered by
the complete CI graph and validated on the real yaVDR system. Code-head evidence
alone is insufficient.

## Gap matrix

| Security area | Current accepted state | Remaining gap | Selected or later work |
|---|---|---|---|
| Actor/device model | Canonical persistent actor, device, session and credential context | Protected enrollment and administration | Later lifecycle-administration slice |
| Authentication | Legacy Basic, optional Managed Basic and browser sessions; strict cookie precedence; issuer binding, absolute expiry and idle expiry accepted | Native/service mechanisms and compatibility retirement | Preserve during Slice 2W |
| Browser sessions | Atomic issue/logout, independent secrets, persistence, absolute expiry, replay denial, outcomes, issuer binding, concurrency limit and idle expiry accepted | Terminal physical retention cleanup and administration | **Slice 2W selected for terminal retention cleanup only** |
| Browser-session retention | No physical deletion API exists in the browser or canonical identity repositories | Bounded deletion of old terminal browser lifecycles | **Slice 2W selected** |
| Browser-session idle expiry | `last_seen_at`, strict optional idle policy, shared cookie/CSRF effectiveness and 60-second write throttle accepted | Physical retention of terminal rows | Slice 2V closed; Slice 2W consumes its terminal state |
| Concurrent browser sessions | Optional `0..64` effective-session limit with atomic deny-new semantics accepted | No automatic eviction; old terminal rows persist | Preserve deny-new semantics; Slice 2W is not eviction |
| Issuing credential lineage | Issuer is revalidated on every browser request | No cascading descendant cleanup | Slice 2W must not delete solely because issuer is revoked |
| Grants and scopes | Exact actor grants and fixed scopes accepted | Protected grant administration | Later bounded administration design |
| Fixed roles | Exact-scope Admin and Read-only accepted | Generic persisted roles | Later administration design |
| CSRF | Enforced for accepted browser mutations with memory-only frontend token | Future owners require explicit contracts | No Slice-2W route/frontend change |
| Central authorization | All registered business and administrative POST families classified | No remaining product POST migration gap | No further route-migration slice |
| Browser lifecycle outcomes | Issue/revoke outcomes accepted | Other operation outcomes and stronger coupling | Later separate slice |
| Accountability | Pre-dispatch, lifecycle outcomes, concurrency and idle denials accepted and secret-free | Cleanup evidence plus later protected reads/export/retention | Slice 2W adds only cleanup write evidence |
| Revisions/idempotency | Domain-specific partial mechanisms only | Common preconditions and durable operation lifecycle | Later Phase 62 slice |
| Administration | No general security-management API | Protected identity, credential, grant and role operations | Later separate slices |
| Native/service clients | Core model is transport-neutral | Enrollment, rotation and revocation contracts | Later Phase 62 slice |
| Audit reads and retention | Append-only writes exist | Protected reads, export, redaction and audit retention | Later audit-product slice |
| Compatibility retirement | Legacy compatibility remains transitional | Retirement criteria and migration tooling | Near final Phase 62 closeout |

## Selected Slice 2W contract

Configuration:

```text
VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS
0                 disabled compatibility default
86400..31536000   enabled retention delay in seconds
fixed batch size  256
```

Selected trigger:

- one bounded pass during Security Runtime initialization;
- after schema and configuration validation;
- before `securityReady`;
- no scheduler, background thread or request-path cleanup.

Selected eligibility:

- explicit browser revocation older than retention;
- absolute expiry older than retention;
- idle expiry older than retention when idle policy is enabled;
- deterministic oldest-terminal-first ordering;
- at most 256 browser lifecycles.

Selected atomic ownership:

1. re-evaluate eligibility inside `BEGIN IMMEDIATE`;
2. append secret-free `browser.session.cleanup` accountability;
3. delete the browser verifier;
4. delete its canonical session only when unreferenced;
5. delete its credential only when it is type `browser-session` and unreferenced;
6. preserve actor, device, issuer, grants, roles and accountability;
7. rollback the entire batch on any failure.

Selected fail-closed behaviour:

- disabled policy is a no-op;
- invalid enabled configuration prevents Security Runtime readiness;
- cleanup SQL, foreign-key or accountability failure prevents readiness;
- no partial cleanup is accepted.

## Explicit Slice-2W exclusions

- periodic cleanup scheduling;
- cleanup in an HTTP request path;
- session listing, logout-all or administration API/UI;
- automatic eviction to satisfy concurrency limits;
- cleanup triggered solely by issuer revocation;
- actor, device, issuer, grant, role or accountability deletion;
- generic identity or credential cleanup;
- generic outcomes, Outbox, revisions or idempotency;
- Android, Android TV or Phase 63-67 runtime.

## Phase 62 dependency order

1. Identity and authorization foundation — accepted.
2. Persistent lifecycle, browser sessions, exact grants and fixed roles — accepted.
3. Business and administrative POST migration — accepted through Slice 2Q.
4. Absolute browser-session lifetime — Slice 2R accepted.
5. Browser issue/revoke outcome accountability — Slice 2S accepted.
6. Issuing-credential lifecycle binding — Slice 2T accepted.
7. Concurrent effective browser-session limit — Slice 2U accepted.
8. Browser-session idle expiry and throttled activity — Slice 2V fully accepted.
9. Browser-session terminal retention cleanup — **Slice 2W selected; not implemented**.
10. Common revisions, idempotency and durable operation lifecycle — open.
11. Broader outcomes, coupling/Outbox and protected audit reads — open.
12. Protected identity, credential, grant and generic-role administration — open.
13. Native/service credential lifecycle — open.
14. Compatibility retirement readiness and final Phase 62 closeout — open.

## Exact next action

Require all five GitHub Actions jobs for the documentation-only Slice-2W
selection head.

After green selection CI, implement only the selected Slice-2W configuration,
repository/service cleanup transaction, startup integration, focused tests,
architecture guard and Make-test registration.

Do not combine Slice 2W with scheduling, administration APIs, issuer cascade,
automatic eviction, generic security administration, Outbox, Android or Phase
63-67 work.
