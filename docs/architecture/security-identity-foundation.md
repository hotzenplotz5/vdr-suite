# Security and Identity Foundation

Status: **Phase 62 completed and real-runtime accepted through Slice 2X.**

## Runtime boundary

The server owns authentication, lifecycle resolution, CSRF, authorization and accountability before protected dispatch.

```text
HttpServerRequest
  -> exact browser-session lifecycle gate for issue/logout
  -> general SecurityHttpGate
       -> presented browser cookie with strict precedence
       -> otherwise Legacy Basic compatibility or optional Managed Basic
       -> persistent actor/device/session/credential resolution
       -> browser-session lifetime, idle, issuer and concurrency policy
       -> exact mutation/Safe-POST classification
       -> browser CSRF verification where applicable
       -> exact permission and backend-scope authorization
       -> append-only pre-dispatch accountability
  -> ApiRouter
  -> domain/backend safety checks and mutation
  -> append-only protected mutation outcome
  -> HttpServerResponse
```

Frontends do not own authentication, role, scope or CSRF decisions. Private providers and adapters do not become security authorities.

## Identity model

`RequestSecurityContext` carries:

- authentication state;
- actor identity and actor type;
- optional device, session and credential identity;
- exact permission grants and backend scopes;
- request ID and optional correlation ID.

Actor types represent security principals such as users, services, agents and system work. Legacy Basic, Managed Basic and browser sessions construct the same transport-neutral context and pass it through persistent lifecycle resolution.

The Phase-62 Actor is the authorization/accountability principal, not a declaration that `Actor == Human User == Profile`. Post-Phase-69 productization must keep at least these concepts distinct:

- Actor;
- Human User / Account;
- Profile;
- Credential;
- Session;
- Device / Client;
- Permission / Grant;
- Backend / Library / Content Scope;
- Capability.

Technical Actors may represent API clients, services, remote VDR-Suite peers or other non-human identities. P1 — Identity Model Audit must inventory the live schema and lifecycle before deciding how Human User/Account and Profile map onto this security authority.

No productization slice may create a parallel identity authority merely to add human accounts or profiles.

## Persistent lifecycle

Suite-owned repositories persist and validate:

- actors;
- devices;
- sessions;
- credentials;
- managed Basic verifiers;
- browser-session verifiers;
- actor permission grants and fixed role assignments;
- append-only accountability events.

Missing, inactive, expired, revoked or cross-owner lifecycle state fails closed.

This persistent Suite identity store is the authority to extend for Human User/account administration, device/session administration and future pairing. `/etc/vdr-suite` remains system/deployment configuration and is not the normal human-user/password database.

## Authentication mechanisms

### Legacy Basic compatibility

Legacy Basic remains a separately identified transitional deployment mode. It does not define the target architecture and is not inherited by Managed Basic or browser identities.

### Managed Basic

Managed Basic uses a separately provisioned persistent identity and one-way modular password verifier. Supported verifier formats are bounded and verified using thread-safe crypt handling and constant-time result comparison.

### Browser sessions

Browser-session issuance generates independent high-entropy session and CSRF secrets. Only a non-secret lookup token and one-way verifier hashes are persisted.

A presented browser cookie has strict precedence. Malformed, duplicate, unknown, expired or revoked browser credentials do not fall back to Basic.

## Browser-session lifecycle policy

The accepted runtime includes:

- atomic issuance of canonical session, browser credential and browser verifier;
- hardened `Secure`, `HttpOnly`, `SameSite=Strict` cookie;
- independent one-time CSRF response retained only in frontend memory;
- atomic logout/revocation;
- configurable immutable absolute lifetime;
- optional per-actor active-session limit;
- optional idle expiry with throttled activity persistence;
- request-time issuing-credential lifecycle binding;
- one bounded startup cleanup pass for terminal browser-session lifecycles.

Retention cleanup deletes only the terminal verifier and its own unreferenced canonical browser session and exact browser credential. Actor, device, issuer, grants, roles and accountability history remain preserved.

## Authorization

Authorization is server-side and combines:

- authenticated persistent actor context;
- exact permission;
- exact backend scope;
- fixed role expansion where assigned;
- independent backend read-only, capability and domain safety policy.

Every registered central POST is either:

- a protected mutation with exact permission/scope and browser CSRF where applicable; or
- an explicitly classified Safe POST that cannot perform the protected mutation effect.

Unknown browser and enforced-mode mutation paths fail closed.

## Accountability

### Pre-dispatch

Append-only events record actor, device, session, authentication state, permission, backend scope, action, operation ID, request ID, correlation ID, decision and reason. Required append failure prevents dispatch.

### Browser lifecycle

Issue, revoke and retention cleanup completion outcomes are persisted without raw secrets.

### Protected mutation outcomes

For each authorized protected mutation reaching the router:

```text
HTTP 200..299  -> event_type=operation.succeeded, outcome=succeeded
all other HTTP -> event_type=operation.failed, outcome=failed
reason_code    -> http_status_<decimal status>
```

The outcome reuses the successful authorization context. If the post-dispatch append fails, the client receives `503 accountability_unavailable`. The runtime makes no cross-system rollback, transactional Outbox or safe-replay claim.

## Secret restrictions

The following are never persisted or reflected into accountability:

- complete Authorization headers;
- plaintext passwords or reversible password material;
- complete Cookie headers or cookie values;
- raw browser session secrets;
- raw CSRF tokens;
- verifier secrets;
- request or response bodies containing credentials.

## Persistence ownership

Direct SQLite access remains repository-owned. Services orchestrate bounded transactions through repository/database abstractions. Accountability tables are append-only and protected by update/delete rejection triggers.

## Real-runtime acceptance

```text
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS
accepted_runtime_head=4762583d5b5170866838ed9f03b928adbf39f99e
source_ci_run_number=6884
source_ci_run_id=30752351218
daemon_sha256=488edade196cedfb92d5393a8725b39c5f5cdfd3265e2b15bab6aadfbe7ef5f5
loader_sha256=3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
configuration_sha256=8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b
runtime_report_sha256=bf165416b5ad041f44b2514182dac582a7f1060bf1ae8cc584964f3fc5a98bdf
evidence_directory=/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51
```

The final pass proved protected HTTP 200 success and deterministic HTTP 500 failure event pairs, context continuity, secret-free evidence, isolated database use, production database preservation, removed systemd override and active final service.

## Productization direction

Legacy Basic and Managed Basic prove compatibility and persistent credential handling; they are not the final human-account bootstrap UX.

The target fresh-install lifecycle has no permanent default administrator credential. A fresh server is claimed through a bounded local operator bootstrap that issues only short-lived setup material, creates the first administrator inside the persistent Suite identity authority and permanently invalidates the bootstrap material. Debian/Ubuntu packaging and source installation use the same identity lifecycle rather than package-specific user storage.

Password-based Human User accounts, when implemented, persist only modern salted one-way verifier material. Plaintext or reversible passwords are never persistent configuration.

Operator recovery is a local, short-lived and accountable flow. Manual SQLite editing is not a supported product recovery workflow.

Living-room/client pairing similarly reuses persistent Device/Credential/Session authority. Pairing approval never implies administrator permission, and QR/human setup codes are short-lived approval material rather than reusable device credentials.

These are productization ownership/safety requirements. Exact Human User/Profile mapping, bootstrap endpoint/schema, setup-code lifecycle and device-pairing credential shape remain bounded implementation decisions for P1/P3/P6 and are not invented by Phase 62.

See [Platform Productization Roadmap](../planning/platform-productization-roadmap.md).

## Compatibility-retirement decision

Immediate removal of Legacy Basic is not deployment-ready because `legacy-basic` remains the code default and packaged configuration does not mandate operator migration to `enforced`.

Phase 62 therefore closes with Legacy Basic retained as an explicitly transitional compatibility mode. Future retirement requires a separate deployment-migration contract covering rollout, recovery and compatibility impact.

## Deferred capabilities

Not required for Phase 62 and now owned by explicit post-69 productization work where applicable:

- protected audit read/export/filter/redaction/retention product;
- P1-P3 Human User/account and generic actor/credential/grant/role administration;
- P4 Household/Profile model;
- P5 Profile-scoped playback/history state;
- P6 product-visible device/session management and pairing;
- P7 content scopes beyond established backend scope;
- native/service credential enrollment before a concrete consumer exists;
- universal revision/idempotency/durable-operation infrastructure;
- transactional Outbox.

Completed later phases remain authoritative for their own accepted runtime foundations; this document does not reopen Phase 62.

## Related documents

- [Phase 62 Final Closeout](../development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](../development/phase-62-slice-2x-runtime-closeout.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Current Architecture State](../development/current-architecture-state.md)
- [Strict Roadmap](../planning/roadmap.md)
