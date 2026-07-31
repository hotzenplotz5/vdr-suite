# Phase 62 Security and Identity Gap Matrix

Status: active Phase 62 planning and implementation matrix  
Repository baseline: `cb77ff66e11dca7db2eafa36525762dcde35102d` (`main`, merge of PR #115)  
Implemented branch slices: Slice 1 through Slice 2H; real-runtime-accepted lifecycle, managed Basic, browser-session issuance/logout, ordinary-route browser authentication, persisted actor grants, fixed exact-scope roles, Remote CSRF, Webfrontend session integration, Timer create/update/delete security migration and both Channel Move aliases

This matrix separates repository truth from accepted targets. A component is not accepted installed runtime until it is connected, tested through the daemon and validated on the real yaVDR system.

Validation rule: code-head evidence alone is insufficient for handoff or installation. The cumulative branch head, including reconciled status and architecture documentation, must pass the complete documentation, test-inventory, regression, daemon-build and packaging graph.

## Gap matrix

| Security area | Current state | Owner/component | Remaining gap | Risk | Next Phase 62 work | Evidence |
|---|---|---|---|---|---|---|
| Actor model | Canonical actor/device/session/credential context; legacy and optional managed actor active; browser sessions use same model | Security domain and lifecycle repositories | Protected enrollment and general user/service administration | Manual provisioning cannot be final multi-user operation | Lifecycle administration | Basic/browser tests and real-VDR managed attribution |
| Device identity | Persisted, actor-bound, revocable; browser issuer requires an existing owned device | identity repositories and issuer | Enrollment, possession proof, trust level and browser-device creation policy | Device ID alone is attribution, not strong trust | Login/device policy | Binding/revocation and issuer validation tests |
| Session identity | Basic sessions persistent; browser sessions generated atomically; isolated HTTPS issue/logout lifecycle and Webfrontend adoption real-runtime accepted | identity repository, issuer and lifecycle service | Refresh, idle timeout and cleanup | No complete long-lived browser operation policy yet | Refresh/cleanup | Issuance, logout, rollback, frontend and real-yaVDR acceptance |
| Credential identity | Managed Basic verifier accepted; browser credential generated atomically and linked to issuing credential | verifier repositories and issuer | Managed password lifecycle plus native/service credentials | First managed credential remains configuration-provisioned | Protected credential administration | Real-VDR Basic evidence and issuance tests |
| Authentication | Legacy/managed Basic and browser sessions authenticate ordinary routes; browser credentials have strict precedence and never fall back to Basic; Webfrontend login/logout is real-runtime accepted | authenticators, dedicated/general gates and Webfrontend session client | Native mechanisms and complete mutation policy | Incomplete route classification could weaken the boundary | Business-route classification | Basic, lifecycle, precedence, frontend, denial and real-runtime evidence |
| Browser-session issuance | Authenticated HTTPS exchange creates a server-generated bounded session and hardened cookie; Webfrontend consumption is real-runtime accepted | issuer, HTTP service and Webfrontend session client | Completion accountability | Lifecycle completion is not yet transactionally represented | Completion evidence | CSPRNG, rollback, cookie, no-reflection, frontend and real-yaVDR tests |
| Entropy/secret generation | Linux `getrandom(2)`, independent 128-bit IDs and 256-bit session/CSRF secrets | issuance service | Runtime failure observability without secret logging | Entropy failure must remain fail-closed | Operational acceptance | entropy-failure and architecture tests |
| CSRF | Independent verifier; logout, Remote, Timer create/update/delete and Channel Move enforce matching cookie-bound tokens; Webfrontend keeps the token in memory only | browser authenticator, gates and Webfrontend session client | Enforce before all remaining cookie-authenticated business mutations | Unmigrated routes must remain fail-closed | One route family per slice | lifecycle, migrated-route, frontend and real-runtime tests |
| Session lifecycle | Atomic issue and coupled verifier/session/credential logout implemented | issuance and lifecycle services | Refresh, idle expiry, cleanup, concurrency and recovery | Stale or orphaned sessions without maintenance policy | Lifecycle maintenance | atomic issuance/revocation tests |
| Roles | Fixed `role.admin` and `role.read-only` assignments use backend-scoped actor-grant persistence; exact-scope Read-only wins for all currently protected mutations and Admin expands only to the explicit Remote, Timer and Channel Move catalogue; installed-runtime accepted | `AuthorizationService` and `SecurityPermissionGrantRepository` | Later generic definitions, assignments and protected administration | Broad or wildcard role semantics could over-grant | Generic role administration after route migration | focused authorization tests plus real-runtime Admin/Read-only evidence |
| Permissions/scopes | Central exact/wildcard direct authorization; browser sessions load active actor grants; fixed roles use exact backend scope only | `AuthorizationService`, `SecurityPermissionGrantRepository` and authenticators | Complete route permission catalogue and protected administration | Legacy wildcard compatibility remains broad and direct provisioning is not final administration | Route migration before generic administration | repository tests plus real-yaVDR empty, scoped, revoked and unavailable grant acceptance |
| Central authorization | Remote, Timer create/update/delete and both Channel Move aliases are migrated; browser precedence, persisted grants, fixed-role constraints and unavailable-store denial are enforced; all other browser business POSTs remain fail-closed | `SecurityHttpGate` and `AuthorizationService` | Complete route permission mapping, safe-POST classification and business CSRF enforcement | Remaining legacy routes still rely on compatibility bypass or fail closed for browser sessions | One business route family per slice | general/dedicated gate, authorization and real-yaVDR evidence |
| Backend policy | Read-only/capability checks remain independent | existing backend/domain services | Preserve for every new identity path | Actor permission must not override backend state | Every slice | existing regression suite |
| Revisions/idempotency | Partial per-domain mechanisms | domain services | Common `If-Match`, idempotency and operation lifecycle | Retry/conflict behaviour inconsistent | Slice 4 | pending |
| Accountability | Append-only application and lifecycle pre-dispatch allow/deny evidence exists and is real-runtime accepted | accountability repository and gates | Completion/outcome events, transactional coupling/outbox, queries and retention | Authorization evidence alone cannot prove lifecycle completion | Completion evidence and Slice 5 | append-only, lifecycle-gate and real-VDR rows |
| Security errors | Stable nested errors plus browser issuance/revocation/CSRF codes | general and lifecycle gates/services | Role-specific browser wording and final Problem Details migration | Clients can mishandle inconsistent transport states | Frontend/Phase 67 hardening | current error tests |
| Secret leak prevention | Verifier rows store one-way values; issued result move-only/wiped; HTTP separates cookie and CSRF; Webfrontend stores CSRF only in memory | authenticators, repositories, issuer and Webfrontend | Repository-wide redaction review | One-time material could leak through future clients | Continued hardening | plaintext-negative, cookie no-reflection, frontend and wipe tests |
| Browser compatibility | Legacy Basic remains active; browser sessions drive the Webfrontend and migrated Remote, Timer and Channel Move routes while unmigrated browser POSTs fail closed | general gate, lifecycle gate and Webfrontend | Remaining business-route migration and later compatibility retirement | Shared legacy credential remains broadly privileged | Route migration | coexistence, no-fallback, grant-isolation and real-yaVDR tests |
| Native/service clients | Identity and authorization model is transport-neutral | security domain | Native/service token enrollment, rotation and refresh | No final app credential contract | later Phase 62 slices | pending |
| Administration | No security-management API | future admin services | Protected issue/revoke/reactivate/rotate/recover operations and generic role management | Direct DB/config operation is not final administration | After business-route migration | pending |

## Mutating and stateful POST inventory

| Route family | Current classification | Phase 62 migration target |
|---|---|---|
| `/api/security/browser-sessions` | Real-runtime-accepted Basic-authenticated self-service session issue | Completion accountability |
| `/api/security/browser-sessions/logout` | Real-runtime-accepted cookie+CSRF self-service revoke | Completion accountability |
| `/api/vdr/remote/actions` | Real-runtime-accepted browser mutation with CSRF, exact permission/scope and fixed-role enforcement | Completion accountability |
| Recording execute aliases | Mutation | permission + browser CSRF; later revision/idempotency |
| Recording validate/preview | Non-mutating POST | explicit safe/CSRF classification |
| Timer create/update/delete | Real-runtime-accepted browser mutation with exact permission/scope, fixed-role and CSRF enforcement | Completion accountability; later common contracts |
| Channel move aliases | Real-runtime-accepted browser mutation with `channels.move`, exact scope, fixed-role and CSRF enforcement | Completion accountability; later common contracts |
| SearchTimer create/update/delete/execute | Mixed mutation workflow | permission + browser CSRF + operation contract |
| SearchTimer validate/plan/preview | Non-mutating POST | explicit safe/CSRF classification |
| EPG/cache/native-fuzzy administration | Administrative state change | `admin.*` permission + browser CSRF |

## Phase 62 slice order

1. **Slice 1 — implemented and real-runtime validated**  
   Canonical context, central permission/scope decision, legacy adapter, Remote enforcement and append-only pre-dispatch evidence.

2. **Slice 2 — active**  
   Real-runtime accepted through Slice 2H: persistence, lifecycle resolution, managed Basic verifier, browser issuance/logout, ordinary-route browser authentication, backend-scoped grants, fixed exact-scope Admin/Read-only roles, Remote CSRF, Webfrontend session integration, Timer create/update/delete migration and both Channel Move aliases. Still required: further business-route migration, completion accountability, cleanup/recovery and native/service credential lifecycle.

3. **Slice 3 — generic roles, permissions, scopes and complete route migration**  
   Full permission catalogue, persisted generic role definitions/assignments, protected administration and safe POST/CSRF classification. Slice 2F does not pull that broad administration surface forward.

4. **Slice 4 — common mutation envelope, revisions, idempotency and operations**  
   Shared preconditions, `If-Match`, `Idempotency-Key`, replay/conflict semantics and durable operations.

5. **Slice 5 — complete accountability, outbox and protected queries**  
   Authentication/session/CSRF/mutation outcomes, outbox, audit reads, export and retention.

6. **Slice 6 — compatibility retirement readiness and closeout**  
   Migration controls, negative end-to-end suite, documentation truth and proof that later runtime was not pulled forward.

Phase 62 remains open. After the Slice 2H documentation closeout, inspect
the remaining POST inventory and select exactly one bounded Webfrontend route
family. Do not combine it with Android, generic administration or later-phase
runtime.
