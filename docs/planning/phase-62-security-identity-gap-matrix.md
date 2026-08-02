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
Slice 2X - Protected Mutation Response Outcomes

Selection state:
contract/documentation only; no Slice-2X production implementation

PR #117:
open, Draft, unmerged
```

A component is not accepted installed runtime until it is connected, covered by
the complete CI graph and validated on the real yaVDR system. Code-head evidence
alone is insufficient.

## Necessity rule

A remaining idea is not Phase-62 implementation work merely because it appears
useful or was previously listed on a roadmap. It must pass all four gates:

1. a binding Phase-62 requirement;
2. a concrete gap in the accepted code;
3. a real distinguishable failure or security consequence;
4. the smallest implementation that closes exactly that gap.

Work that fails this proof remains unselected. A later requirement may reopen it,
but no implementation is justified in advance.

## Fresh post-Slice-2W proof

Exactly one remaining item passes the necessity rule:

```text
Phase 62 Slice 2X
Protected Mutation Response Outcomes
```

Binding contract:

- [Slice 2X — Protected Mutation Response Outcomes](../development/phase-62-slice-2x-protected-mutation-response-outcomes.md)

### Requirement

The Phase-62 exit criteria require every privileged mutation to have actor,
decision and outcome evidence.

### Accepted code gap

`SecurityHttpGate::appendDecisionEvent()` records only
`dispatch_authorized`/`dispatch_denied` before dispatch. After an allowed POST,
`TestHttpServer::handleRequest()` calls `ApiRouter::handleClientPost()` and
returns the result without a business-mutation outcome event.

Slice 2S records outcomes only for browser-session issue/logout and explicitly
excludes ordinary business mutations.

### Concrete failure

An authorized successful mutation and an authorized mutation that returns a
router/backend/domain error currently leave the same pre-dispatch evidence. The
accountability store cannot distinguish their observed results.

### Minimal closing change

Reuse the existing gate context and append exactly one `operation.succeeded` or
`operation.failed` event after an already-protected mutation returns. No route,
permission, role, schema, repository, configuration variable, frontend or audit
read API is required.

## Candidate proof table

| Candidate | Binding requirement | Demonstrated current failure | Smallest justified result | Decision |
|---|---|---|---|---|
| Protected mutation response outcomes | Explicit exit criterion: actor, decision and outcome evidence | Allowed business POSTs record only pre-dispatch authorization; success and returned failure are indistinguishable | One post-router event using existing context and repository | **Selected as Slice 2X** |
| Protected audit HTTP read | No Phase-62 exit criterion requires a production audit reader | No security or runtime failure caused by absence of an HTTP read route was demonstrated | None | **Not necessary** |
| Audit export/filter/redaction/retention | No current Phase-62 consumer or acceptance requirement | No demonstrated failure | None | **Not necessary** |
| Generic security administration | Current accepted identities/grants/roles are provisioned without a production administration API | No required current operation is blocked | None until a concrete operator workflow is required | **Not necessary now** |
| Native/service credential lifecycle | Phase 62 requires representable Agent identity; the transport-neutral model already represents it | No Phase-62 native/service client currently needs enrollment or rotation | Defer to the phase/client that introduces the real consumer | **Not necessary now** |
| Common revisions/idempotency/operation framework | Exit criterion says only where required | No specific accepted mutable resource was proven unsafe by this analysis | Prove per resource before selecting any common mechanism | **Not proven** |
| Transactional Outbox/cross-system coupling | Would improve crash consistency but is not itself an exit criterion | No concrete current failure proves that a universal framework is the minimal fix | Do not build without a separately demonstrated crash/recovery requirement | **Not proven** |
| Compatibility retirement | Legacy compatibility is transitional | Final retirement readiness has not yet been evaluated after mandatory outcomes | Reassess at closeout | Later closeout decision |

## Gap matrix

| Security area | Current accepted state | Proven remaining requirement | Unproven or later work |
|---|---|---|---|
| Actor/device model | Canonical persistent actor, device, session and credential context | None demonstrated after Slice 2W | Production administration only after a real workflow is required |
| Authentication | Legacy Basic, optional Managed Basic and browser sessions; strict cookie precedence; issuer binding, absolute expiry, idle expiry and terminal retention accepted | Compatibility-retirement readiness at final closeout | Native/service lifecycle only with a concrete client requirement |
| Browser sessions | Atomic issue/logout, independent secrets, persistence, replay denial, outcomes, issuer binding, concurrency, idle expiry and bounded cleanup accepted | None demonstrated | Listing/logout-all/admin remain optional |
| Grants and fixed roles | Exact actor grants and fixed exact-scope Admin/Read-only roles accepted | None demonstrated | Generic role/grant administration remains optional |
| CSRF | Enforced for all accepted browser mutations with memory-only frontend token | Preserve existing behavior | No new CSRF feature selected |
| Central authorization | Every registered central POST is protected or explicitly Safe POST | None demonstrated | No further route-migration slice |
| Pre-dispatch accountability | Actor, authorization decision and dispatch allow/deny are append-only and fail closed | Preserve existing behavior | No audit read selected |
| Browser lifecycle outcomes | Issue/revoke and cleanup outcomes accepted | None | No broader lifecycle work selected |
| Business mutation outcomes | No post-router result event for already-protected business POSTs | **Slice 2X** | Cross-system crash atomicity remains unproven |
| Revisions/idempotency | Domain-specific partial mechanisms | None proven by this analysis | Must be justified per resource |
| Security administration | No general production management API | None proven | Select only for a concrete operator requirement |
| Native/service clients | Core model is transport-neutral and represents service/agent actors | None proven for Phase 62 | Phase 63/client-owned lifecycle when needed |
| Audit product | Append-only persistence and test/runtime inspection exist | None proven | HTTP read/export/redaction/retention not selected |
| Compatibility retirement | Legacy compatibility remains transitional | Final readiness decision after mandatory Slice 2X | Near closeout |

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

## Selected Slice 2X contract

For every authorized existing protected mutation that reaches
`ApiRouter::handleClientPost()`:

```text
HTTP 200..299  -> operation.succeeded / succeeded
all other HTTP -> operation.failed    / failed
reason_code    -> http_status_<decimal status>
```

The event reuses the existing actor, device, session, authentication,
permission, backend, action, operation, request and correlation context.

No body, header, cookie, credential or secret is persisted.

If the post-dispatch append fails, the original result is replaced by HTTP 503
`accountability_unavailable`. The slice does not claim rollback of an already
executed external/domain effect and does not claim replay safety.

Exact owners:

- `SecurityGateDecision`;
- `SecurityHttpGate`;
- `TestHttpServer`;
- unchanged `AccountabilityEventRepository`.

No new route, permission, role, schema, index, repository, service,
configuration, frontend or packaging component.

## Explicit Slice-2X exclusions

- protected audit reads or audit frontend;
- export, filters, pagination, redaction, deletion or retention;
- security administration;
- native/service credential lifecycle;
- transactional Outbox or generic cross-system commit atomicity;
- revisions, `If-Match`, idempotency keys or durable replay;
- compatibility retirement;
- Android, Android TV or Phase 63-67 runtime.

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
10. Necessity-based post-Slice-2W analysis — complete.
11. Protected Mutation Response Outcomes — Slice 2X selected, not implemented.
12. Compatibility-retirement readiness and final Phase-62 closeout — evaluate only after Slice 2X acceptance.

No other implementation item is currently proven necessary.

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

Complete the canonical documentation update and final selection CI. After all
five jobs are green, implement only Slice 2X as documented.

Do not implement an audit reader, administration API, Outbox, generic operation
framework, native/service lifecycle or other feature unless a separate binding
requirement, concrete code gap and failure case first prove it necessary.

Do not reopen Slice 2W without a changed relevant acceptance fingerprint. PR
#117 remains open, Draft and unmerged.
