# Phase 62 Security and Identity Gap Matrix

Status: active Phase 62 planning and implementation matrix  
Repository baseline: `cb77ff66e11dca7db2eafa36525762dcde35102d` (`main`, merge of PR #115)  
Implemented branch slices: Slice 1; real-runtime-accepted lifecycle, managed Basic, browser-session issuance/logout, ordinary-route browser authentication and accountability runtime; repository-validated persisted actor grants for browser sessions are not yet installed-runtime accepted

This matrix separates repository truth from accepted targets. A component is not accepted installed runtime until it is connected, tested through the daemon and validated on the real yaVDR system.

Validation rule: code-head evidence alone is insufficient for handoff or installation. The cumulative branch head, including reconciled status and architecture documentation, must pass the complete documentation, test-inventory, regression, daemon-build and packaging graph.

## Gap matrix

| Security area | Current state | Owner/component | Remaining gap | Risk | Next Phase 62 work | Evidence |
|---|---|---|---|---|---|---|
| Actor model | Canonical actor/device/session/credential context; legacy and optional managed actor active; browser sessions use same model | Security domain and lifecycle repositories | Protected enrollment and general user/service administration | Manual provisioning cannot be final multi-user operation | Lifecycle administration | Basic/browser tests and real-VDR managed attribution |
| Device identity | Persisted, actor-bound, revocable; browser issuer requires an existing owned device | identity repositories and issuer | Enrollment, possession proof, trust level and browser-device creation policy | Device ID alone is attribution, not strong trust | Login/device policy | Binding/revocation and issuer validation tests |
| Session identity | Basic sessions persistent; browser sessions generated atomically; isolated HTTPS issue/logout lifecycle real-runtime accepted | identity repository, issuer and lifecycle service | Refresh, idle timeout and cleanup | No complete long-lived browser operation policy yet | Refresh/cleanup | Issuance, logout, rollback and real-yaVDR acceptance |
| Credential identity | Managed Basic verifier accepted; browser credential generated atomically and linked to issuing credential | verifier repositories and issuer | Managed password lifecycle plus native/service credentials | First managed credential remains configuration-provisioned | Protected credential administration | Real-VDR Basic evidence and issuance tests |
| Authentication | Legacy/managed Basic and browser sessions authenticate ordinary read routes; browser credentials have strict precedence and never fall back to Basic | authenticators and dedicated/general gates | Frontend adoption, native mechanisms and complete mutation policy | Incorrect client handling or incomplete route classification could weaken the boundary | Business-route classification and frontend adoption | Basic, lifecycle, precedence, denial and real-runtime ordinary-route evidence |
| Browser-session issuance | Authenticated HTTPS exchange creates a server-generated bounded session and hardened cookie; real-runtime accepted | issuer, HTTP service and dedicated gate | Completion accountability and frontend adoption | Incorrect client handling could leak one-time secrets | Completion evidence and frontend integration | CSPRNG, rollback, cookie, no-reflection and real-yaVDR tests |
| Entropy/secret generation | Linux `getrandom(2)`, independent 128-bit IDs and 256-bit session/CSRF secrets | issuance service | Runtime failure observability without secret logging | Entropy failure must remain fail-closed | Operational acceptance | entropy-failure and architecture tests |
| CSRF | Independent verifier; dedicated logout enforces matching cookie-bound token | browser authenticator and lifecycle gate | Enforce before all cookie-authenticated business mutations | General cookie auth without route CSRF permits cross-site attempts | Route classification and Gate integration | logout correct/missing/wrong/expired/revoked tests |
| Session lifecycle | Atomic issue and coupled verifier/session/credential logout implemented | issuance and lifecycle services | Refresh, idle expiry, cleanup, concurrency and recovery | Stale or orphaned sessions without maintenance policy | Lifecycle maintenance | atomic issuance/revocation tests |
| Roles | Not implemented | none | Role definitions and actor-role assignments | Permission administration does not scale | Slice 3 | pending |
| Permissions/scopes | Central exact/wildcard authorization; legacy/managed grants remain configuration-backed; browser sessions load active actor grants from additive persistence | `AuthorizationService`, `SecurityPermissionGrantRepository` and authenticators | Roles, assignments, protected grant administration and complete route permission catalogue | Legacy wildcard compatibility remains broad and direct DB provisioning is not final administration | Role/administration design after business-route migration | repository, authenticator, scope-isolation and fail-closed resolution tests |
| Central authorization | Remote migrated; browser precedence and persisted actor-grant loading are connected; all browser-authenticated POSTs remain fail-closed; lifecycle routes use the isolated self-service gate | `SecurityHttpGate` and `BrowserSessionHttpGate` | Complete route permission mapping, safe-POST classification and business CSRF enforcement | Most legacy routes still rely on compatibility bypass | Route classification and CSRF migration | general/dedicated gate, grant-unavailability and real-VDR authentication evidence |
| Backend policy | Read-only/capability checks remain independent | existing backend/domain services | Preserve for every new identity path | Actor permission must not override backend state | Every slice | existing regression suite |
| Revisions/idempotency | Partial per-domain mechanisms | domain services | Common `If-Match`, idempotency and operation lifecycle | Retry/conflict behaviour inconsistent | Slice 4 | pending |
| Accountability | Append-only application and lifecycle pre-dispatch allow/deny evidence exists and is real-runtime accepted | accountability repository and gates | Completion/outcome events, transactional coupling/outbox, queries and retention | Authorization evidence alone cannot prove lifecycle completion | Completion evidence and Slice 5 | append-only, lifecycle-gate and real-VDR rows |
| Security errors | Stable nested errors plus browser issuance/revocation/CSRF codes | general and lifecycle gates/services | Client integration and final Problem Details migration | Clients can mishandle inconsistent transport states | Frontend/Phase 67 hardening | current error tests |
| Secret leak prevention | Verifier rows store one-way values; issued result move-only/wiped; HTTP separates cookie and CSRF | authenticators, repositories, issuer and HTTP service | Frontend memory/storage and repository-wide redaction review | One-time material could leak through client logging/storage | Frontend integration hardening | plaintext-negative, cookie no-reflection and wipe tests |
| Browser compatibility | Legacy Basic remains active; issued browser cookies authenticate ordinary read routes with strict precedence and persisted actor grants; browser POSTs remain blocked | general gate plus browser lifecycle gate | Frontend adoption, complete business CSRF and later compatibility retirement | Shared legacy credential remains broadly privileged | Route migration then frontend integration | coexistence, no-fallback, grant-isolation and real-runtime read-route tests |
| Native/service clients | Identity and authorization model is transport-neutral | security domain | Native/service token enrollment, rotation and refresh | No final app credential contract | later Phase 62 slices | pending |
| Administration | No security-management API | future admin services | Protected issue/revoke/reactivate/rotate/recover operations | Direct DB/config operation is not final administration | Slice 3 after browser lifecycle | pending |

## Mutating and stateful POST inventory

| Route family | Current classification | Phase 62 migration target |
|---|---|---|
| `/api/security/browser-sessions` | Real-runtime-accepted Basic-authenticated self-service session issue | Completion accountability and frontend adoption |
| `/api/security/browser-sessions/logout` | Real-runtime-accepted cookie+CSRF self-service revoke | Completion accountability and frontend adoption |
| `/api/vdr/remote/actions` | Migrated mutation with operation ID | Browser cookie additionally requires CSRF before authorization/dispatch |
| Recording execute aliases | Mutation | permission + browser CSRF; later revision/idempotency |
| Recording validate/preview | Non-mutating POST | explicit safe/CSRF classification |
| Timer create/update/delete | Mutation | permission + browser CSRF; later common contracts |
| Channel move aliases | Mutation | permission + browser CSRF |
| SearchTimer create/update/delete/execute | Mixed mutation workflow | permission + browser CSRF + operation contract |
| SearchTimer validate/plan/preview | Non-mutating POST | explicit safe/CSRF classification |
| EPG/cache/native-fuzzy administration | Administrative state change | `admin.*` permission + browser CSRF |

## Phase 62 slice order

1. **Slice 1 — implemented and real-runtime validated**  
   Canonical context, central permission/scope decision, legacy adapter, Remote enforcement and append-only pre-dispatch evidence.

2. **Slice 2 — active**  
   Real-runtime accepted: persistence, lifecycle resolution, managed Basic verifier, legacy-bypass separation, backend-scoped Remote authorization, browser verifier, CSPRNG material, atomic issuance, hardened Basic-to-session HTTPS exchange, cookie+CSRF logout, ordinary-route browser authentication, coupled revocation and lifecycle pre-dispatch accountability. Repository-validated but not yet installed-runtime accepted: additive actor permission grants, browser grant loading and fail-closed grant-store unavailability. Still required: business-route classification and CSRF, frontend integration, completion accountability, cleanup/recovery and native/service credential lifecycle.

3. **Slice 3 — roles, permissions, scopes and complete route migration**  
   Permission catalogue, persisted roles/grants/scopes, administration permissions and safe POST/CSRF classification.

4. **Slice 4 — common mutation envelope, revisions, idempotency and operations**  
   Shared preconditions, `If-Match`, `Idempotency-Key`, replay/conflict semantics and durable operations.

5. **Slice 5 — complete accountability, outbox and protected queries**  
   Authentication/session/CSRF/mutation outcomes, outbox, audit reads, export and retention.

6. **Slice 6 — compatibility retirement readiness and closeout**  
   Migration controls, negative end-to-end suite, documentation truth and proof that later runtime was not pulled forward.

Phase 62 remains open. After repository and separately approved runtime acceptance of persisted browser grants, the next strict work is business-route permission classification and browser CSRF enforcement—not Android or later-phase runtime.
