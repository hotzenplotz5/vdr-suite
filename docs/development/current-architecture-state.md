# VDR-Suite Current Architecture State

## Purpose

This document describes implemented architecture. Target contracts that are accepted but not implemented remain in ADRs and planning documents.

Baseline: `cb77ff66e11dca7db2eafa36525762dcde35102d` on 2026-07-27, plus the active Phase 62 Slice 1 and Slice 2 foundation changes described below.

## Ownership model

```text
VDR
  -> native devices, schedules, timers, recordings, replay, OSD and plugin execution

VDR-Suite
  -> backend identity and scope
  -> actor/device/credential/session/request security context
  -> persistent identity lifecycle state
  -> server-side policy and authorization
  -> domain services and persistent read models
  -> guarded operations and accountability
  -> client-facing REST and VdrSuiteClientApi contracts

Private adapters/providers
  -> RESTfulAPI, SVDRP, Streamdev, TVScraper, SuiteBridge
```

Frontend modules do not call private backend protocols or provider databases directly and do not own authentication, lifecycle or authorization decisions.

## Backend and snapshot foundation

```text
BackendNode
  -> BackendRegistry
  -> BackendRegistryService
  -> backend-scoped adapter/service calls
  -> backend-scoped snapshots and caches
  -> change detection and change feed
  -> Suite REST / Client API
```

Stable backend IDs, backend-scoped reads, capability reporting, server-enforced read-only access mode, snapshot/change sequencing and SSE/live-update foundations are implemented. Secure remote Agent identity, generation, lease and command fencing remain Phase 63 work.

## Phase 62 security boundary

```text
HttpServerRequest
  -> SecurityHttpGate
       -> LegacyBasicAuthenticator (transitional)
       -> RequestSecurityContext
       -> PersistentIdentityResolver
            -> SecurityIdentityRepository
       -> AuthorizationService
       -> AccountabilityEventRepository
  -> ApiRouter
  -> existing controller/service/domain safety checks
```

The first migrated mutation is `POST /api/vdr/remote/actions`. It requires `remote.control` scoped to the request backend and a durable pre-dispatch accountability decision.

Implemented identity values are actor, device, credential, session, authentication state, grants, request ID and correlation ID. Actor/device/session/credential metadata is stored in additive SQLite tables and resolved on each authenticated request. Persisted inactive, expired, revoked, missing or mismatched state fails closed before router dispatch.

The persistence foundation does not yet issue or verify production passwords, bearer tokens or cookie sessions. The current authenticator remains a transitional compatibility adapter.

`legacy-basic` preserves the existing local browser contract by default. `enforced` permits anonymous GETs and rejects every not-yet-migrated POST before router dispatch. In `enforced`, the embedded compatibility credential and grants are disabled unless explicitly configured.

## Independent safety decisions

The following remain separate and cumulative:

1. authentication: who or what presented credentials;
2. persistent identity lifecycle resolution: whether actor, device, credential and session remain valid;
3. actor authorization: permission and backend scope;
4. backend policy: backend exists, enabled and accepts writes;
5. capability policy: backend supports the action;
6. mutation safety: validation, revision, idempotency, operation and readback rules;
7. accountability: durable decision and outcome evidence.

No frontend state replaces these server-side decisions.

## Recordings 2 architecture

Recordings are lazy and backend-scoped. Recordings 2 remains the sole delivered Recording browser and owns folder state, cards, detail navigation, metadata, people, artwork, Genre integration and guarded rename/move/trash workflows. These mutation routes still require Phase 62 authorization migration.

## Metadata, Genre and EPG architecture

Phase 61 established backend-scoped target bindings, people relations, provider/derived evidence, canonical Genre identities and assignment states, indexed query-only browse paths and frontend integration. Provider acquisition is asynchronous. Normal Genre/global-search reads do not call provider or private backend protocols.

Suite-owned EPG identity remains backend-native and backend-scoped. Richer canonical ProgramEvent/provenance remains partial and is required before later TimerIntent orchestration.

## Global search architecture

```text
Frontend search dialog
  -> VdrSuiteClientApi.fetchClientGlobalSearch()
  -> GET /api/search
  -> GlobalSearchApiRuntime
  -> GlobalSearchController/Service/Repository
  -> query-only existing SQLite data
```

The first slice searches one selected backend across persisted Recording and EPG titles, subtitles and people with bounded windows, deterministic pagination and no provider resolution.

## Remote and live-overlay architecture

```text
Frontend remote module
  -> VdrSuiteClientApi
  -> SecurityHttpGate for POST mutation
       -> persistent identity lifecycle resolution
  -> LiveRemoteApiRuntime
  -> backend-neutral RemoteAction / LiveOverlay services
  -> BackendAccessPolicy and capability checks
  -> private backend adapter
```

PR #115 supplies the current 360×1220 PNG Remote and extended help/navigation/REC behaviour. Streaming and legacy OSD remain separate Phase 65/66 domains.

## SearchTimer and Timer architecture

SearchTimer domain values, discovery, preview cache, validation, RESTfulAPI command mapping, native capability handling and controlled mutation/readback paths exist. They do not constitute central multi-backend Timer orchestration and their POST routes are not yet migrated to the new actor authorization boundary.

Phase 64 introduces durable `TimerIntent`, `TimerAssignment`, `NativeTimerBinding`, scheduler and reconciler semantics only after Phase 62 and Phase 63 prerequisites.

## Current persistence boundaries

- SQLite remains the central Suite-owned metadata/read-model database.
- Domain repositories own SQL; controllers and frontend modules do not.
- `SecurityIdentityRepository` owns actor, device, credential and session lifecycle tables.
- Compatibility bootstrap uses `INSERT OR IGNORE` so restart cannot overwrite revocation state.
- The identity repository stores credential identifiers and lifecycle metadata, not submitted secrets.
- The Phase 62 accountability repository owns its SQLite schema and append operations.
- Database triggers reject accountability-row updates and deletes.
- Dedicated query-only connections serve documented read paths.
- Provider databases and filesystem paths are not public Suite identities.
- ADR-0050 remains the domain-repository SQLite boundary.

## Implemented safety boundaries

- backend scope on persisted/query paths;
- server-enforced read-only backend mode;
- guarded Recording validation/preview/execution/readback;
- allowlisted remote actions and operation IDs;
- centralized actor authorization for the first remote mutation;
- persisted actor/device/credential/session state resolved before authorization;
- fail-closed credential/session expiry and revocation;
- fail-closed rejection of unmigrated POST routes in enforced mode;
- append-only pre-dispatch allow/deny accountability;
- credential-safe security errors and identity persistence;
- no frontend-owned authorization;
- no provider lookup during documented query-only reads.

## Important incomplete architecture

| Area | Current state | Roadmap owner |
| --- | --- | --- |
| Actor/request security model | First runtime boundary implemented and real-runtime validated; only compatibility actor authenticated | Phase 62 |
| Persistent actor/device/session/credential lifecycle | Repository and request-time enforcement foundation implemented; secure issuance and protected management missing | Phase 62 |
| Roles, grants and backend scopes | Missing persisted assignment model | Phase 62 |
| Complete server authorization | Remote action migrated; remaining mutations and sensitive reads open | Phase 62 |
| Append-only accountability | First pre-dispatch repository implemented; full catalogue/outbox/query lifecycle open | Phase 62 |
| Universal revision/idempotency | Partial per-domain mechanisms only | Phase 62 |
| Secure Backend Agent lifecycle | Contract/foundation only | Phase 63 |
| TimerIntent orchestration | Missing | Phase 64 |
| Streaming Gateway | Missing | Phase 65 |
| Legacy OSD bridge | Missing | Phase 66 |
| Stable `/api/v1` and SDK compatibility | Partial/unreleased | Phase 67 |
| Recommendations / knowledge graph | Not implemented | Phase 68 |

## Related documents

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Architecture Index](../architecture/index.md)
- [Security and Identity Foundation](../architecture/security-identity-foundation.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Phase 62 Slice 1](phase-62-security-identity-foundation-slice-1.md)
- [Phase 62 Slice 2](phase-62-security-identity-foundation-slice-2.md)
- [Metadata-Backed Genre Browser](../architecture/metadata-genre-browser.md)
- [Backend-Scoped Global Search](../architecture/global-search.md)
- [Live Remote and OSD Contract](../architecture/live-remote-osd-contract.md)
- [Architecture Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Strict Roadmap](../planning/roadmap.md)
