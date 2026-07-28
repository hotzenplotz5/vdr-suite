# VDR-Suite Current State

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Phase 62 Gap Matrix](planning/phase-62-security-identity-gap-matrix.md)
- [Current Architecture State](development/current-architecture-state.md)
- [Phase 62 Slice 1](development/phase-62-security-identity-foundation-slice-1.md)
- [Phase 62 Slice 2](development/phase-62-security-identity-foundation-slice-2.md)
- [Phase 61 and Performance Closeout](development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](development/post-phase-61-platform-runtime-closeout.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Completed Phases](development/completed-phases.md)
- [ADR Index](adr/index.md)

## Repository baseline

This document was reconciled on 2026-07-27 against:

```text
origin/main
cb77ff66e11dca7db2eafa36525762dcde35102d
Merge pull request #115 from hotzenplotz5/agent/configurable-remote-mapping
```

The SHA is a time-bound evidence point. Every new task must fetch `origin/main`, determine the current head and inspect the local worktree before changing files.

## Current verified position

```text
Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Completed post-phase platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)
Configurable photorealistic VDR Remote (#115)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next strict runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Current Phase 62 state:
Active; Slice 1 is real-runtime validated. Slice 2 has real-runtime-accepted lifecycle persistence plus an implemented first managed credential verifier on Draft PR #117.
```

Phase 61 remains completed. Phase 62 is not complete. The active work does not advance Phase 63-67 runtime.

## Implemented runtime truth on `main`

Current `main` contains:

- daemon-owned SQLite persistence, migrations and domain repository boundaries;
- BackendNode/BackendRegistry, backend-scoped snapshots, change feed, SSE foundations and server-enforced read-only backend policy;
- channels, current-programme and channel-day views, persistent EPG cache and the EPG timeline;
- Recordings 2 as the sole delivered recording browser, including folders, cards, detail view, metadata, people, artwork, Genre integration and guarded rename/move/trash actions;
- SearchTimer list, discovery, preview, validation, native capability handling and controlled mutation foundations;
- persistent backend-scoped Recording and EPG metadata identities, people relations, Genre evidence, assignment states and query-only browse paths;
- the accepted EPG hierarchy Film, Serie, Dokumentation and Sport, plus result-backed Film subgenres;
- asynchronous provider acquisition with provider-failure isolation and no provider resolution from normal Genre or search GET requests;
- backend-neutral remote actions and live-overlay snapshots through Suite-owned API and Client API boundaries;
- backend-scoped global search over persisted Recording and EPG titles, subtitles and people;
- the merged 360×1220 transparent PNG remote, 35 existing hotspots, overview/EPG/help integration and guarded REC start/stop workflow from PR #115;
- packaging, install staging, daemon builds and real-system acceptance workflows.

## Active Phase 62 branch truth

The active Phase 62 branch owns this request path:

```text
HttpServerRequest
  -> SecurityHttpGate
       -> transitional LegacyBasicAuthenticator
       -> optional ManagedBasicAuthenticator
            -> CredentialVerifierRepository
       -> RequestSecurityContext
       -> PersistentIdentityResolver
            -> SecurityIdentityRepository
       -> AuthorizationService
       -> append-only AccountabilityEventRepository
  -> ApiRouter
  -> existing controller/service/domain safety checks
```

### Slice 1 — implemented and real-runtime validated

- canonical actor, device, session and request security context values;
- explicit anonymous, authenticated, invalid, expired and revoked states;
- backend-scoped permission grants and centralized authorization decisions;
- stable security error codes without credential reflection;
- pre-dispatch authorization for `POST /api/vdr/remote/actions` using `remote.control@<backend>`;
- append-only SQLite accountability rows for allow and deny decisions;
- request and correlation ID propagation;
- explicit `legacy-basic` and fail-closed `enforced` rollout modes;
- real yaVDR evidence for anonymous denial, invalid-credential denial and authenticated Browser Remote actions.

### Slice 2 — lifecycle foundation and first managed verifier implemented

- additive actor, device, session, credential and Basic-verifier tables;
- compatibility lifecycle bootstrap without storing the Basic Authorization value;
- persisted actor/device/session/credential ownership bindings;
- request-time lifecycle resolution before authorization;
- persisted expiry and revocation enforcement;
- restart-safe bootstrap that does not overwrite revoked records;
- optional separate managed actor/device/session/credential provisioning;
- strict Basic parsing and `crypt_r` verification against yescrypt or SHA-512 crypt hashes;
- no default managed identity and no default managed permission;
- partial/unsupported managed configuration fails security-runtime startup;
- managed identities cannot use the legacy bypass for unmigrated POST routes;
- invalid credentials in `enforced` mode are rejected rather than treated as anonymous;
- repository, provisioning, verifier, resolver, HTTP-gate and architecture tests.

The existing legacy browser credential remains transitional and broadly privileged only for compatibility. The optional managed verifier stores a unique login binding and one-way modular password hash, never the submitted header, decoded password, plaintext password or reversible secret.

## Open Phase 62 work

Phase 62 still requires:

- protected per-user/service credential issuance, server-side hash generation/password change and recovery;
- browser cookie sessions, secure cookie policy, CSRF, refresh, logout, expiry cleanup and protected lifecycle management;
- native/service token enrollment and rotation;
- persisted roles, permissions, grants and backend/resource scopes;
- complete permission mapping and server-side authorization for all mutations and sensitive reads;
- universal revision and `If-Match` rules where resource state is mutable;
- durable idempotency-key replay semantics and operation records;
- mutation completion/outcome evidence and transactional outbox delivery;
- full authentication, authorization, mutation and security event catalogue;
- protected audit reads, redaction, retention and audit-of-audit;
- failure-injection and real-runtime acceptance across all migrated routes.

## Compatibility boundary

The local browser remains compatible by default through `legacy-basic`. That actor alone retains the temporary compatibility bypass for not-yet-migrated POST routes.

An optional managed Basic identity can coexist for isolated identity, permission and revocation tests. It may use authenticated reads and explicitly migrated routes, but receives `security_policy_not_migrated` for an unmigrated POST.

The compatibility mode is not a permanent architecture exemption. Frontend state and disabled buttons are never authorization evidence. The server remains the enforcement owner.

## Pull request classification at this baseline

| PR | Repository truth |
| ---: | --- |
| #112 | Open Draft from an old base; competing pure-SVG asset proposal and not current runtime truth. |
| #113 | Closed unmerged; explicitly superseded by merged PR #115. |
| #114 | Merged documentation truth refresh. |
| #115 | Merged configurable photorealistic PNG Remote and extended Remote functions; current `main` head. |
| #116 | Open Draft, mergeable; Android/client API feasibility and proposed ADR-0051. ADR-0051 is not accepted runtime truth on `main`. |
| #117 | Open Draft, mergeable Phase 62 implementation branch; Slice 1 validated and Slice 2 lifecycle plus first managed verifier active. Must not be auto-merged. |

Open PR content remains lower-trust than merged code and accepted ADRs.

## Accepted target contracts versus implementation

Accepted ADRs through ADR-0050 are on `main`. ADR-0013, ADR-0041, ADR-0042, ADR-0048 and ADR-0049 define the relevant Phase 62 direction. Their acceptance does not mean their complete runtime exists.

ADR-0051 is proposed only in Draft PR #116 at this baseline. It may consume Phase 62 contracts later but does not change Phase 62 scope and does not authorize Android implementation in this phase.

## Later phase boundaries

- Phase 63 owns production remote-site and Backend Agent runtime.
- Phase 64 owns complete TimerIntent and multi-backend orchestration.
- Phase 65 owns Streaming Gateway and media sessions.
- Phase 66 owns legacy OSD compatibility runtime.
- Phase 67 owns stable public `/api/v1`, SDK and compatibility release.

The current unversioned `/api/...` routes are compatibility routes, not a stable public API.

## Boundary rules

- VDR remains native runtime authority.
- VDR-Suite owns external domain, policy, orchestration, persistent read models and client contracts.
- Browsers do not call RESTfulAPI, SVDRP, Streamdev, TVScraper or SuiteBridge directly.
- Authentication, credential verification, persistent identity resolution and authorization are separate decisions.
- Backend read-only and capability checks remain independent of actor permissions.
- Frontends do not own authorization decisions.
- Submitted credentials do not belong in URLs, logs, error bodies, request IDs, identity lifecycle rows or accountability payloads; only a one-way verifier hash may be persisted in its dedicated repository.
- Completed phases are not silently reopened by optional extensions.
