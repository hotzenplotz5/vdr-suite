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

Selected next bounded implementation slice:
Slice 2X - Protected Accountability Event Read

Selection state:
contract/documentation only; no Slice-2X production implementation

PR #117:
open, Draft, unmerged
```

A component is not accepted installed runtime until it is connected, covered by
the complete CI graph and validated on the real yaVDR system. Code-head evidence
alone is insufficient.

## Fresh post-Slice-2W selection

The post-Slice-2W analysis compared the concrete remaining candidates by
security value, dependency order, owner set, source testability, real-runtime
acceptance and scope-expansion risk.

Exactly one next slice is selected:

```text
Phase 62 Slice 2X
Protected Accountability Event Read
```

The binding contract is:

- [Slice 2X — Protected Accountability Event Read](../development/phase-62-slice-2x-protected-accountability-event-read.md)

No Slice-2X production code, schema migration, packaging change, installation or
runtime mutation is part of the selection head.

## Candidate comparison

| Candidate | Security value | Dependencies and owner set | Test/runtime boundary | Scope risk | Selection result |
|---|---|---|---|---|---|
| Protected bounded accountability read | Makes existing security evidence inspectable without changing VDR domain state | Existing identity, exact grants, fixed roles, append-only repository and HTTP security gate | Exact GET, fixed limit, isolated DB, deterministic rows and audit-of-audit failure injection | Low when export, filters, frontend and retention stay excluded | **Selected as Slice 2X** |
| Generic mutation outcomes | Closes privileged success/failure evidence gap | Every protected route and domain owner; unambiguous fail-closed behavior needs stronger coupling or Outbox | Requires post-dispatch failure and partial-mutation scenarios across route families | High | Deferred |
| Transactional Outbox / stronger coupling | Strong mutation durability foundation | Database/domain transaction ownership across heterogeneous operations | Broad crash, retry, delivery and reconciliation acceptance | Very high | Deferred |
| Common revisions, idempotency and durable operation lifecycle | Prevents stale writes and duplicate execution | Stable resource revisions and operation semantics across several domains | Per-resource concurrency, replay and recovery scenarios | High | Deferred |
| Security administration | Enables actor, identity, credential, grant and role changes | Multiple dangerous mutations, lifecycle rules and UI/API ownership | Extensive deny, rollback, recovery and audit scenarios | High | Deferred and must be split |
| Native/service credential lifecycle | Enables future non-browser clients | Enrollment authority, rotation/revocation administration and Phase-63-facing policy | Secret delivery, replay, revocation and recovery acceptance | High | Deferred |
| Audit export/redaction/retention | Completes a broader audit product | Privacy, storage, retention and operational policy | Large data, export integrity and retention safety | High | Deferred; not combined with read foundation |
| Compatibility retirement | Removes transitional behavior | Remaining Phase-62 contracts and migration tooling | Full compatibility and recovery proof | Blocked | Near final closeout |

## Gap matrix

| Security area | Current accepted state | Remaining gap after selection | Candidate later work |
|---|---|---|---|
| Actor/device model | Canonical persistent actor, device, session and credential context | Protected enrollment and administration | Bounded lifecycle-administration slices |
| Authentication | Legacy Basic, optional Managed Basic and browser sessions; strict cookie precedence; issuer binding, absolute expiry, idle expiry and terminal retention accepted | Native/service mechanisms and compatibility retirement | Native/service credential lifecycle or retirement slice |
| Browser sessions | Atomic issue/logout, independent secrets, persistence, absolute expiry, replay denial, outcomes, issuer binding, concurrency limit, idle expiry and bounded terminal cleanup accepted | Listing, logout-all and protected administration remain absent | Later separate administration design |
| Browser-session retention | One bounded startup pass deletes only old terminal verifiers and unreferenced canonical browser rows with exact accountability | No periodic scheduler or operator-facing administration | Closed for Slice 2W; scheduling/admin remain separate optional gaps |
| Browser-session idle expiry | `last_seen_at`, strict optional idle policy, shared cookie/CSRF effectiveness and 60-second write throttle accepted | None for request-time idle effectiveness | Preserve accepted contract |
| Concurrent browser sessions | Optional `0..64` effective-session limit with atomic deny-new semantics accepted | No automatic eviction | Preserve deny-new semantics unless explicitly selected later |
| Issuing credential lineage | Issuer revalidated on every browser request; terminal cleanup does not cascade solely from issuer state | No descendant lifecycle administration | Later explicit lifecycle-administration design |
| Grants and scopes | Exact actor grants and fixed scopes accepted | Protected grant administration | Later bounded administration design |
| Fixed roles | Exact-scope `role.admin` and `role.read-only` accepted for the explicit catalogue | Slice 2X will add only global audit-read membership to the fixed admin catalogue; generic roles remain absent | Later role-administration design |
| CSRF | Enforced for accepted browser mutations with memory-only frontend token | Future mutating owners require explicit contracts | Slice 2X is GET-only and does not require CSRF |
| Central authorization | All registered central business and administrative POST families classified | Sensitive security GET classification remains absent until Slice 2X implementation | Implement only the exact Slice-2X route |
| Browser lifecycle outcomes | Issue, revoke and cleanup outcomes accepted | Other operation outcomes and stronger coupling | Bounded later outcomes/coupling work |
| Accountability writes | Pre-dispatch, lifecycle outcomes, concurrency/idle denials and cleanup writes accepted and secret-free | Generic mutation completion evidence remains absent | Deferred outcomes/Outbox work |
| Accountability reads | No protected production read path; only test-owned unbounded `listAll()` exists | Selected Slice 2X adds one exact latest-event query with a fixed 100-row maximum and audit-of-audit | Export, pagination/filtering, configurable redaction and retention remain later |
| Revisions/idempotency | Domain-specific partial mechanisms only | Common preconditions, idempotency and durable operation lifecycle | Bounded operation-lifecycle slices |
| Administration | No general security-management API | Protected identity, credential, grant and role operations | One or more later bounded administration slices |
| Native/service clients | Core model is transport-neutral | Enrollment, rotation and revocation contracts | Bounded native/service credential slice |
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
5. delete its credential only when it is type `browser-session` and unreferenced;
6. preserve actor, device, issuer, grants, roles and accountability;
7. roll back the complete batch on any failure.

Accepted real-runtime proof:

- disabled policy no-op;
- full rollback and HTTP 503 fail-closed behavior after forced accountability failure;
- active and within-retention preservation;
- old revoked, absolute-expired and idle-expired deletion;
- no issuer-only cleanup;
- non-browser credential and re-referenced canonical-row preservation;
- exact secret-free cleanup events;
- 258 candidates with exactly 256 deterministic deletions;
- SQLite integrity, unchanged production database/configuration/loader, removed runtime override, active final daemon and zero VDR domain mutations.

## Selected Slice 2X boundary

### Exact route and bound

```text
GET /api/security/accountability/events
GET /api/security/accountability/events?limit=<1..100>
default limit 50
newest first by recorded_at DESC, event_id DESC
```

Malformed, duplicate, zero, over-limit and unknown query parameters fail before
repository access. No cursor, offset, filters or export are selected.

### Exact authorization

```text
security.audit.read@*
```

A direct exact grant or exact global `role.admin@*` may authorize the route.
Legacy Basic compatibility cannot bypass this sensitive-read policy. A
non-global admin role and `role.read-only` do not grant it.

### Atomic audit-of-audit

The existing pre-dispatch authorization event remains mandatory. The bounded
SELECT and one secret-free `operation.succeeded` event execute in one local
`BEGIN IMMEDIATE` transaction. Query, append or commit failure returns no rows.
This local read coupling does not claim generic mutation Outbox semantics.

### Data and ownership

- `AccountabilityEventRepository` owns the ordered bounded SELECT and additive index;
- a dedicated service owns the bound and read/outcome transaction;
- `SecurityHttpGate` owns protected GET classification and exact authorization;
- the HTTP owner uses a fixed response-field allowlist and `Cache-Control: no-store`;
- no frontend owner is added.

### Explicit Slice-2X exclusions

- export, streaming, pagination, filters and arbitrary history traversal;
- configurable redaction, audit retention, deletion or archival;
- frontend viewer;
- generic outcomes, Outbox, revisions or idempotency;
- security administration or native/service credential lifecycle;
- compatibility retirement;
- Android, Android TV or Phase 63–67 runtime.

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
10. Fresh post-Slice-2W gap analysis — completed.
11. Protected bounded accountability event read — Slice 2X selected, not implemented.
12. Broader outcomes and stronger coupling/Outbox — open.
13. Common revisions, idempotency and durable operation lifecycle — open.
14. Protected identity, credential, grant and generic-role administration — open.
15. Native/service credential lifecycle — open.
16. Audit export, configurable redaction and retention — open.
17. Compatibility retirement readiness and final Phase 62 closeout — open.

## Selection gate

Before production implementation, the Slice-2X contract, Current State, Current
Status, Handoff and this matrix must be mutually consistent and all five CI jobs
must pass on the final selection head:

- `docs-check`;
- `make-test-audit`;
- `frontend-regression-test`;
- `fast-regression-test`;
- `packaging-regression-test`.

## Exact next action

After the fully green selection CI, implement only Slice 2X as documented. Do not
start export, redaction, retention, administration, generic outcomes, Outbox,
revisions, idempotency, credential lifecycle, Android, Android TV or Phase 63–67
runtime.

Do not reopen Slice 2W without a changed relevant acceptance fingerprint. PR
#117 remains open, Draft and unmerged.