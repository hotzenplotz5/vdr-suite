# VDR-Suite Current Project Status

## Current verified position

Baseline reconciled on 2026-07-27 against `origin/main` commit `cb77ff66e11dca7db2eafa36525762dcde35102d`, the merge of PR #115.

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

Current Phase 62 status:
Active; Slice 1 is real-runtime validated and the persistent lifecycle foundation of Slice 2 is implemented on its Draft branch.
```

## Stable implemented scope

- daemon-owned SQLite, migrations, repositories and backend registry;
- backend-scoped snapshots, change feed, SSE foundation and server-enforced read-only mode;
- channels, EPG timeline, channel-day view, Recording and Timer read paths;
- Recordings 2 with metadata, people, artwork, Genres and guarded actions;
- SearchTimer list, preview, validation and controlled mutation foundations;
- persistent Recording/EPG metadata, people and Genre read models;
- query-only provider-free Genre and global-search GET paths;
- backend-neutral RemoteAction and LiveOverlay paths;
- isolated remote pressed-state and duplicate-dispatch guard from PR #110;
- backend-scoped global search from PR #111;
- merged 360×1220 PNG Remote and extended help/navigation/REC behaviour from PR #115;
- modular frontend Client API ownership and install/runtime staging.

## Phase 62 Slice 1

Implemented and real-runtime validated:

- `RequestSecurityContext` with actor, device, session, grants, request ID and correlation ID;
- centralized `AuthorizationService` with exact/wildcard permission and backend scope decisions;
- explicit legacy compatibility and fail-closed enforced modes;
- server-side protection of `POST /api/vdr/remote/actions` with `remote.control`;
- append-only accountability persistence before dispatch;
- stable 400/401/403/503 security errors without credential reflection;
- focused authorization, configuration, repository, HTTP-gate and architecture tests;
- real yaVDR evidence for anonymous denial, invalid-credential denial and authenticated Browser Remote dispatch.

The existing `BackendAccessPolicy` remains a separate backend-state guard. It does not replace actor authorization.

## Phase 62 Slice 2 persistence foundation

Implemented on the Draft branch:

- additive `security_actors`, `security_devices`, `security_sessions` and `security_credentials` tables;
- server-owned compatibility identity bootstrap without storing the Authorization secret;
- persisted actor/device/session/credential bindings;
- request-time `PersistentIdentityResolver` before authorization;
- persisted session and credential expiry/revocation enforcement;
- restart-safe `INSERT OR IGNORE` bootstrap that does not reactivate revoked records;
- `credential_expired` and `credential_revoked` error decisions;
- repository, resolver and HTTP-gate lifecycle tests.

The current persisted credential row is metadata for the transitional Basic credential. It is not a password hash, bearer token, production session or final authentication mechanism.

## Open Phase 62 limitations

The platform still lacks:

- secure per-user/service credential issuance and verification;
- browser cookie/CSRF, native token, refresh, logout, recovery and protected lifecycle-management contracts;
- persisted roles, grants and backend scopes;
- complete mutation and sensitive-read permission migration;
- universal revision and idempotency contracts;
- mutation outcome and transactional-outbox delivery;
- complete authentication/security event catalogue and protected audit query/retention;
- full failure injection and real-runtime closeout across all migrated routes.

Phase 62 remains active and incomplete.

## Pull request truth

- PR #115 is merged and defines current Remote runtime truth.
- PR #113 is closed as superseded by #115.
- PR #112 remains an open old-base Draft and is not current runtime truth.
- PR #116 remains an open, mergeable Draft. Its proposed ADR-0051 is not on `main` and is consumer context only.
- PR #117 is the active Phase 62 Draft and must not be merged or auto-merged by the implementation workflow.

## Immediate implementation focus

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

Complete the remaining secure issuance and lifecycle-management part of Slice 2, then continue persisted roles/grants and route-by-route authorization without advancing Phase 63-67 runtime.

### Preferred edit path for new chats

Prefer direct GitHub repository updates for existing files when the complete current file has been fetched and the edit can be reviewed as a bounded diff.

Use local edits first only when the change requires:

- broad generated-file or binary work;
- local compilation, formatting or repository-wide transformations that cannot be expressed safely as bounded connector edits;
- a workaround because the GitHub connector blocks a file operation.

Never replace a complete existing file from a truncated fetch. Fetch missing ranges first, preserve historical detail through explicit archives when appropriate, and inspect the resulting commit diff before treating an update as correct.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Phase 62 Slice 1](phase-62-security-identity-foundation-slice-1.md)
- [Phase 62 Slice 2](phase-62-security-identity-foundation-slice-2.md)
- [Phase 61 and Performance Closeout](phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](post-phase-61-platform-runtime-closeout.md)
- [Current Architecture State](current-architecture-state.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)
- [Architecture Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](../planning/parity-audit-and-frontend-gap-roadmap.md)
