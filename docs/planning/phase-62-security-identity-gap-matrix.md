# Phase 62 Security and Identity Gap Matrix

Status: active Phase 62 planning and implementation matrix  
Repository baseline: `cb77ff66e11dca7db2eafa36525762dcde35102d` (`main`, merge of PR #115)  
First implementation slice: Phase 62 Slice 1

This matrix separates repository runtime truth from accepted architecture contracts.
A document or ADR entry is not counted as implemented runtime.

## Start-state findings

- `TestHttpServer` authenticated every request by comparing one complete
  `Authorization` header value. The default value was embedded in the runtime.
- The result of that comparison was not represented as an actor, device,
  session, principal, permission, scope, or authorization decision.
- `BackendAccessPolicy` already enforced backend existence, enabled state, and
  read-only state. That is a backend safety policy, not actor authorization.
- Mutating services already contain useful safety primitives, including
  backend scope, capability checks, operation IDs in several workflows,
  selected duplicate prevention, and selected readback verification.
- There was no durable append-only implementation of ADR-0049's
  `AccountabilityEvent`.
- The unversioned `/api/...` routes remain compatibility routes. Phase 62 does
  not publish `/api/v1` and does not claim Phase 67.
- ADR-0051 is not part of this baseline. It remains proposed in Draft PR #116
  and is treated only as client-consumer context.

## Gap matrix

| Security area | Current runtime state | Current owner/component | Affected routes and mutations | Accepted ADR direction | Concrete gap | Risk | Planned Phase 62 slice | Test evidence | Documentation evidence | Dependencies | Later phase not pulled forward |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Principal / actor model | Phase 62 Slice 1 adds canonical `ActorIdentity`, `ActorType`, and `RequestSecurityContext` | `core/security/include/SecurityIdentity.h` | All HTTP requests; first enforced mutation is remote control | ADR-0013, ADR-0041, ADR-0049 | Only the legacy local actor is produced by runtime authentication; persistent users, agents, and services are not yet issued | Actions cannot yet be attributed to independently managed identities | Slice 1 foundation; Slice 2 persistence and issuance | `test-security-authorization`, `test-security-http-gate` | Phase 62 Slice 1 development and architecture documents | Existing HTTP request headers | Phase 63 agent deployment |
| User identity | Representable as `ActorType::User`; one explicit compatibility actor is emitted | `LegacyBasicAuthenticator` | Local browser and clients presenting legacy Basic credential | ADR-0013, ADR-0041 | No user repository, password lifecycle, enrollment, recovery, disable workflow, or per-user credential | Shared credentials prevent individual accountability | Slice 2 | Planned repository, lifecycle, and revocation tests | This matrix | Principal model | No Android application |
| Device identity | Optional `DeviceIdentity` is carried and audited; compatibility device ID is configurable | `SecurityIdentity.h`, `LegacyBasicAuthenticator` | Protected remote mutation | ADR-0041, ADR-0049 | No device registration, trust level, key material, rotation, or revocation repository | A stolen shared credential cannot be isolated to one device | Slice 2 | Current revoked-device authorization unit test; persistence tests pending | Architecture document | User/session persistence | Phase 63 agent trust runtime |
| Session identity | Optional `SessionIdentity` with active, expired, and revoked states | `SecurityIdentity.h`, `AuthorizationService` | Protected remote mutation | ADR-0041 | Legacy Basic is stateless compatibility; no session store, expiry clock, refresh, logout, or revocation persistence | Credential remains valid until external configuration changes | Slice 2 | Expired and revoked session negative tests exist at policy boundary | Architecture document | User/device persistence | Phase 67 public client contract |
| Authentication mechanism | Explicit `LegacyBasicAuthenticator`; legacy compatibility and enforced rollout modes | `SecurityConfiguration.h`, `LegacyBasicAuthenticator.h`, `SecurityHttpGate.h` | All HTTP requests | ADR-0041, ADR-0048 | This is not production authentication; no cookie session, CSRF protection, native token issuance, rotation, or MFA | Shared Basic credential is unsuitable for remote/public exposure | Slice 1 adapter, Slice 2 production mechanism | Missing, invalid, and authenticated request tests | Phase 62 Slice 1 document | HTTP headers, runtime configuration | Phase 67 stable public authentication contract |
| Session lifecycle | Policy can deny expired/revoked states | `AuthorizationService` | Protected mutations | ADR-0041 | No creation, refresh, expiry scheduler, logout, revocation store, or cleanup | Revocation cannot yet be executed through runtime | Slice 2 | Unit negatives present; lifecycle integration pending | Matrix | Session repository | None |
| Revocation | Actor/device/session inactive states are fail-closed | `AuthorizationService` | Protected mutations | ADR-0041, ADR-0049 | No persisted revocation state or administrative command | Compromised identity cannot yet be centrally revoked | Slice 2 and Slice 3 administration policy | Revoked actor/device/session unit tests | Matrix | Identity persistence and admin permissions | No Phase 63 operations |
| Roles | Not implemented | No runtime owner | All mutations and protected reads | ADR-0013 | No role definitions or actor-role assignment | Permission management does not scale | Slice 3 | Pending role-assignment tests | Matrix | User repository | None |
| Permissions | Phase 62 Slice 1 adds central exact or wildcard permission matching; `remote.control` is first permission | `AuthorizationService` | `POST /api/vdr/remote/actions` | ADR-0013, ADR-0041 | Other mutation permissions are not migrated | Shared compatibility actor still has wildcard rights by default | Slice 1 first route; Slice 3 complete mutation catalogue | Allowed and missing-permission tests | Architecture document | Route migration inventory | Phase 64 timer orchestration |
| Backend scopes | Phase 62 Slice 1 supports exact backend or wildcard grants | `PermissionGrant`, `AuthorizationService` | Protected remote mutation uses request `backendId` | ADR-0013, ADR-0041, ADR-0042 | No persisted scope assignments; other routes still rely only on backend safety policy | Actor can otherwise affect unintended sites | Slice 1 first route; Slice 3 all mutations | Wrong-backend negative test | Architecture document | Backend IDs and request parsing | Phase 63 remote-site runtime |
| Central server authorization | Phase 62 Slice 1 evaluates before router dispatch | `SecurityHttpGate` | First route: remote action; enforced mode rejects unmigrated POST routes | ADR-0013, ADR-0041, ADR-0042 | Remaining mutation routes need explicit permission mappings and domain-level context propagation | Route added without policy could become writable | Slice 1 fail-closed gate; Slice 3 migration | Anonymous, invalid, forbidden, wrong-scope, allowed, and unmigrated-route tests | Development document | Mutation inventory | No shadow API |
| Backend read-only enforcement | Already implemented independently of actor permissions | `BackendAccessPolicy`, mutation services | Remote, timer, recording, channel, SearchTimer workflows | ADR-0013, ADR-0041 | Must remain a separate hard boundary after actor authorization | Actor permission could otherwise bypass read-only site policy | Preserved in every slice | Existing backend policy tests plus full suite | Existing architecture docs and Phase 62 docs | Backend registry | Phase 63 site operation |
| Mutation preconditions | Selected workflows have validation and safety gates | Recording, timer, channel, SearchTimer services | Recording execute; timer actions; channel move; SearchTimer mutations | ADR-0042 | No universal envelope or common order across all routes | Inconsistent conflict and retry behavior | Slice 4 | Pending common contract tests | Matrix | Route migration and operation model | Phase 64 timer orchestration |
| Revisions | Selected domain-specific revisions/generations exist; no universal HTTP contract | Existing mutation services and backend generation models | Timer, recording, channel, SearchTimer mutations | ADR-0042, ADR-0048 | `If-Match` / expected revision is not consistently accepted, checked, and returned | Lost updates and stale writes | Slice 4 | Pending missing/wrong/stale revision tests | Matrix | Shared mutation envelope | Phase 67 stable public API |
| Idempotency keys | Selected operation IDs and duplicate controls exist; no common header contract | Existing action/workflow services | Remote and selected recording/SearchTimer operations | ADR-0042, ADR-0048 | `Idempotency-Key` is not universal, persisted, or conflict-checked across mutation routes | Retries can duplicate side effects | Slice 4 | Pending repeated-key same/different payload tests | Matrix | Durable operation repository | Phase 64/65 domain orchestration |
| Operation IDs | Remote request already requires `operationId`; Phase 62 Slice 1 propagates it into authorization audit | Remote action domain and `SecurityHttpGate` | `POST /api/vdr/remote/actions` | ADR-0042, ADR-0049 | Other routes use inconsistent operation identifiers; no shared operation lifecycle store | Cross-system tracing and retry resolution are incomplete | Slice 1 propagation; Slice 4 common lifecycle | Audit test confirms operation context | Architecture document | Request parser and audit repository | Phase 65 media sessions |
| Accountability event model | Phase 62 Slice 1 implements append-only SQLite rows and update/delete blockers | `AccountabilityEventRepository` | Authentication failures and remote authorization decisions | ADR-0049 | Event catalogue is partial; no transactional outbox, retention, protected query API, export, or mutation completion event | Missing or incomplete forensic evidence | Slice 1 pre-dispatch decisions; Slice 5 full catalogue/outbox/query | Repository append-only and audit allow/deny tests | Architecture document | SQLite and request context | Phase 67 public audit API |
| Audit actor and device | Actor, actor type, device, session, auth state, request, correlation, backend, operation, decision, reason, and outcome are persisted | `SecurityHttpGate` | Protected remote mutation and compatibility authentication denial | ADR-0049 | Native/agent identities are not yet issued; mutation completion evidence is later | Audit may identify only compatibility actor | Slice 1 then Slice 2/Slice 5 | Audit field assertions | Architecture document | Identity issuance | Phase 63 agents |
| Audit failure semantics | Allowed protected mutation is denied with 503 if pre-dispatch audit append fails | `SecurityHttpGate` | Remote action | ADR-0042, ADR-0049 | No transactional outbox joining domain state and audit completion | Other mutations can still dispatch without Phase 62 audit | Slice 1 first route; Slice 5 outbox | Audit-unavailable negative test | Development document | Durable SQLite | No later domain execution |
| Security error codes | Phase 62 Slice 1 adds machine-readable nested error with request ID and `no-store` | `SecurityHttpGate` | Authentication and authorization failures; enforced-mode migration guard | ADR-0048 | Other routes retain ad-hoc errors; `ApiResponse` still lacks general response headers | Clients must handle mixed error shapes during migration | Slice 1 first security errors; Slice 3/Slice 4 convergence | Error-code and no-secret assertions | Architecture document | HTTP response model | Phase 67 stable `/api/v1` errors |
| Browser compatibility | Default `legacy-basic` mode preserves the current local Basic-auth gate and wildcard rights | `SecurityConfiguration`, `TestHttpServer` | Existing frontend GETs and POSTs | ADR-0041 compatibility must be controlled | Shared credential remains insecure for non-local exposure | Accidental exposure grants broad control | Slice 1 explicit adapter; later controlled retirement | Existing frontend/full tests plus compatibility gate tests | Development document | Existing browser credential | Phase 67 client release |
| Native client compatibility | Headers, request IDs, scoped grants, and machine errors are transport-neutral | Security domain and HTTP gate | Unversioned compatibility API only | ADR-0048; proposed ADR-0051 is not accepted on main | No production token/session API and no stable `/api/v1` | Native clients cannot rely on a public stable contract | Slice 2-Slice 5 prepare contracts | Contract tests pending | Matrix | Identity/session runtime | Phase 67 public API/SDK |
| Administrative operations | Existing EPG cache and native-fuzzy routes have no actor-specific admin permission | Router/controllers | EPG cache refresh, preview-cache refresh, native fuzzy refresh, stale-probe delete | ADR-0013, ADR-0049 | No `admin.*` permission catalogue or protected audit query | High-impact operations share broad credential | Slice 3 | Pending admin allow/deny tests | Matrix | Roles and permissions | No Phase 63 operations |
| Credential/token leak prevention | Phase 62 Slice 1 never persists or reflects the Authorization value; errors use opaque request ID | Authenticator, gate, audit repository | All security failures | ADR-0041, ADR-0048, ADR-0049 | No systematic repository-wide redaction guard yet; browser storage/cookie policy is pending production auth | Secrets may enter future logs or responses | Slice 1 first tests; Slice 2 redaction and storage guard | Invalid credential is absent from body and audit fields | Architecture document | Logging inventory | Phase 67 public client hardening |

## Mutating and stateful POST inventory

The following is the Phase 62 migration inventory derived from `ApiRouter` and
the live-remote runtime. Validation, preview, and planning POSTs are listed
because HTTP method alone cannot decide whether a request mutates state.

| Route family | Runtime owner | Classification at Phase 62 start | Phase 62 migration target |
|---|---|---|---|
| `/api/vdr/remote/actions` | `LiveRemoteApiRuntime`, `RemoteActionController`, `RemoteActionService` | Mutating; operation ID, allowlist, capability, and backend write policy already exist | Migrated in Slice 1 to `remote.control@backend` with pre-dispatch accountability |
| `/api/recordings/actions/execute` and `/api/vdr/recordings/actions/execute` | Recording action execution controller/service | Mutating | Slice 3 permission, then Slice 4 common revision/idempotency |
| `/api/recordings/actions/validate`, `/preview` aliases | Recording validation/preview controllers | Non-mutating POST | Explicit read/preview policy during Slice 3; not silently treated as mutation |
| `/api/vdr/timers/actions/create`, `/update`, `/delete` | Timer action controller/services | Mutating | Slice 3 permissions; Slice 4 revisions/idempotency; Phase 64 retains orchestration |
| `/api/vdr/channels/move` and alias | Channel move controller/service | Mutating | Slice 3 permission; Slice 4 revision/idempotency |
| `/api/searchtimers`, create/update/delete/execute aliases | SearchTimer controllers/services | Mixed mutating workflow | Slice 3 permission catalogue; Slice 4 operation contract |
| SearchTimer validate/plan/preview aliases | SearchTimer validation/planning services | Non-mutating POST | Explicit safe classification during Slice 3 |
| `/api/epg/cache/refresh` | EPG cache controller | Administrative state change | Slice 3 `admin.epg.refresh` |
| SearchTimer preview cache refresh aliases | Preview cache refresh controller | Administrative cache mutation | Slice 3 admin permission |
| Native fuzzy refresh aliases | Native fuzzy operator refresh controller | Administrative probe/refresh mutation | Slice 3 admin permission |
| Native fuzzy stale-probe delete aliases | Stale-probe administration controller | Administrative deletion | Slice 3 admin permission |

## Phase 62 slice order

1. **Slice 1 Security identity foundation and first protected mutation**  
   Canonical request identity, central permission/scope decision, explicit
   legacy compatibility adapter, remote-control enforcement, request and
   correlation IDs, append-only pre-dispatch authorization evidence.
2. **Slice 2 Persistent identities, credentials, devices, sessions, and revocation**  
   User/device/session repositories, issuance, expiry, logout, revoke,
   rotation, browser-origin/CSRF contract, and native-client credential
   boundary.
3. **Slice 3 Roles, permissions, scopes, and complete mutation-route migration**  
   Permission catalogue, role assignment, administrative permissions, and
   explicit classification of non-mutating POST routes.
4. **Slice 4 Common mutation envelope, revisions, idempotency, and operation lifecycle**  
   Shared preconditions, `If-Match`, `Idempotency-Key`, replay semantics,
   conflict codes, durable operations, and consistent decision order.
5. **Slice 5 Complete accountability catalogue, transactional outbox, and protected queries**  
   Mutation outcomes, authentication lifecycle events, export/retention,
   protected audit reads, and fail-before-dispatch guarantees for every
   privileged mutation.
6. **Slice 6 Compatibility retirement readiness and Phase 62 closeout**  
   Migration controls, negative end-to-end suite, documentation truth, and
   explicit evidence that Phase 63-67 runtime was not pulled forward.

Phase 62 remains open after Slice 1.
