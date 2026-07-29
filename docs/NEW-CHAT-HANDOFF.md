# VDR-Suite New Chat Handoff

## Purpose

This is the canonical entry point for every new VDR-Suite chat.

Read this file before repeating repository-wide analysis or real-runtime
acceptance. A changed chat alone is not a reason to start over. Trust completed
items marked **VERIFIED** unless a relevant repository, binary, configuration,
database, routing, or behaviour fingerprint changed.

Detailed non-secret runtime evidence is preserved in
[Phase 62 Runtime Evidence](development/phase-62-runtime-evidence.md). The
repository implementation contract for the current slice is preserved in
[Phase 62 Slice 3A Public Origin and Base Path](development/phase-62-public-origin-base-path.md).

## Canonical project entry points

- [Current project truth](CURRENT.md)
- [Strict roadmap](planning/roadmap.md)
- [Phase map](planning/phase-map.md)
- [Parity and frontend gap roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
- [ADR index](adr/index.md)
- [Completed phases](development/completed-phases.md)

## Stable project position

```text
Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Completed post-phase platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)

Current active runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation
```

Do not resurrect Phase 61, PR #110, or PR #111 as current implementation work.

## Mandatory resume rule

Recheck only volatile state:

1. local branch, HEAD, upstream, and clean/dirty state;
2. remote branch head and divergence;
3. PR Draft/head/base/CI state;
4. installed daemon fingerprint when runtime work is planned;
5. active Nginx fingerprints when routing work is planned;
6. files changed since this handoff in the active slice.

Do not repeat completed browser-session acceptance unless the code, installed
binary, configuration, database contract, active routing, or tested behaviour
changed. Avoid HTTP probes during a read-only check when they would create
security or accountability records.

## Current active workstream

```text
Repository:
hotzenplotz5/vdr-suite

Real yaVDR checkout:
/home/yavdr/vdr-suite-phase62

Local branch:
phase62-pr117

Remote branch:
phase-62-security-identity-foundation

Pull request:
#117
feat(security): establish Phase 62 identity and authorization foundation

Accepted real-runtime security baseline:
d0500da0aa43b11b57eca23d5d757d070b2beb98

Slice 3A repository implementation commit:
8007b77afe55a750096e75baeb8772f2f28227dd

Base branch and recorded base SHA:
main
cb77ff66e11dca7db2eafa36525762dcde35102d
```

The current branch head is a descendant of the Slice 3A implementation commit
because this handoff itself may receive follow-up documentation repairs.

## Pull request rules

PR #117 must remain open and Draft. Do not mark it ready, merge it, enable
auto-merge, rewrite its branch, or mutate review state without explicit
approval.

The PR description still contains **STALE** wording saying ordinary-route
installed-runtime browser-session acceptance is outstanding. Runtime evidence
in this handoff and the dedicated evidence document is newer.

## Real yaVDR security baseline

**VERIFIED on 2026-07-29 for the accepted security baseline:**

```text
Daemon unit:
vdr-suite-daemon.service

Installed executable:
/usr/sbin/vdr-suite-daemon

Recorded pre-acceptance backup:
/usr/sbin/vdr-suite-daemon.pre-d0500da0-20260729-062203

Daemon configuration:
/etc/default/vdr-suite-daemon

Database:
/var/lib/vdr-suite/vdr-suite.db

Listener:
0.0.0.0:18080
```

At the acceptance point:

- the daemon was active;
- the installed binary matched the accepted security branch build;
- ordinary browser-session authentication was accepted on real yaVDR;
- the database remained structurally consistent;
- temporary acceptance lifecycle data was cleaned up.

PIDs, service timestamps, working-tree state, active hashes, and active Nginx
fingerprints are volatile. Recheck them before mutation, but do not repeat the
whole security acceptance suite when they are unchanged.

## Completed Phase 62 security work

**VERIFIED and real-runtime accepted:**

- persistent actors, devices, sessions, and credentials;
- managed Basic verifier;
- browser-session credential persistence;
- atomic browser-session issuance;
- exact browser-session issue and logout lifecycle routes;
- ordinary-route browser-cookie authentication;
- cookie authentication precedence;
- malformed and duplicate cookie rejection;
- revoked-cookie rejection;
- fail-closed browser mutation behaviour;
- accepted and denied accountability evidence;
- logout and atomic lifecycle revocation;
- replay rejection after revocation;
- database verification and test-data cleanup.

Do not relabel these points as pending merely because a new chat starts.

## Secret restrictions

The managed Basic plaintext password used during earlier acceptance is no
longer available.

Therefore:

- never print Authorization headers;
- never print browser cookies or CSRF tokens;
- never expose password hashes or production verifier material;
- never inspect `/proc/.../environ`;
- do not rotate or reprovision credentials without explicit approval;
- do not claim authenticated public-origin acceptance can be repeated until an
  approved credential path exists.

## Public routing truth

### yaVDR ownership

**VERIFIED:**

- public `/api/*` belongs to the yaVDR Uvicorn backend through
  `/run/yavdr-backend/uvicorn.sock`;
- yaVDR already exposes `/api/vdr/*`;
- VDR-Suite internal daemon routes also use `/api/vdr/*`;
- VDR-Suite must never take over public root `/api/`.

### Temporary Suite lifecycle exposure

The recorded temporary manual snippet is:

```text
/etc/nginx/snippets/vdr-suite-browser-sessions.conf
```

It exposes only the two exact browser-session lifecycle POST routes. It is
acceptance plumbing, not the target public contract.

### Frontend publication

The Suite frontend installation owner is:

```text
/usr/share/vdr-suite/web/frontend
```

The daemon serves it through its own route and asset tables. Public yaVDR
`/frontend/*` resolves through the yaVDR application or SPA fallback;
`/var/www/html/frontend` is not the installed Suite owner.

## Phase 62 Slice 3A repository state

```text
Phase 62 Slice 3A
Public Origin and Base-Path Integration
```

**IMPLEMENTED IN THE REPOSITORY at commit `8007b77a`:**

- repository-managed Nginx server-context snippet;
- canonical `/vdr-suite` redirects;
- prefix-stripping proxy to `127.0.0.1:18080`;
- forwarding headers and SSE-safe proxy settings;
- cookie path rewrite from `/` to `/vdr-suite/`;
- immutable frontend public-URL resolver;
- relative initial HTML bootstrap assets;
- URL adaptation for canonical Suite API, SSE, script, logo, artwork, Remote,
  DOM, and CSS URL sinks;
- direct-daemon compatibility;
- daemon asset-table and runtime-install integration;
- separate inactive Nginx snippet installation;
- Node, architecture, ownership, and install-staging tests;
- focused implementation documentation.

### Implemented public contract

```text
/vdr-suite/
/vdr-suite/frontend/...
/vdr-suite/api/...
/vdr-suite/channel-logos/...
/vdr-suite/recording-artwork/...
```

Internal daemon paths remain unchanged:

```text
/frontend/...
/api/...
/channel-logos/...
/recording-artwork/...
```

Canonical redirects are:

```text
/vdr-suite          -> /vdr-suite/frontend/
/vdr-suite/         -> /vdr-suite/frontend/
/vdr-suite/frontend -> /vdr-suite/frontend/
```

The Nginx snippet contains no root `/api` location, no Uvicorn socket reference,
and no `sub_filter` response rewriting.

### Slice 3A files

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

### Repository validation

The isolated pre-push checks passed:

- JavaScript syntax validation;
- public-URL Node runtime tests;
- public-origin architecture contracts;
- Python syntax validation;
- isolated frontend and Nginx install staging with mode `0644`.

GitHub Actions run 6475 reached the documentation gate and failed because the
previous handoff replacement omitted mandatory canonical links and historical
status markers. The Slice 3A product diff itself was not reported as failing;
downstream jobs were skipped after the documentation gate. The current handoff
restores those required entrypoints and markers. Verify the newest CI run before
continuing.

## Runtime activation boundary

The repository implementation has **not** been installed or activated on the
real yaVDR runtime.

No active Nginx site, service, package, database, credential, or production file
was changed by the repository implementation.

The repository target `install-runtime` installs the frontend resolver but does
not install or activate Nginx. The full `install` target or explicit
`install-nginx` target installs the snippet as an inactive file; the active site
must include it separately.

A later runtime activation requires a separate explicit approval and an exact
reviewed procedure covering:

- backups and hashes;
- daemon/frontend installation;
- Nginx snippet installation;
- active-site include change;
- removal of the temporary exact lifecycle include;
- `nginx -t`;
- explicitly approved service reloads;
- unauthenticated route-provenance checks;
- authenticated public-origin acceptance through an approved credential path;
- rollback.

## Still open after Slice 3A repository implementation

- green CI for the final Slice 3A branch head;
- synchronization of the real local checkout to the remote branch;
- separate real-runtime installation and public-origin acceptance;
- frontend login/logout UI;
- frontend memory-only CSRF handling;
- business-mutation CSRF integration;
- persisted roles and grants;
- broader permission migration;
- refresh, idle expiry, cleanup, and concurrent-session policy;
- protected user and credential administration;
- completion accountability and transactional outbox.

## Exact next action

1. verify the CI run for the handoff repair descendant of `8007b77a`;
2. if CI is green, fast-forward `/home/yavdr/vdr-suite-phase62` only after
   checking its working tree;
3. present the exact runtime installation and rollback procedure;
4. obtain explicit approval before any real yaVDR or active Nginx mutation.

## Maintenance rule

Before ending any Phase 62 session:

1. update this file when repository, PR, runtime, routing, or next-action truth
   changed;
2. append durable non-secret evidence to the runtime-evidence document;
3. mark stale statements explicitly;
4. record the exact next permitted action and approval boundary;
5. keep secrets and production verifier material out of both documents.

A future chat should need minutes, not half its context window, to resume.
