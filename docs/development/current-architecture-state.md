# VDR-Suite Current Architecture State

## Purpose

This document describes implemented architecture. Accepted target contracts that are not connected to runtime remain in ADRs and planning documents.

Baseline: `cb77ff66e11dca7db2eafa36525762dcde35102d` plus completed Phase 62 on PR #117.

## Ownership model

```text
VDR
  -> native devices, schedules, timers, recordings, replay, OSD and plugins

VDR-Suite
  -> backend identity and scope
  -> credential verification and browser-session issuance
  -> actor/device/credential/session/request context
  -> persistent identity lifecycle
  -> server-side authorization and CSRF policy
  -> guarded operations and append-only accountability
  -> client-facing REST and Client API contracts

Private adapters/providers
  -> RESTfulAPI, SVDRP, Streamdev, TVScraper, SuiteBridge
```

Frontend modules do not call private backend protocols directly and do not own authentication, lifecycle, CSRF or authorization decisions.

## Completed Phase 62 security boundary

```text
HttpServerRequest
  -> BrowserSessionHttpGate for exact issue/logout routes
  -> SecurityHttpGate for ordinary requests and protected mutations
       -> strict presented-browser-cookie precedence
       -> otherwise Legacy Basic compatibility or optional Managed Basic
       -> persistent actor/device/session/credential lifecycle resolution
       -> browser-session absolute/idle/issuer/concurrency effectiveness
       -> exact route and backend-scope extraction
       -> cookie-bound CSRF for browser mutations
       -> exact actor grant and fixed-role authorization
       -> append-only pre-dispatch accountability
  -> ApiRouter
  -> protected operation implementation
  -> append-only operation.succeeded or operation.failed outcome
  -> response
```

Every registered central POST is either a protected mutation or an explicitly classified Safe POST. Unknown browser and enforced-mode mutation paths fail closed.

## Implemented identity and authentication

- canonical persistent actor, device, session and credential identities;
- explicit authentication state, request ID and optional correlation ID;
- Legacy Basic compatibility using a separate transitional identity;
- optional Managed Basic with one-way modular password verifier;
- browser-session credentials with independent session and CSRF secrets;
- strict cookie parsing and no fallback from a presented invalid browser credential;
- persistent lifecycle checks for actor, device, session, credential and issuing credential;
- service and agent actor types representable for Phase 63 without implementing Agent runtime.

No Authorization header, plaintext password, complete cookie, raw session secret or raw CSRF token is persisted.

## Browser-session lifecycle

The runtime supports:

- atomic Basic-to-browser-session issuance;
- hardened `Secure`, `HttpOnly`, `SameSite=Strict` cookie;
- independent one-time CSRF token delivered through no-store JSON;
- atomic logout revoking verifier, canonical session and browser credential;
- immutable absolute lifetime;
- optional per-actor active-session limit with deny-new semantics;
- optional idle expiry with throttled `last_seen_at` persistence;
- request-time issuing-credential lifecycle binding;
- bounded startup cleanup of terminal browser-session lifecycles.

Browser lifecycle and retention accountability is append-only and secret-free.

## Authorization model

`PermissionGrant` combines an exact permission with an exact backend scope or wildcard where explicitly allowed. Fixed roles expand only to their defined exact permissions; Read-only precedence prevents mutation rights.

Protected central mutation families include Remote, Timer, Channel Move, Recording, SearchTimer, Native Fuzzy and query-scoped cache refresh operations. Backend read-only, capability and domain validation remain independent cumulative safety decisions after actor authorization.

## Accountability model

Pre-dispatch events record actor, device, session, authentication state, permission, backend scope, action, operation ID, request ID, correlation ID, decision and reason. Required append failure prevents dispatch.

For each already-protected authorized mutation reaching the router:

```text
HTTP 200..299  -> operation.succeeded / succeeded
all other HTTP -> operation.failed    / failed
reason_code    -> http_status_<decimal status>
```

The post-router event reuses the successful authorization context. Outcome append failure changes the response to `503 accountability_unavailable`; it does not claim domain rollback, transactional Outbox coupling or safe automatic replay.

## Persistence boundaries

- Suite-owned SQLite remains the central metadata and security store.
- Domain repositories own SQL; controllers and frontend modules do not.
- security identity, verifier, grant, browser-session, retention and accountability repositories own their exact tables and transactions;
- database triggers reject accountability update/delete;
- browser issue/logout and retention cleanup use bounded explicit transactions;
- runtime acceptance uses an isolated database copy and verifies production logical state remains unchanged.

## Final runtime evidence

```text
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS
accepted_runtime_head=4762583d5b5170866838ed9f03b928adbf39f99e
daemon_sha256=488edade196cedfb92d5393a8725b39c5f5cdfd3265e2b15bab6aadfbe7ef5f5
loader_sha256=3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
configuration_sha256=8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b
runtime_report_sha256=bf165416b5ad041f44b2514182dac582a7f1060bf1ae8cc584964f3fc5a98bdf
evidence_directory=/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51
```

## Compatibility boundary

Legacy Basic compatibility remains explicitly transitional. Immediate removal is not deployment-ready because `legacy-basic` remains the code default and packaged deployments do not yet mandate migration to `enforced`.

This is the final Phase-62 retirement decision. A future removal requires a deployment-migration contract covering rollout, recovery and compatibility impact.

## Later architecture owners

| Area | Current state | Later owner |
|---|---|---|
| Backend Agent | actor representation only; no enrollment, transport, lease or command runtime | Phase 63 |
| TimerIntent orchestration | not implemented | Phase 64 |
| Streaming Gateway | not implemented | Phase 65 |
| Legacy OSD bridge | not implemented | Phase 66 |
| Stable public API/SDK hardening | partial internal contracts only | Phase 67 |
| Audit product | append-only persistence exists; HTTP read/export product deferred | separate necessity proof |
| Generic security administration | not required for Phase 62 | concrete operator workflow |
| Universal revisions/idempotency/Outbox | not accepted as generic infrastructure | resource-specific proof |

## Related documents

- [Current State](../CURRENT.md)
- [Phase 62 Final Closeout](phase-62-closeout.md)
- [Slice 2X Runtime Closeout](phase-62-slice-2x-runtime-closeout.md)
- [Security and Identity Foundation](../architecture/security-identity-foundation.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Strict Roadmap](../planning/roadmap.md)
