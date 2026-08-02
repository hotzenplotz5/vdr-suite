# VDR-Suite Strict Roadmap

## Purpose

This file owns the strict forward execution order. Completed history belongs in
[Completed Phases](../development/completed-phases.md); compact numbering belongs
in the [Phase Map](phase-map.md); detailed prerequisites belong in the
[Implementation Dependency Map](implementation-dependency-map.md).

> Work is read from top to bottom. Later phases may not bypass identity,
> authorization, accountability, lifecycle fencing or stable-domain
> prerequisites by moving policy into a frontend, plugin or provider.
>
> A roadmap item is not automatically an implementation requirement. New runtime
> work requires a binding phase requirement, a concrete accepted-code gap, a real
> failure/security consequence and the smallest closing change.

## Current verified position

Baseline: `main @ cb77ff66e11dca7db2eafa36525762dcde35102d`.

```text
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

Current runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Phase 62 runtime accepted through:
Slice 2W - Browser-Session Terminal Retention Cleanup

Current bounded slice:
Slice 2X - Protected Mutation Response Outcomes

Slice 2X state:
production implementation complete;
focused tests and architecture guard complete;
isolated install/runtime harness complete;
real yaVDR acceptance pending.
```

## Completed prerequisites

### Phase 60.15 — Recording metadata and artwork preparation

Status: **Completed.**

Separated native, normalized Suite and provider-derived Recording fields;
established provider-neutral artwork references; preserved lazy loading and
no-provider fallback.

### Phase 61 — Suite Metadata and Genre Platform

Status: **Completed.**

Delivered persistent backend-scoped Recording/EPG bindings, people relations,
provider/derived evidence, canonical Genre assignments, explicit assignment
states, indexed browse paths, EPG hierarchy and frontend integration.

Evidence:

- [Phase 61 and Performance Closeout](../development/phase-61-metadata-genre-performance-closeout.md)
- PR #100 and its focused, regression, architecture, build/install and real
  system acceptance.

### Post-Phase-61 Performance Hardening (B1-B4)

Status: **Completed, non-numbered.**

PRs #102 through #108 completed candidate fast paths, architecture correction,
atomic evidence writes, no-op synchronization, indexed windows, unchanged-event
suppression and ETYPES-cycle throttling.

### Post-Phase-61 platform runtime features

Status: **Completed, non-numbered.**

- VDR Remote and Live Overlay hardening (#110)
- Backend-scoped Global Search (#111)
- Configurable photorealistic VDR Remote (#115)
- [Post-Phase-61 Platform Runtime Closeout](../development/post-phase-61-platform-runtime-closeout.md)

# Strict execution order

## Phase 62 — Identity, RBAC and Accountability Foundation

Status: **Active; incomplete.**

Goal: production-grade actor identity, scoped server-side authorization and
append-only accountability.

### Accepted runtime through Slice 2W

The accepted runtime includes:

- canonical actor, device, session, credential, request and correlation context;
- persistent lifecycle resolution;
- Legacy Basic compatibility, optional Managed Basic and browser sessions;
- strict cookie precedence and cookie-bound CSRF;
- exact grants and fixed exact-scope Admin/Read-only roles;
- classification of every central POST as protected or explicitly Safe POST;
- protected Remote, Timer, Channel Move, Recording, SearchTimer, Native Fuzzy and
  query-scoped refresh mutations;
- append-only pre-dispatch accountability;
- browser issue/revoke outcome accountability;
- issuing-credential lifecycle binding;
- absolute lifetime, optional concurrency limit and optional idle expiry;
- bounded terminal browser-session retention cleanup with atomic secret-free
  accountability;
- focused, architecture, complete CI and guarded real-yaVDR proof through Slice
  2W.

Evidence:

- [Phase 62 Gap Matrix](phase-62-security-identity-gap-matrix.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)
- [Slice 2W Runtime Closeout](../development/phase-62-slice-2w-runtime-closeout.md)

### Implemented Slice 2X — runtime acceptance pending

The exit criteria require actor, decision and outcome evidence for every
privileged mutation. Slice 2X closes the prior protected-business-mutation gap:

```text
HTTP 200..299  -> operation.succeeded / succeeded
all other HTTP -> operation.failed    / failed
reason_code    -> http_status_<decimal status>
```

Implemented boundary:

- `SecurityGateDecision` retains the successful authorization context;
- `SecurityHttpGate` appends the post-router outcome;
- `TestHttpServer` invokes it after POST dispatch and before final response;
- the append-only repository remains unchanged;
- no route, permission, role, schema, configuration, frontend or packaging
  component is added;
- post-dispatch append failure returns 503 `accountability_unavailable` without a
  domain rollback or replay-safety claim.

Validation boundary:

- focused success/failure/context/failure-path tests;
- architecture order/scope/secret-source guard;
- isolated inner event-pair runner;
- guarded entrypoint for backup, candidate installation, dual-database systemd
  override, rollback and final production-service restoration;
- dedicated [Slice 2X yaVDR Runbook](../development/phase-62-slice-2x-runtime-acceptance-runbook.md).

Contract:

- [Slice 2X — Protected Mutation Response Outcomes](../development/phase-62-slice-2x-protected-mutation-response-outcomes.md)

The earlier implementation/harness head
`4b61583b604626cd49e213356241759c81e60d04` passed VDR-Suite CI #6871, Run ID
`30750871845`. Because the isolated runtime-entry fingerprint was added later,
the final current head requires a fresh complete five-job CI before installation.

### Remaining proven required order

1. **Final source gate for Slice 2X.**
   All five CI jobs must be green on the exact current head containing the
   isolated runtime entrypoint.
2. **Bounded real yaVDR Slice-2X acceptance.**
   Prove one protected HTTP 200 outcome pair and one deterministic protected HTTP
   500 outcome pair against an isolated scenario database, with rollback and
   production restoration.
3. **Slice-2X runtime closeout.**
   Record accepted head, CI, daemon/loader/configuration/report hashes, evidence
   directory and final service state.
4. **Compatibility-retirement readiness and final Phase-62 closeout.**
   Evaluate only after Slice 2X is fully accepted. This is a closeout decision,
   not advance authorization for another feature slice.

No other implementation item is currently proven necessary.

### Work requiring a separate necessity proof

Do not implement merely because useful:

- protected audit HTTP reads, frontend, export, filtering, pagination, redaction
  or retention;
- generic actor, identity, credential, grant or role administration;
- native/service credential lifecycle before a real client requires it;
- universal revision, `If-Match`, idempotency or durable operation framework;
- transactional Outbox or generic cross-system commit coupling.

### Phase 62 exit criteria

- different actors can hold different rights on the same backend;
- denial is server-side for every protected route;
- browser sessions are securely issued, expired, revoked and CSRF-protected;
- the second-house/read-only scenario remains proven;
- every privileged mutation has actor, decision and outcome evidence;
- required pre-dispatch accountability failure prevents dispatch;
- revision/idempotency mechanisms exist only where a concrete resource contract
  proves them necessary;
- Agent identities can be represented for Phase 63;
- compatibility-retirement readiness is explicitly decided.

### Forbidden shortcuts

- no frontend-owned role or CSRF decision;
- no ordinary logs as the accountability database;
- no compatibility mode claimed as final authentication;
- no plaintext/reversible credential persistence;
- no complete cookie, raw session or raw CSRF persistence;
- no managed identity inheriting legacy unmigrated-POST bypass;
- no browser mutation without server-side CSRF;
- no privileged dispatch before authorization/accountability;
- no feature without a requirement-to-code failure chain;
- no Phase 63-67 runtime declared through Phase 62 preparation.

## Phase 63 — Backend Agent and Secure Multi-Site Runtime

Status: **Planned after Phase 62.**

Scope includes Agent enrollment/device identity, protected transport, generation,
heartbeat/lease, capability/snapshot publication, durable command/result flow,
fenced read-only operation and private provider selection.

## Phase 64 — Timer Intent and Multi-Backend Orchestration

Status: **Planned after Phase 63.**

Scope includes durable timer intents/assignments/native bindings, deterministic
scheduling/reconciliation, eligibility, duplication/ambiguity policy,
readback/drift and uncertain-dispatch recovery.

## Phase 65 — Streaming Gateway and Media Sessions

Status: **Planned after Phase 64.**

Scope includes authorized short-lived media sessions, Gateway-owned connections,
capacity leases, Live/Recording pass-through, range/seek/reconnect and private
provider routing.

## Phase 66 — Legacy OSD Compatibility Bridge

Status: **Planned after Phase 65.**

Scope includes immutable OSD snapshots/deltas, resynchronization, one fenced
controller lease, allowlisted input and no arbitrary command tunnel.

## Phase 67 — Public API and Client Compatibility Hardening

Status: **Planned after Phase 66.**

Scope includes request/correlation IDs, Problem Details-compatible errors,
versioned discovery, resource-specific ETag/`If-Match`, durable operations where
required, pagination, migration aliases and compatibility tests.

## Phase 68 — Recommendation and Content Knowledge Graph

Status: **Later vision.**

Requires stable metadata/provenance, actor privacy, stable identities, mature
accountability and public API contracts.

# Cross-cutting completion gates

- **Identity gate:** stable Suite identity and explicit native binding.
- **Provider gate:** provider facts carry provenance and never become hidden
  authority.
- **Mutation gate:** authentication, CSRF where applicable, authorization,
  required preconditions, durable dispatch evidence, verification and
  accountability.
- **Native boundary gate:** no raw VDR pointer/lock crosses async, network or DB
  work.
- **Client gate:** clients consume Suite contracts, never private provider
  details.
- **Acceptance gate:** focused tests, regressions, build/package validation and
  real-system proof where runtime behavior changes.

## Related documents

- [Current State](../CURRENT.md)
- [Phase Map](phase-map.md)
- [Implementation Dependency Map](implementation-dependency-map.md)
- [Phase 62 Gap Matrix](phase-62-security-identity-gap-matrix.md)
- [Architecture Gap Matrix](architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](parity-audit-and-frontend-gap-roadmap.md)
