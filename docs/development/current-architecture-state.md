# VDR-Suite Current Architecture State

## Purpose

This document describes implemented architecture. Target contracts that are accepted but not implemented remain in ADRs and planning documents.

Baseline: `cb77ff66e11dca7db2eafa36525762dcde35102d` on 2026-07-27, plus the active Phase 62 Slice 1 and Slice 2 branch changes described below.

## Ownership model

```text
VDR
  -> native devices, schedules, timers, recordings, replay, OSD and plugin execution

VDR-Suite
  -> backend identity and scope
  -> credential verification
  -> actor/device/credential/session/request security context
  -> persistent identity lifecycle state
  -> server-side policy and authorization
  -> domain services and persistent read models
  -> guarded operations and accountability
  -> client-facing REST and VdrSuiteClientApi contracts

Private adapters/providers
  -> RESTfulAPI, SVDRP, Streamdev, TVScraper, SuiteBridge
```

Frontend modules do not call private backend protocols or provider databases directly and do not own authentication, credential verification, lifecycle or authorization decisions.

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

## Phase 62 current runtime security boundary

```text
HttpServerRequest
  -> SecurityHttpGate
       -> LegacyBasicAuthenticator (transitional)
       -> optional ManagedBasicAuthenticator
            -> CredentialVerifierRepository
       -> RequestSecurityContext
       -> PersistentIdentityResolver
            -> SecurityIdentityRepository
       -> AuthorizationService
       -> AccountabilityEventRepository
  -> ApiRouter
  -> existing controller/service/domain safety checks
```

The first migrated mutation is `POST /api/vdr/remote/actions`. It requires `remote.control` scoped to the request backend and a durable pre-dispatch accountability decision.

Implemented runtime identity values are actor, device, credential, session, authentication state, grants, request ID and correlation ID. Actor/device/session/credential metadata is stored in additive SQLite tables and resolved on each authenticated request. Persisted inactive, expired, revoked, missing or mismatched state fails closed before router dispatch.

The existing local browser still uses the transitional legacy credential. One optional separately configured managed Basic identity can be provisioned with its own actor, device, session, credential, permissions and one-way password verifier. Strict Basic parsing and `crypt_r` verification support yescrypt and SHA-512 crypt hashes. No managed identity or permission is enabled by default.

`legacy-basic` preserves the existing local browser contract by default. Only the configured legacy actor and credential retain the temporary bypass for not-yet-migrated POST routes. A managed identity may use authenticated reads and explicitly migrated routes, but an unmigrated POST returns `security_policy_not_migrated` before router dispatch.

`enforced` permits anonymous GETs, rejects invalid/expired/revoked presented credentials, and rejects every not-yet-migrated POST before router dispatch. The embedded compatibility credential and grants are disabled unless explicitly configured.

## Staged browser-session verifier boundary

The branch contains this implemented and tested foundation, but `TestHttpServer` and `SecurityHttpGate` do not call it yet:

```text
future Cookie request
  -> BrowserSessionAuthenticator
       -> strict vdr_suite_session cookie parsing
       -> token-id lookup
       -> BrowserSessionCredentialRepository
            -> security_browser_session_credentials
       -> crypt_r verification of one-way session-secret hash
       -> independent X-CSRF-Token verification
  -> future PersistentIdentityResolver
  -> future SecurityHttpGate integration
```

The table binds a non-secret lookup token ID to actor, device, session, browser credential, and issuing credential identities. It stores separate one-way modular hashes for the session secret and CSRF secret, plus active, expiry, and revocation state. Complete cookie values, raw session secrets, raw CSRF values, passwords, and Authorization headers are not stored.

Strict cookie parsing rejects malformed values and duplicate target cookies. The verifier distinguishes anonymous, invalid, expired, and revoked states and requires an independent CSRF secret for future mutation authentication. Tests cover valid, wrong, unknown, malformed, duplicate, expired, revoked, and CSRF-negative cases.

This foundation does not yet issue a cookie, define a login/logout HTTP route, set cookie attributes, establish authentication precedence, pass cookie identities through the runtime resolver/Gate, or reject real mutation requests for missing/invalid CSRF. Existing installed request behavior is unchanged.

## Independent safety decisions

The following remain separate and cumulative:

1. credential verification: whether submitted authentication material is valid;
2. authentication: which actor/device/session/credential context it establishes;
3. persistent identity lifecycle resolution: whether actor, device, credential and session remain valid;
4. CSRF verification for browser-session mutations;
5. actor authorization: permission and backend scope;
6. backend policy: backend exists, is enabled and accepts writes;
7. capability policy: backend supports the action;
8. mutation safety: validation, revision, idempotency, operation and readback rules;
9. accountability: durable decision and outcome evidence.

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
Frontend or managed client
  -> VdrSuiteClientApi / Suite HTTP request
  -> SecurityHttpGate for POST mutation
       -> credential verification
       -> persistent identity lifecycle resolution
       -> remote.control@backend authorization
       -> pre-dispatch accountability
  -> LiveRemoteApiRuntime
  -> backend-neutral RemoteAction / LiveOverlay services
  -> BackendAccessPolicy and capability checks
  -> private backend adapter
```

PR #115 supplies the current 360×1220 PNG Remote and extended help/navigation/REC behaviour. Streaming and legacy OSD remain separate Phase 65/66 domains.

## SearchTimer and Timer architecture

SearchTimer domain values, discovery, preview cache, validation, RESTfulAPI command mapping, native capability handling and controlled mutation/readback paths exist. They do not constitute central multi-backend Timer orchestration and their POST routes are not yet migrated to the new actor authorization boundary.

The legacy browser can still reach these compatibility POST routes during migration. The managed identity cannot; it receives `security_policy_not_migrated`. The staged browser-session verifier changes no route behavior until Gate integration. Phase 64 introduces durable `TimerIntent`, `TimerAssignment`, `NativeTimerBinding`, scheduler and reconciler semantics only after Phase 62 and Phase 63 prerequisites.

## Current persistence boundaries

- SQLite remains the central Suite-owned metadata/read-model database.
- Domain repositories own SQL; controllers and frontend modules do not.
- `SecurityIdentityRepository` owns actor, device, credential and session lifecycle reads and lifecycle-state changes.
- `SecurityIdentityProvisioningRepository` owns idempotent identity creation and verifies persisted ownership metadata.
- Compatibility and managed provisioning use `INSERT OR IGNORE`, so restart cannot overwrite or reactivate existing lifecycle state.
- `CredentialVerifierRepository` owns login-to-credential bindings and one-way modular password hashes.
- `BrowserSessionCredentialRepository` owns browser-session token-ID bindings, one-way session/CSRF hashes, expiry, and revocation state.
- Submitted Authorization headers, decoded passwords, plaintext passwords, complete cookie values, raw session secrets, raw CSRF values, and reversible secrets are not persisted.
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
- centralized actor authorization for the first Remote mutation;
- separate optional managed identity with bounded Basic parsing and one-way password verification;
- persisted actor/device/credential/session state resolved before runtime authorization;
- fail-closed credential/session expiry and revocation;
- invalid presented credentials are not downgraded to anonymous in enforced mode;
- managed identities cannot inherit the legacy unmigrated-POST bypass;
- fail-closed rejection of unmigrated POST routes in enforced mode;
- append-only pre-dispatch allow/deny accountability;
- credential-safe errors, lifecycle persistence and verifier persistence;
- staged strict browser cookie parser with one-way session-secret verification;
- staged independent CSRF verification bound to an active unexpired browser-session record;
- architecture guards against raw browser credential storage;
- no frontend-owned authorization;
- no provider lookup during documented query-only reads.

## Important incomplete architecture

| Area | Current state | Roadmap owner |
| --- | --- | --- |
| Actor/request security model | First runtime boundary real-runtime validated; legacy actor plus one optional configured managed Basic actor are representable | Phase 62 |
| Credential verification | Managed Basic verifier real-runtime accepted; browser-session verifier foundation tested; protected issuance, password change/recovery and native/service mechanisms missing | Phase 62 |
| Browser session and CSRF | Persistent verifier, strict parser, expiry/revocation and independent CSRF check implemented; issuance, cookie attributes, HTTP/Gate wiring and actual mutation enforcement missing | Phase 62 |
| Persistent actor/device/session/credential lifecycle | Repository and request-time Basic enforcement real-runtime accepted; protected browser-session management and cleanup missing | Phase 62 |
| Roles, grants and backend/resource scopes | Missing persisted assignment model | Phase 62 |
| Complete server authorization | Remote action migrated; managed clients fail closed on remaining POSTs; browser-session path not wired; full route migration open | Phase 62 |
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
