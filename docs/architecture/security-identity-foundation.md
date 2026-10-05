# Security and Identity Foundation

Status: **Phase 62 completed and real-runtime accepted through Slice 2X.**

## Runtime boundary

The server owns authentication, lifecycle resolution, CSRF, authorization and accountability before protected dispatch.

```text
HttpServerRequest
  -> exact browser-session lifecycle gate for issue/logout
  -> general SecurityHttpGate
       -> presented browser cookie with strict precedence
       -> otherwise Human Account password bridge or optional Managed Basic
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

Actor types represent users, services, agents and system work. Human Account password login, Managed Basic and browser sessions construct the same transport-neutral context and pass it through persistent lifecycle resolution.

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

## Authentication mechanisms

### Retired Legacy Basic compatibility

Legacy Basic is no longer a runtime authentication mechanism. The daemon does not parse the former deployment mode, compatibility header, compatibility identity or compatibility grant environment inputs. Stale values preserved in an older defaults file are inert and cannot recreate an authenticated actor.

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

## Compatibility-retirement decision

The guarded real yaVDR migration/rollback acceptance completed successfully on
2026-10-01 before implementation deletion. It proved the supported Human
Account identity survived the complete compatibility transition and that the
deployment could be left in enforced mode without identity drift.

Legacy Basic runtime compatibility is now removed. In particular:

- `SecurityConfiguration` no longer has a Legacy Basic or enforced deployment
  mode;
- `LegacyBasicAuthenticator` is deleted;
- `VDR_SUITE_SECURITY_MODE`, `VDR_SUITE_BASIC_AUTH` and
  `VDR_SUITE_LEGACY_BASIC_*` are not consumed by runtime configuration;
- HTTP and browser-session gates no longer contain Legacy Basic fallback;
- startup no longer provisions a compatibility identity;
- HbbTV authorization no longer injects compatibility grants;
- old package-default lines may survive an upgrade because existing defaults
  files are preserved, but those values are inert.

Human Account browser authentication, persistent browser sessions and optional
Managed Basic remain independent supported authorities. Historical retirement
acceptance evidence remains retained; it is evidence for the migration boundary,
not a currently executable rollback contract.

## Deferred capabilities

Not required for Phase 62:

- protected audit read/export/filter/redaction/retention product;
- generic actor, credential, grant or role administration API/UI;
- native/service credential enrollment before a concrete consumer exists;
- universal revision/idempotency/durable-operation infrastructure;
- transactional Outbox;
- Phase 63-67 runtime.

The generic Human Account / credential / grant administration product surface was deferred from Phase 62, but is now being advanced as the active [Post-Phase-69 Multiuser Productization Workstream](../development/post-phase69-multiuser-workstream.md). This does not reopen Phase 62. MU.5 is completed and ADR-0067 is accepted. MU.0-MU.7 are completed; MU.6D merged as PR #411. MU.7 grant administration is completed with deterministic grant-set revision/ETag, global accounts.grants.view/modify authority, desired-state ensure/revoke, server-owned permission/scope allowlisting and final-admin protection. REAL YAVDR acceptance passed; PR #413 merged. MU.8 implementation is complete through MU.8C. MU.8A merged as PR #415 with separately documented real-runtime acceptance still pending; MU.8B merged as PR #416 after focused yaVDR acceptance; MU.8C merged as PR #417 after focused yaVDR acceptance with PR and post-merge main CI green. MU.9A read-only Account administration UI merged as PR #418 after exact merged-main yaVDR acceptance; PR and post-merge hosted CI are green. MU.9B Account Lifecycle Mutation UI merged as PR #420 after real yaVDR acceptance; PR and post-merge hosted CI are green. The successor MU.9 slice has not yet been selected.

## Related documents

- [Phase 62 Final Closeout](../development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](../development/phase-62-slice-2x-runtime-closeout.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Current Architecture State](../development/current-architecture-state.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Post-Phase-69 Multiuser Productization Workstream](../development/post-phase69-multiuser-workstream.md)
- [ADR-0067 Human Account and Backend Access Administration](../adr/ADR-0067-human-account-backend-access-administration.md)
- [MU.5 Administration Architecture Acceptance](../development/post-phase69-mu5-administration-architecture-acceptance.md)
- [MU.6A Human Account Lifecycle Authority Foundation](../development/post-phase69-mu6-account-lifecycle-foundation.md)
- [MU.6B Public Account Item + Revision/ETag](../development/post-phase69-mu6b-public-account-item.md)
- [MU.6C Public Account Lifecycle Mutation](../development/post-phase69-mu6c-public-account-lifecycle-mutation.md)
- [MU.6D Atomic Account CREATE + Durable Idempotency](../development/post-phase69-mu6d-account-create-idempotency.md)
- [MU.7 Backend Access / Permission Grant Administration](../development/post-phase69-mu7-account-grant-administration.md)
- [MU.8A Safe Credential / Session Metadata](../development/post-phase69-mu8a-credential-session-metadata.md)
- [MU.8B Session Revoke](../development/post-phase69-mu8b-session-revoke.md)
