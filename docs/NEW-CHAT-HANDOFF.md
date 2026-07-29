# VDR-Suite New Chat Handoff

## Purpose

This is the canonical entry point for every new VDR-Suite chat. Read it before
repeating repository-wide analysis or real-runtime acceptance. A new chat alone
is not a reason to start over.

Trust completed items marked **VERIFIED** unless a relevant repository, binary,
configuration, database, routing, or behaviour fingerprint changed.

## Canonical reading

- [Current project truth](CURRENT.md)
- [Strict roadmap](planning/roadmap.md)
- [Phase map](planning/phase-map.md)
- [Parity and frontend gap roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture audit gap matrix](planning/architecture-audit-gap-matrix.md)
- [ADR index](adr/index.md)
- [Completed phases](development/completed-phases.md)
- [Phase 61 closeout](development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 runtime closeout](development/post-phase-61-platform-runtime-closeout.md)
- [Phase 62 runtime evidence](development/phase-62-runtime-evidence.md)
- [Slice 3A contract](development/phase-62-public-origin-base-path.md)

## Stable project position

```text
Latest completed runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Historical umbrella track:
Phase 58 - Frontend and Live Parity

Completed platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)

Current active phase:
Phase 62 - Identity, RBAC and Accountability Foundation
```

## Resume rule

Recheck only volatile state:

1. local branch, HEAD, upstream, and clean/dirty state;
2. remote head/divergence and PR Draft/base/CI state;
3. installed daemon fingerprint before runtime work;
4. active Nginx fingerprints before routing work;
5. files changed since this handoff in the active slice.

Do not repeat completed browser-session acceptance unless a relevant
fingerprint changed. Avoid HTTP probes during read-only verification when they
would create security or accountability records.

## Active workstream

```text
Repository: hotzenplotz5/vdr-suite
Checkout: /home/yavdr/vdr-suite-phase62
Local branch: phase62-pr117
Remote branch: phase-62-security-identity-foundation
Pull request: #117
Base: main @ cb77ff66e11dca7db2eafa36525762dcde35102d
Accepted security runtime baseline: d0500da0aa43b11b57eca23d5d757d070b2beb98
Slice 3A implementation commit: 8007b77afe55a750096e75baeb8772f2f28227dd
Final green Slice 3A repository head: 50bc66ba12dc663b77ebd141a85e167666218b09
CI: run 6485, completed successfully
```

PR #117 must remain open and Draft. Do not mark it ready, merge it, enable
auto-merge, rewrite its branch, or mutate review state without explicit
approval.

The PR description contains stale wording that ordinary-route installed-runtime
browser-session acceptance is still pending and still names older CI. The
runtime evidence and this handoff are newer.

## Real yaVDR security baseline

**VERIFIED on 2026-07-29:**

```text
Unit: vdr-suite-daemon.service
Executable: /usr/sbin/vdr-suite-daemon
Backup: /usr/sbin/vdr-suite-daemon.pre-d0500da0-20260729-062203
Configuration: /etc/default/vdr-suite-daemon
Database: /var/lib/vdr-suite/vdr-suite.db
Listener: 0.0.0.0:18080
```

Completed real-runtime acceptance includes persistent identity lifecycle,
managed Basic verification, browser-session issuance/logout, ordinary-route
cookie authentication and precedence, malformed/duplicate/revoked-cookie
denial, fail-closed browser mutations, accountability, atomic revocation,
revoked-cookie replay denial, database checks, and cleanup.

Do not relabel this acceptance as pending because a chat changed.

## Secret restrictions

The earlier managed Basic plaintext password is unavailable. Never print
Authorization headers, cookies, CSRF tokens, password hashes, production
verifier material, or `/proc/.../environ`. Do not rotate or reprovision
credentials without explicit approval.

## Public routing truth

**VERIFIED:** public `/api/*` belongs to yaVDR Uvicorn through
`/run/yavdr-backend/uvicorn.sock`. yaVDR already owns `/api/vdr/*`; VDR-Suite
internal routes also use `/api/vdr/*`. VDR-Suite must never claim public root
`/api/`.

The temporary manual lifecycle snippet is:

```text
/etc/nginx/snippets/vdr-suite-browser-sessions.conf
```

The Suite frontend owner is `/usr/share/vdr-suite/web/frontend`. Public yaVDR
`/frontend/*` is the yaVDR application/SPA fallback, not the Suite frontend.

## Phase 62 Slice 3A repository state

```text
Phase 62 Slice 3A
Public Origin and Base-Path Integration
```

**IMPLEMENTED AND CI-VALIDATED in the repository:**

- public namespace `/vdr-suite/` with frontend, API, logo, and artwork paths;
- internal daemon paths remain unchanged;
- canonical redirects to `/vdr-suite/frontend/`;
- prefix-stripping proxy to `127.0.0.1:18080`;
- forwarded prefix, SSE-safe settings, and cookie path `/vdr-suite/`;
- no root `/api` location, Uvicorn socket, or `sub_filter`;
- immutable `VdrSuitePublicUrl` resolver loaded first;
- relative initial HTML assets and direct-daemon compatibility;
- canonical Suite URL adaptation for fetch, SSE, scripts, DOM, CSS, logos,
  artwork, and Remote assets;
- query and fragment preservation, including encoded slash values in queries;
- daemon asset-table/runtime-install integration;
- separately installed inactive Nginx snippet;
- Node, architecture, ownership, daemon-build, and install-staging tests.

Files:

```text
Makefile
core/http/src/TestHttpServerPaths.inc
docs/development/phase-62-public-origin-base-path.md
mk/public-origin.mk
packaging/nginx/vdr-suite.conf
tools/check_frontend_ownership_contracts.py
tools/check_public_origin_architecture.py
web/frontend/index.html
web/frontend/platform/public-url.js
web/frontend/tests/test_public_url_runtime.js
```

Follow-up validation fixes after `8007b77a`:

- restored mandatory canonical handoff markers;
- normalized the relative bootstrap paths for legacy core ownership contracts;
- limited unsafe encoded-path validation to the pathname so encoded query values
  such as `ARD%2FZDF` remain valid;
- added the corresponding public-prefix regression test.

GitHub Actions run 6485 passed:

- documentation checks;
- strict Make and test inventory;
- frontend regression and ownership contracts;
- public URL runtime tests;
- fast regression tests;
- daemon build;
- packaging and install staging.

## Runtime activation boundary

Slice 3A has **not** been installed or activated on real yaVDR. No active Nginx
site, service, package, database, credential, or production file was changed.

`install-runtime` installs the frontend resolver but does not install or activate
Nginx. Full `install` or explicit `install-nginx` installs an inactive snippet;
the active site must include it separately.

Runtime activation requires separate explicit approval and an exact procedure
for backups/hashes, frontend installation, snippet installation, active-site
include replacement, removal of the temporary lifecycle include, `nginx -t`,
approved reloads, route-provenance checks, authenticated acceptance through an
approved credential path, and rollback.

## Still open

- synchronization of the real checkout to the final green repository head;
- separately approved runtime installation and public-origin acceptance;
- frontend login/logout and memory-only CSRF handling;
- business-mutation CSRF integration;
- roles, grants, broader permission migration, lifecycle policy, protected
  administration, completion accountability, and transactional outbox.

## Exact next action

1. verify the real checkout is clean and fast-forward it to
   `50bc66ba12dc663b77ebd141a85e167666218b09`;
2. verify the resulting local HEAD and branch tracking only;
3. present the exact runtime installation and rollback procedure;
4. obtain explicit approval before real yaVDR or active Nginx mutation.

## Maintenance rule

Update this file whenever repository, PR, runtime, routing, or next-action truth
changes. Preserve durable non-secret evidence, mark stale statements, record the
next approval boundary, and keep all secrets out of the repository.
