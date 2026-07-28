# Phase 62 Security and Identity Gap Matrix

Status: active Phase 62 planning and implementation matrix  
Repository baseline: `cb77ff66e11dca7db2eafa36525762dcde35102d` (`main`, merge of PR #115)  
Implemented branch slices: Slice 1; real-runtime-accepted lifecycle and managed Basic increments; CI-validated browser-session verifier and atomic issuance foundations not yet connected to HTTP/Gate runtime

This matrix separates repository truth from accepted targets. A staged component is not active request-path runtime unless connected and tested through `TestHttpServer` and `SecurityHttpGate`.

## Gap matrix

| Security area | Current state | Owner/component | Remaining gap | Risk | Next Phase 62 work | Evidence |
|---|---|---|---|---|---|---|
| Actor model | Canonical actor/device/session/credential context; legacy and optional managed actor active; browser sessions use same model | Security domain and lifecycle repositories | Protected enrollment and general user/service administration | Manual provisioning cannot be final multi-user operation | Lifecycle administration | Basic and browser tests; real-VDR managed attribution |
| Device identity | Persisted, actor-bound, revocable; browser issuer requires an existing owned device | identity repositories and issuer | Enrollment, possession proof, trust level and browser-device creation policy | Device ID alone is attribution, not strong trust | Login/device policy | Binding/revocation and issuer validation tests |
| Session identity | Basic sessions persistent; browser session IDs generated with 128-bit entropy and atomically persisted | identity repository and `BrowserSessionIssuanceService` | HTTP login, refresh, logout, idle timeout and cleanup | No deployable browser flow yet | HTTP login/logout | Issuance, collision rollback and verifier tests |
| Credential identity | Managed Basic verifier accepted; browser credential generated atomically and linked to issuing credential | verifier repositories and issuer | Managed password lifecycle plus native/service credentials | First managed credential remains configuration-provisioned | Protected credential administration | Real-VDR Basic evidence and issuance tests |
| Authentication | Legacy/managed Basic active; browser cookie verifier implemented but not Gate-wired | authenticators and Gate | Authentication precedence, HTTP response contract and native mechanisms | Enabling cookies without integration policy would be unsafe | Login and Gate integration | Basic matrix and browser verifier tests |
| Browser-session issuance | Server-generated IDs/secrets, bounded lifetime and atomic three-row transaction implemented | `BrowserSessionIssuanceService` and repositories | Expose only through authenticated HTTP login and add accountability | Incorrect response handling could leak one-time secrets | Secure login response | CSPRNG, lifetime, rollback and wipe tests |
| Entropy/secret generation | Linux `getrandom(2)`, independent 128-bit IDs and 256-bit session/CSRF secrets | issuance service | Deployment/runtime failure reporting and operational observability without secret logging | Entropy failure must remain fail-closed | HTTP error/accountability contract | entropy-failure and architecture tests |
| CSRF | Independent verifier bound to active browser session | `BrowserSessionAuthenticator` | Deliver token securely and enforce before all cookie-authenticated mutations | Cookie auth without CSRF permits cross-site attempts | Gate integration and route classification | correct/missing/wrong/expired/revoked tests |
| Session lifecycle | Issuance writes one expiry to session, credential and verifier; revocation primitives exist | identity/browser repositories | Coupled logout, refresh, idle expiry, cleanup and concurrency policy | Orphaned/long-lived sessions without administration | Logout and lifecycle service | atomic issuance and revocation tests |
| Roles | Not implemented | none | Role definitions and actor-role assignments | Permission administration does not scale | Slice 3 | pending |
| Permissions/scopes | Central exact/wildcard authorization; transient legacy/managed grants | `AuthorizationService` and configuration | Persisted roles/grants/scopes and browser grant loading | Legacy wildcard compatibility remains broad | Slice 3 | authorization and real-VDR scope evidence |
| Central authorization | Remote migrated; managed/unmapped POSTs fail before router; browser path not connected | `SecurityHttpGate` | Cookie precedence, CSRF decision and complete route mapping | Most legacy routes still rely on compatibility bypass | Browser integration then route migration | Gate and real-VDR Timer/Remote evidence |
| Backend policy | Read-only/capability checks remain independent | existing backend/domain services | Preserve for every new identity path | Actor permission must not override backend state | Every slice | existing regression suite |
| Revisions/idempotency | Partial per-domain mechanisms | domain services | Common `If-Match`, idempotency and operation lifecycle | Retry/conflict behaviour inconsistent | Slice 4 | pending |
| Accountability | Append-only pre-dispatch allow/deny evidence exists | `AccountabilityEventRepository` | Issuance/login/logout/CSRF events, outcomes, outbox, queries and retention | Browser lifecycle otherwise lacks forensic evidence | HTTP integration and Slice 5 | append-only and real-VDR rows |
| Security errors | Stable nested errors, request ID and `no-store` for current Gate | Gate | Login/logout/cookie/CSRF-specific codes | Clients would otherwise receive inconsistent shapes | HTTP integration | current error tests |
| Secret leak prevention | Basic and browser verifier rows store one-way values only; issued result is move-only and wiped | authenticators, repositories, issuer, architecture guards | Secure HTTP response, frontend memory/storage and repository-wide redaction review | One-time cookie/CSRF material could leak in logs or responses | Login hardening | plaintext-negative, no-log and wipe tests |
| Browser compatibility | Legacy Basic remains active; staged issuer/verifier change no request behaviour | current Gate plus staged browser components | Controlled switch, cookie policy, logout and CSRF | Shared legacy credential remains broadly privileged | Slice 2 integration then closeout | coexistence tests and explicit no-wiring guard |
| Native/service clients | Identity and authorization model is transport-neutral | security domain | Native/service token enrollment, rotation and refresh | No final app credential contract | later Phase 62 slices | pending |
| Administration | No security-management API | future admin services | Protected issue/revoke/reactivate/rotate/recover operations | Direct DB/config operation is not final administration | Slice 3 after browser lifecycle | pending |

## Mutating and stateful POST inventory

| Route family | Current classification | Phase 62 migration target |
|---|---|---|
| `/api/vdr/remote/actions` | Migrated mutation with operation ID | Browser cookie additionally requires CSRF before authorization/dispatch |
| Recording execute aliases | Mutation | permission + browser CSRF; later revision/idempotency |
| Recording validate/preview | Non-mutating POST | explicit safe/CSRF classification |
| Timer create/update/delete | Mutation | permission + browser CSRF; later common contracts |
| Channel move aliases | Mutation | permission + browser CSRF |
| SearchTimer create/update/delete/execute | Mixed mutation workflow | permission + browser CSRF + operation contract |
| SearchTimer validate/plan/preview | Non-mutating POST | explicit safe/CSRF classification |
| EPG/cache/native-fuzzy administration | Administrative state change | `admin.*` permission + browser CSRF |
| Future browser login/logout | Credential/session lifecycle | authenticated issuance, secure cookie response, coupled logout and accountability |

## Phase 62 slice order

1. **Slice 1 — implemented and real-runtime validated**  
   Canonical context, central permission/scope decision, legacy adapter, Remote enforcement and append-only pre-dispatch evidence.

2. **Slice 2 — active**  
   Real-runtime accepted: persistence, lifecycle resolution, managed Basic verifier, legacy-bypass separation and backend-scoped Remote authorization. Implemented and CI validated: browser verifier, CSPRNG material, bounded lifetime, atomic session/credential/verifier issuance, rollback and secret wiping. Still required: HTTP login/logout, secure cookie response, authentication precedence, resolver/Gate integration, real CSRF enforcement, accountability, cleanup/recovery and native/service credential lifecycle.

3. **Slice 3 — roles, permissions, scopes and complete route migration**  
   Permission catalogue, persisted roles/grants/scopes, administration permissions and safe POST/CSRF classification.

4. **Slice 4 — common mutation envelope, revisions, idempotency and operations**  
   Shared preconditions, `If-Match`, `Idempotency-Key`, replay/conflict semantics and durable operations.

5. **Slice 5 — complete accountability, outbox and protected queries**  
   Authentication/session/CSRF/mutation outcomes, outbox, audit reads, export and retention.

6. **Slice 6 — compatibility retirement readiness and closeout**  
   Migration controls, negative end-to-end suite, documentation truth and proof that later runtime was not pulled forward.

Phase 62 remains open. The next strict work is HTTP login/logout and secure cookie construction, followed by resolver/Gate integration and real CSRF enforcement—not Android or later-phase runtime.
