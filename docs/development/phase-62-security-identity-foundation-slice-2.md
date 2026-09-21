# Phase 62 Persistent Identity Lifecycle — Slice 2

Status: lifecycle, managed Basic, browser-session issuance/logout, ordinary-route browser authentication and persisted browser actor grants are real-VDR accepted; business-mutation classification and CSRF enforcement remain open; Phase 62 remains active

## Purpose

Phase 62 Slice 1 established explicit request identity, centralized authorization, the first protected mutation and append-only pre-dispatch accountability. Slice 2 removes transient-only identity assumptions and builds a deployable browser/native credential lifecycle in small fail-closed increments.

The slice now contains seven cumulative increments:

1. persistent actor, device, session and credential lifecycle state;
2. an optional separately provisioned managed Basic verifier;
3. a persistent browser-session credential repository and cookie/CSRF verifier;
4. atomic server-side browser-session material generation and persistence;
5. two isolated HTTP lifecycle routes for Basic-to-session exchange and cookie-plus-CSRF logout;
6. strict-precedence browser authentication for ordinary application read routes;
7. additive persisted actor grants and backend scopes for browser contexts.

A valid browser cookie now authenticates ordinary application read requests.
Presented browser credentials have strict precedence and never fall back to Basic.
Every browser-authenticated POST remains fail-closed until that route has an
explicit permission, safe/mutating classification and applicable CSRF contract.

## Active runtime boundary

```text
HttpServerRequest
  -> exact POST /api/security/browser-sessions?
       -> BrowserSessionHttpGate
            -> LegacyBasicAuthenticator or optional ManagedBasicAuthenticator
            -> PersistentIdentityResolver
            -> pre-dispatch accountability: session.issue.self
       -> BrowserSessionHttpService
            -> BrowserSessionIssuanceService
            -> Set-Cookie plus one-time CSRF JSON

  -> exact POST /api/security/browser-sessions/logout?
       -> BrowserSessionHttpGate
            -> BrowserSessionAuthenticator
            -> PersistentIdentityResolver
            -> independent X-CSRF-Token verification
            -> pre-dispatch accountability: session.revoke.self
       -> BrowserSessionHttpService
            -> BrowserSessionLifecycleService
            -> atomic verifier/session/credential revocation
            -> expired Set-Cookie

  -> every other request
       -> SecurityHttpGate
            -> BrowserSessionAuthenticator when a cookie is presented
                 -> BrowserSessionCredentialRepository
                 -> SecurityPermissionGrantRepository
            -> otherwise LegacyBasicAuthenticator
            -> otherwise optional ManagedBasicAuthenticator
            -> PersistentIdentityResolver
            -> AuthorizationService
            -> AccountabilityEventRepository
       -> ApiRouter
```

The dedicated browser lifecycle gate runs before the general gate but handles only the two exact POST paths. It does not create a second application-authorization path.

## Implemented lifecycle and managed Basic scope

- additive `security_actors`, `security_devices`, `security_sessions` and `security_credentials` tables;
- active, expiry and revocation state evaluated for every authenticated Basic request;
- restart-safe compatibility bootstrap with `INSERT OR IGNORE`;
- optional separately provisioned managed actor/device/session/credential;
- dedicated `security_basic_credential_verifiers` table;
- strict bounded Basic parsing;
- thread-safe `crypt_r` verification of yescrypt (`$y$`) and SHA-512 crypt (`$6$`) hashes;
- constant-time verifier-result comparison;
- managed identities receive no default permission and never inherit the legacy unmigrated-POST bypass;
- stable fail-closed lifecycle and credential errors before router dispatch.

## Browser-session storage and verifier scope

`BrowserSessionCredentialRepository` owns `security_browser_session_credentials`. Each row stores:

- non-secret token ID;
- actor, device, session, browser credential and issuing credential bindings;
- separate one-way modular hashes for the session and CSRF secrets;
- active, expiry, revocation, creation and update state.

The cookie value is:

```text
<token-id>.<high-entropy-session-secret>
```

The complete cookie value, raw session secret, raw CSRF token, submitted Cookie header, Authorization header, plaintext password and reversible secret are not persisted.

`BrowserSessionAuthenticator` provides bounded parsing, duplicate rejection, invalid/expired/revoked/authenticated outcomes, context construction and independent CSRF verification only for a valid active session.

## Atomic browser-session issuance contract

`BrowserSessionIssuanceService` generates all secret material server-side.

### Entropy and identifiers

- entropy is read from Linux `getrandom(2)` with complete-read and `EINTR` handling;
- token ID, session ID and credential ID each contain 128 random bits with distinct prefixes;
- session secret and CSRF secret each contain independent 256 random bits encoded as unpadded Base64url;
- the two secrets use independent random crypt salts;
- both hashes use SHA-512 crypt with `rounds=10000`;
- token ID is a lookup identifier, not an authentication secret.

### Lifetime

- default: 28,800 seconds (8 hours);
- minimum: 300 seconds (5 minutes);
- maximum: 86,400 seconds (24 hours);
- one UTC expiry is written to session, credential and verifier records.

### Issuing identity validation

Inside the write transaction the service re-reads and validates:

- active, unrevoked actor;
- active, unrevoked device owned by that actor;
- active, unexpired, unrevoked issuing credential owned by that actor.

Caller-supplied lifecycle claims are not trusted.

### Atomic persistence

The service acquires the transaction lease and starts `BEGIN IMMEDIATE`. It creates:

1. one `security_sessions` row;
2. one `security_credentials` row of type `browser-session`, with the issuing credential recorded as rotation predecessor;
3. one `security_browser_session_credentials` row containing lookup identity and one-way hashes.

All three rows commit together. Any validation failure, identifier conflict, repository failure or commit failure rolls back. The collision test deliberately lets the first two inserts succeed and forces the verifier insert to fail; neither intermediate identity row survives.

### One-time result handling

A successful issuance returns IDs, sensitive cookie value, sensitive CSRF token and expiry to the HTTP service. `IssuedBrowserSession` is move-only; copy construction and assignment are disabled. Its destructor and `clearSecrets()` overwrite the sensitive buffers. The result must not be logged.

## HTTP login contract

`POST /api/security/browser-sessions` is a credential exchange, not a plaintext-password JSON endpoint.

- authentication source: existing legacy Basic or optional managed Basic;
- persistent actor/device/session/credential lifecycle is resolved before issuance;
- anonymous or invalid credentials return `401` and a Basic challenge;
- successful response status is `200`;
- response body contains only `csrfToken`, `expiresAt` and `requestId`;
- response uses `Cache-Control: no-store` and `Pragma: no-cache`;
- the cookie value is delivered only through `Set-Cookie`;
- cookie attributes are `Path=/`, `Max-Age=28800`, `HttpOnly`, `Secure` and `SameSite=Strict`;
- no `Domain` attribute is emitted;
- cookie value, session ID and credential ID are not included in JSON.

The CSRF value is deliberately readable by the authenticated client and must be retained only in client memory by future frontend integration. The `HttpOnly` session secret remains inaccessible to browser JavaScript.

## HTTP logout contract

`POST /api/security/browser-sessions/logout` accepts only:

- one valid `vdr_suite_session` cookie;
- one matching `X-CSRF-Token` header;
- active persistent actor/device/session/credential state.

Basic credentials do not substitute for the browser cookie on logout. A missing or wrong CSRF token returns `403 csrf_validation_failed` before lifecycle mutation.

`BrowserSessionLifecycleService` starts `BEGIN IMMEDIATE` and atomically revokes:

1. the browser verifier row;
2. the canonical session row;
3. the canonical browser credential row.

A successful logout returns `204`, `no-store`/`no-cache` and an expired cookie using `Max-Age=0`, a 1970 `Expires` date, `HttpOnly`, `Secure` and `SameSite=Strict`.

## Ordinary-route authentication and route-safety rule

The lifecycle gate still recognizes exactly two POST paths. The general gate now
authenticates browser cookies for ordinary application requests.

- valid browser sessions can access ordinary read routes;
- a presented browser cookie takes precedence over Basic;
- invalid, expired, revoked, duplicate or unknown cookies never fall back;
- browser-cookie failures do not emit a misleading Basic challenge;
- every browser-authenticated POST returns
  `503 security_policy_not_migrated` before business dispatch.

The legacy compatibility bypass still belongs only to the configured legacy
actor and credential. Managed Basic identities use migrated routes only with
explicit permission and scope.

## Persisted browser permission-grant contract

`SecurityPermissionGrantRepository` owns
`security_actor_permission_grants`.

The additive key is:

```text
actor_id + permission + backend_id
```

`BrowserSessionAuthenticator` loads only active, unrevoked rows for the
authenticated actor. A successful empty result is valid and supplies no rights.

Grant-store failure is distinct from an empty result. The authenticated context
is marked unavailable and `SecurityHttpGate` returns
`503 permission_grants_unavailable`.

Browser sessions never inherit:

- legacy compatibility grants;
- managed-Basic grants;
- issuing-credential grants;
- implicit defaults or wildcards.

This increment does not add roles, protected grant administration or business
mutation authorization.

## Accountability boundary

The dedicated lifecycle gate writes append-only pre-dispatch decisions for:

- `session.issue.self`;
- `session.revoke.self`;
- authentication failure;
- `csrf_validation_failed`;
- successful self-service lifecycle authorization.

These records prove the security decision before the lifecycle service runs. A complete issuance/revocation outcome event, transactional coupling between lifecycle change and outcome evidence, protected audit queries and retention remain open.

## SQLite ownership boundary

Phase 62 SQL remains in approved repository implementation units under `core/security/src/`.

- public repository headers contain declarations and domain values only;
- issuance and lifecycle services use `Database` and repository abstractions and contain no direct `sqlite3_*` call;
- `SecurityIdentityIssuanceRepository.cpp` owns session/credential inserts;
- identity and browser repositories own revocation updates;
- `SecurityPermissionGrantRepository.cpp` owns actor permission-grant SQL;
- the daemon and affected tests link the complete `SECURITY_SRC` graph;
- arbitrary services, headers, REST controllers, helpers and unregistered tests remain outside the direct SQLite boundary.

## Real-VDR acceptance of the persistence/revocation foundation

The lifecycle increment was installed on yaVDR using `/var/lib/vdr-suite/vdr-suite.db`.

Observed on 2026-07-28:

- startup created compatibility identity bindings;
- revoking `legacy-basic-credential` blocked browser authentication immediately without restart;
- accountability recorded `credential_revoked` and `dispatch_denied`;
- restoring the credential restored browser access without restart;
- the next Remote request was authorized as `remote.control@default` and recorded as `dispatch_authorized`.

## Real-VDR acceptance of the managed verifier increment

Observed on 2026-07-28:

- separate managed actor `user-phase62-admin`, device, session, credential and `$6$` verifier were provisioned;
- correct credentials returned `200` for `GET /api/backends`;
- wrong password returned `401 invalid_credentials` with `no-store` and no reflection;
- unmigrated Timer POST returned `503 security_policy_not_migrated` before dispatch;
- Remote operation `phase62-managed-1785212278` succeeded with `remote.control@default`;
- accountability recorded invalid, denied/unmapped and allowed/dispatch-authorized decisions with expected identity attribution.

## Real-VDR acceptance of the isolated HTTPS lifecycle

Observed on 2026-07-28 through the installed daemon and the local HTTPS reverse proxy:

- anonymous session issuance returned `401 authentication_required`;
- managed-Basic session issuance returned `200`;
- the response contained one-time CSRF, expiry and request ID values;
- the cookie carried `Path=/`, `HttpOnly`, `Secure` and `SameSite=Strict`;
- logout without CSRF returned `403 csrf_validation_failed`;
- valid cookie-plus-CSRF logout returned `204`;
- replay of the revoked browser credential returned `401 credential_revoked`;
- verifier, canonical session and canonical browser credential were all inactive with populated revocation timestamps;
- SQLite integrity returned `ok`;
- the foreign-key check returned no violations;
- the lifecycle consistency query returned zero inconsistent rows;
- accountability recorded issue allow, CSRF deny, revoke allow and revoked-credential replay deny decisions.

The HTTPS proxy rule is a deliberately local runtime integration. No yaVDR-Ansible playbook was run, and the acceptance did not modify the yaVDR-Ansible repository.

## Real-VDR acceptance of persisted browser grants

Observed on 2026-07-29 through the installed daemon and the Suite public origin
at repository head `47adb6577511209bfe7288ce8ce0fbe03b53a94c`:

- daemon-only installation produced installed SHA-256 `652dfc6a29f466fca977d34587db8a39bbc631509b735e02f8dd1942c46088e1`;
- `security_actor_permission_grants` and its active-grant index were created
  additively with zero default rows;
- an issued browser session authenticated ordinary reads with an empty
  successfully resolved grant set;
- two `remote.control` grants for separate backend scopes were loaded for the
  authenticated actor;
- revoking the `default` scope preserved the alternate scope;
- browser POSTs remained blocked by `503 security_policy_not_migrated` both
  before and after grant persistence;
- temporary grant-table unavailability returned
  `503 permission_grants_unavailable`;
- restoring the table restored ordinary read access for the same session;
- cookie-plus-CSRF logout returned `204`;
- replay returned `401 credential_revoked`;
- verifier, canonical session and browser credential were revoked consistently;
- acceptance grant rows were removed;
- `PRAGMA quick_check` returned `ok`, foreign-key violations remained zero and
  no accountability append failure was observed.

The daemon backup is `/var/backups/vdr-suite-phase62-slice2c-20260729-171704`. No frontend, Nginx or credential configuration
was changed.

## Automated validation

GitHub Actions VDR-Suite CI run 6367 completed successfully on atomic issuance code head `b79e3fdc597dd8eb245641ba2c7363dd9542e631`.

GitHub Actions VDR-Suite CI run 6429 completed successfully on HTTP lifecycle code head `f69a2dd87929b38a5561d1259901a72d4e78093c`.

The HTTP lifecycle run passed:

- documentation and repository structure;
- strict Make/test inventory and complete test-graph dry-run;
- frontend regression;
- complete fast C++/runtime regression;
- issuance, HTTP response, dedicated gate, lifecycle revocation and existing Security gate tests;
- full daemon link;
- packaging and install staging.

Focused tests cover secure cookie attributes, no cookie reflection in JSON,
one-time CSRF response, successful verifier consumption, atomic logout revocation,
revoked post-logout authentication, anonymous/wrong-password login denial,
missing-CSRF denial, ordinary-route precedence, persisted actor grants,
backend-scope separation, empty grant resolution, unavailable grant persistence
and lifecycle accountability decisions.

The Slice 2C local validation passed `make test-security`,
`make test-ci-fast`, focused grant/authenticator/gate tests and the daemon build.
Installed-runtime acceptance of persisted browser grants completed on 2026-07-29. The next approval boundary is business-route classification and browser CSRF enforcement.

## Explicitly not included

- enabling browser-authenticated business POST routes;
- actual browser CSRF enforcement for Remote, Timer, Recording or other business mutations;
- protected role, assignment or grant-administration APIs;
- frontend login/logout user interface or client-memory CSRF handling;
- refresh, idle timeout, cleanup, concurrent-session policy, recovery or enrollment;
- complete issuance/revocation outcome accountability and transactional outbox;
- protected managed/native/service credential administration;
- persisted roles and role assignments or protected grant-administration APIs;
- complete route migration;
- universal revision, idempotency, operation lifecycle or outbox completion;
- Phase 63-67 runtime or clients.

## Acceptance evidence

| Criterion | Evidence |
|---|---|
| Persistent lifecycle is enforced and recoverable | repository tests and real-VDR revoke/restore |
| Managed Basic is independent from legacy compatibility | tests and real-VDR acceptance |
| Browser secrets persist only as independent one-way hashes | repository/authenticator/issuance/architecture tests |
| Entropy is server-generated | `getrandom(2)` and architecture guard |
| Session, credential and verifier issuance is atomic | forced-collision rollback test |
| Login uses existing authenticated credentials without a plaintext JSON password | dedicated gate and HTTP tests |
| Cookie is secret-only and hardened | `Set-Cookie` attribute and no-reflection tests |
| CSRF is delivered separately and not persisted raw | HTTP response and repository tests |
| Logout requires cookie plus matching CSRF | dedicated gate tests |
| Logout revokes verifier, session and credential atomically | lifecycle and post-logout verifier tests |
| Browser cookies authenticate ordinary reads with strict no-fallback precedence | general-gate and real-runtime acceptance |
| Browser grants come only from active actor rows | grant-repository, authenticator and gate tests plus real-yaVDR actor/scope acceptance |
| Empty grant resolution differs from persistence failure | repository tests plus real-yaVDR empty/unavailable/recovery acceptance |
| Browser sessions inherit no Basic grants | negative grant-isolation tests |
| Lifecycle allow/deny and CSRF decisions are append-only | dedicated gate accountability tests |
| Daemon and packaging link the HTTP lifecycle | CI run 6429 |
| Installed HTTPS lifecycle behaves fail-closed | real-yaVDR `401 -> 200 -> 403 -> 204 -> 401 credential_revoked` acceptance |

Canonical checks:

```text
make test-security
make test-architecture
make test-docs
make test
```

Phase 62 remains open. The next strict increment is explicit business-route classification and server-side browser CSRF enforcement, beginning with one bounded route family before dispatch. Completion accountability, maintenance/recovery and protected lifecycle administration remain later work.
