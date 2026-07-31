# VDR-Suite Current Project Status

## Current verified position

Baseline remains `origin/main`
`cb77ff66e11dca7db2eafa36525762dcde35102d`, with active work in Draft
PR #117.

```text
Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Current active runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Repository and runtime accepted through:
Slice 2H - Channel Move security migration

Accepted source/runtime head:
2e0b31f671edf18393d7d48ea6e15697fc3a044d

GitHub Actions:
VDR-Suite CI #6559, successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30627974107

Installed daemon SHA-256:
ff7582b6fdb6a2faa7d0e29f6795ad634ea76d95a42280a6140e005e249cbf52

Installed deferred runtime loader SHA-256:
e4860a2b7c613919f3a084fc625f398bd5f339191ae48133cfc76431c0189ca9
```

Phase 61 remains completed. Phase 62 remains active and incomplete. Phase 63-67
runtime has not been advanced.

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

- `RequestSecurityContext` with actor, device, session, credential, grants, request ID and correlation ID;
- centralized `AuthorizationService` with exact/wildcard permission and backend scope decisions;
- explicit legacy compatibility and fail-closed enforced modes;
- server-side protection of `POST /api/vdr/remote/actions` with `remote.control`;
- append-only accountability persistence before dispatch;
- stable security errors without credential reflection;
- real yaVDR evidence for anonymous denial, invalid-credential denial and authenticated Browser Remote dispatch.

`BackendAccessPolicy` remains an independent backend-state guard and never replaces actor authorization.

## Phase 62 Slice 2

### Lifecycle persistence — implemented and real-runtime accepted

- additive `security_actors`, `security_devices`, `security_sessions` and `security_credentials` tables;
- compatibility bootstrap without storing Authorization secrets;
- persisted actor/device/session/credential ownership bindings;
- request-time `PersistentIdentityResolver` before authorization;
- persisted expiry and revocation enforcement;
- restart-safe bootstrap that never reactivates revoked records;
- real yaVDR revoke/restore evidence without daemon restart.

### Managed Basic verifier — implemented and real-runtime accepted

- optional separate managed actor/device/session/credential provisioning;
- `security_basic_credential_verifiers` login-to-credential binding;
- yescrypt or SHA-512 crypt one-way password hashes;
- strict bounded Basic parsing and thread-safe `crypt_r` verification;
- no managed identity or permission enabled by default;
- startup failure for partial, unsupported or conflicting configuration;
- no legacy bypass for managed identities;
- real yaVDR positive GET, wrong-password 401, unmigrated Timer 503, migrated Remote 200 and accountability evidence.

### Browser-session verifier — implemented and CI validated

- additive `security_browser_session_credentials` table;
- actor/device/session/browser-credential/issuing-credential bindings;
- non-secret lookup token plus separate one-way session and CSRF hashes;
- bounded `vdr_suite_session` parsing and duplicate rejection;
- independent `X-CSRF-Token` verification;
- active, expiry and revocation outcomes;
- positive and negative cookie/CSRF tests;
- architecture guards against raw browser-secret persistence.

### Atomic browser-session issuance — implemented and CI validated

- Linux `getrandom(2)` as production CSPRNG with full-read and `EINTR` handling;
- independent 128-bit token/session/credential IDs;
- independent 256-bit Base64url session and CSRF secrets;
- independent SHA-512 crypt salts and `rounds=10000` verifier hashes;
- bounded lifetime: 5 minutes minimum, 8 hours default, 24 hours maximum;
- actor/device/issuing-credential revalidation inside `BEGIN IMMEDIATE`;
- atomic creation of session, browser credential and verifier rows;
- rollback on validation, collision, repository or commit failure;
- forced-collision test proving no intermediate lifecycle rows survive;
- move-only issuance result whose cookie and CSRF buffers are explicitly wiped.

### Isolated HTTP login/logout lifecycle — implemented and CI validated

- `POST /api/security/browser-sessions` exchanges an already authenticated legacy/managed Basic context for a browser session;
- no plaintext-password JSON login contract;
- successful login returns `200`, one-time `csrfToken`, expiry and request ID under `no-store`/`no-cache`;
- session secret is delivered only through `Set-Cookie`;
- cookie uses `Path=/`, `Max-Age=28800`, `HttpOnly`, `Secure`, `SameSite=Strict` and no `Domain`;
- `POST /api/security/browser-sessions/logout` accepts only a valid browser cookie plus matching `X-CSRF-Token`;
- missing/wrong CSRF fails with `403 csrf_validation_failed` before revocation;
- verifier, canonical session and browser credential are revoked in one transaction;
- successful logout returns `204` and an expired hardened cookie;
- dedicated pre-dispatch accountability records `session.issue.self`, `session.revoke.self`, authentication denial and CSRF denial;
- exact-route tests prove browser cookies do not authenticate ordinary GETs or Remote/application POSTs.

The browser-session lifecycle and ordinary-route browser authentication are
installed and real-runtime accepted. Browser cookies have strict precedence
when presented, persisted actor grants are resolved for browser contexts and
unmigrated browser POST routes remain fail-closed.

## Cumulative Slice 2C-2H acceptance

The accepted installed runtime now includes:

- persisted browser actor grants and unavailable-store recovery;
- ordinary-route browser-cookie authentication with no Basic fallback;
- Webfrontend login/logout and memory-only CSRF state;
- fixed exact-backend `role.admin` and `role.read-only`;
- browser-CSRF migration for Remote;
- browser-CSRF migration for Timer create/update/delete;
- browser-CSRF migration for both Channel Move aliases;
- exact permission, backend scope and fixed-role accountability;
- fail-closed query/trailing-slash and unrelated-route boundaries;
- mutation-free Channel Move runtime acceptance.

Slice 2H completed with 22 controlled Channel Move security requests,
`real_channel_moves=0`, the acceptance browser session revoked, grants restored,
SQLite healthy and the daemon active.

## Open Phase 62 limitations

The platform still lacks:

- remaining Recording, SearchTimer and administrative mutation migration;
- explicit safe classification for validation, preview and planning POSTs;
- completion/outcome accountability and transactional coupling/outbox;
- refresh, idle expiry, cleanup and concurrent-session policy;
- protected identity, credential, role and grant administration;
- native/service credential lifecycle;
- generic persisted role definitions beyond the fixed catalogue;
- common revision, idempotency and operation contracts;
- protected audit query/export/retention;
- final compatibility-retirement and Phase 62 closeout evidence.

## Pull request truth

- PR #115 is merged and defines current Remote runtime truth.
- PR #113 is closed as superseded by #115.
- PR #112 remains an old-base Draft and is not current runtime truth.
- PR #116 remains an open Draft; proposed ADR-0051 is consumer context only.
- PR #117 is the active Phase 62 Draft, accepted through Slice 2H, and must not be merged, marked ready or auto-merged by this workflow.

## Immediate implementation focus

First complete and validate the Slice 2H documentation closeout. Then inspect
the remaining mutating/stateful POST inventory and plan exactly one next Phase
62 route family.

Do not combine the next route-family implementation with generic
administration, native/service credentials, idempotency, later runtime phases
or a PR-ready transition.

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
