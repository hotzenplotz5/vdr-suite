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
Active; first coherent security/identity slice implemented on its phase branch.
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

Implemented:

- `RequestSecurityContext` with actor, device, session, grants, request ID and correlation ID;
- centralized `AuthorizationService` with exact/wildcard permission and backend scope decisions;
- explicit legacy compatibility and fail-closed enforced modes;
- server-side protection of `POST /api/vdr/remote/actions` with `remote.control`;
- append-only accountability persistence before dispatch;
- stable 400/401/403/503 security errors without credential reflection;
- focused authorization, configuration, repository, HTTP-gate and architecture tests.

The existing `BackendAccessPolicy` remains a separate backend-state guard. It does not replace actor authorization.

## Open Phase 62 limitations

The platform still lacks persistent user/device/session/role/grant lifecycle, production authentication, complete mutation migration, universal revision/idempotency, mutation outcome/outbox delivery, protected audit query/retention and repository-wide security acceptance.

Phase 62 remains active and incomplete.

## Pull request truth

- PR #115 is merged and defines current Remote runtime truth.
- PR #113 is closed as superseded by #115.
- PR #112 remains an open old-base Draft and is not current runtime truth.
- PR #116 remains an open, mergeable Draft. Its proposed ADR-0051 is not on `main` and is consumer context only.

## Immediate implementation focus

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

Continue route-by-route authorization, identity/session persistence, mutation preconditions, idempotency, complete accountability and outbox work without advancing Phase 63-67 runtime.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Phase 62 Slice 1](phase-62-security-identity-foundation-slice-1.md)
- [Current Architecture State](current-architecture-state.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)
- [Architecture Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](../planning/parity-audit-and-frontend-gap-roadmap.md)
