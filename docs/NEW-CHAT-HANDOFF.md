# VDR-Suite New Chat Handoff

## Purpose

This is the canonical entry point for every new VDR-Suite chat.

Read this file before repeating repository-wide analysis or real-runtime
acceptance. Detailed Phase 62 evidence is preserved in
[Phase 62 Runtime Evidence](development/phase-62-runtime-evidence.md).

A new chat must trust completed items marked **VERIFIED** unless a relevant
fingerprint changed. A changed chat alone is not a reason to start over.

## Mandatory resume rule

Recheck only volatile state:

1. local branch, HEAD, upstream, and clean/dirty state;
2. remote branch head and divergence;
3. PR Draft/head/base/CI state;
4. installed daemon fingerprint when runtime work is planned;
5. active Nginx fingerprints when routing work is planned;
6. files changed since this handoff in the current slice.

Do not repeat completed browser-session acceptance unless the code, installed
binary, configuration, database contract, active routing, or tested behaviour
changed.

Avoid HTTP probes during a read-only check when they would create security or
accountability records.

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

Accepted runtime code baseline:
d0500da0aa43b11b57eca23d5d757d070b2beb98

Base branch and recorded base SHA:
main
cb77ff66e11dca7db2eafa36525762dcde35102d
```

The current branch contains documentation-only descendants of the accepted
runtime code baseline. Commit `449b7a15b3106bcecc16f1860f3b3140a72767e5`
added the durable Phase 62 runtime-evidence document.

## Recorded GitHub state

**VERIFIED on 2026-07-29 before the handoff documentation commits:**

- PR #117 was open and Draft;
- it was not merged;
- head was `d0500da0aa43b11b57eca23d5d757d070b2beb98`;
- base was `cb77ff66e11dca7db2eafa36525762dcde35102d`;
- the branch was 167 commits ahead and 0 behind the recorded base;
- GitHub reported the PR as mergeable;
- workflow run 6469 completed successfully.

The PR description contains **STALE** wording saying ordinary-route installed
runtime acceptance has not been completed.

The two documentation commits do not change product code, runtime behaviour,
PR Draft state, or merge state.

Do not mark the PR ready, merge it, change runtime, or mutate Nginx without a
separate explicit instruction.

## Real yaVDR runtime baseline

**VERIFIED from completed acceptance on 2026-07-29:**

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
- the installed binary matched the accepted branch build;
- ordinary browser-session authentication was accepted on the real yaVDR
  runtime;
- the database remained structurally consistent;
- temporary acceptance lifecycle data was cleaned up.

PIDs, service timestamps, working-tree state, active hashes, and active Nginx
fingerprints are volatile. Recheck them before mutation, but do not repeat the
whole acceptance suite when they are unchanged.

## Phase 62 security status

### Completed and real-runtime accepted

**VERIFIED:**

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

### Still open

- public-origin and base-path integration;
- frontend login/logout UI;
- frontend in-memory CSRF handling;
- business-mutation CSRF integration;
- persisted roles and grants;
- broader permission migration;
- refresh, idle expiry, cleanup, and concurrent-session policy;
- protected user and credential administration;
- completion accountability and transactional outbox.

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

### Temporary Suite exposure

The recorded temporary manual snippet is:

```text
/etc/nginx/snippets/vdr-suite-browser-sessions.conf
```

It exposes only:

```text
POST /api/security/browser-sessions
POST /api/security/browser-sessions/logout
```

These exact routes are temporary acceptance plumbing, not the target public
contract.

### Frontend publication

The Suite frontend installation owner is:

```text
/usr/share/vdr-suite/web/frontend
```

The daemon serves that frontend through its own route and asset tables.

Public yaVDR `/frontend/*` resolves through the yaVDR application or SPA
fallback. `/var/www/html/frontend` is not the installed Suite owner.

## Current strict slice

```text
Phase 62 Slice 3A
Public Origin and Base-Path Integration
```

### Public contract target

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

Nginx strips `/vdr-suite` and proxies only this namespace to
`127.0.0.1:18080`. Public root `/api/` remains owned by yaVDR.

### Slice 3A includes

- repository-managed Nginx server-context snippet;
- canonical `/vdr-suite` redirects;
- prefix-stripping proxying;
- forwarding headers and SSE-safe behaviour;
- cookie path rewrite to `/vdr-suite/`;
- immutable frontend public-URL resolver;
- relative initial HTML bootstrap assets;
- resolver use for API, SSE, scripts, logos, artwork, and Remote assets;
- daemon asset-table and install integration;
- architecture, ownership, Node, and staging tests;
- focused documentation.

### Slice 3A excludes

- login/logout UI;
- roles, grants, and user administration;
- enabling additional mutations;
- full business-mutation CSRF integration;
- refresh, idle expiry, cleanup, and concurrency policy;
- completion outbox work.

## Selected bootstrap design

Canonical public page:

```text
/vdr-suite/frontend/
```

Planned redirects:

```text
/vdr-suite          -> /vdr-suite/frontend/
/vdr-suite/         -> /vdr-suite/frontend/
/vdr-suite/frontend -> /vdr-suite/frontend/
```

Initial `index.html` assets become relative so one installed document works on
the direct daemon and under the public prefix.

A new first-loaded file:

```text
web/frontend/platform/public-url.js
```

exposes an immutable:

```text
window.VdrSuitePublicUrl.basePath
window.VdrSuitePublicUrl.resolvePath(path)
```

It derives the prefix from its own script URL, preserves queries and fragments,
and rejects schemes, protocol-relative URLs, backslashes, invalid Suite roots,
and accidental double-prefixing.

Nginx response rewriting and `sub_filter` are not part of the design.

## Established frontend URL owners

Do not rediscover this list from scratch:

```text
web/frontend/index.html
web/frontend/api/client-api.js
web/frontend/api/live-remote-client-api.js
web/frontend/platform/deferred-runtime-loader.js
web/frontend/channel-day-program-compat.js
web/frontend/channel-day-program.js
web/frontend/channel-logos.js
web/frontend/epg-cache.js
web/frontend/epg-metadata-detail.js
web/frontend/recordings2-folder-artwork.js
web/frontend/recordings2-person-search-view.js
web/frontend/modules/remote.js
web/frontend/modules/genres.js
```

Before implementation, run only one bounded search for direct URL sinks and
newly added files.

## Architecture and installation owners

```text
core/http/src/TestHttpServerRoutes.inc
core/http/src/TestHttpServerAssets.inc
core/http/src/TestHttpServerPaths.inc
mk/install.mk
mk/local-test-groups.mk
mk/maintenance-tests.mk
tools/frontend_ownership_contracts_core.py
tools/frontend_ownership_contracts_current.py
tools/check_architecture.py
web/frontend/tests/test_remote_runtime.js
docs/development/frontend-architecture.md
docs/development/client-api-frontend-module-boundary-plan.md
```

Planned new files:

```text
packaging/nginx/vdr-suite.conf
web/frontend/platform/public-url.js
tools/check_public_origin_architecture.py
web/frontend/tests/test_public_url.js
```

Exact test naming may be aligned with existing repository conventions.

## Nginx Slice 3A contract

The repository snippet must:

- contain only `/vdr-suite` locations and redirects;
- use trailing-slash `proxy_pass` to strip the prefix;
- proxy to `127.0.0.1:18080`;
- forward the public prefix;
- preserve SSE;
- rewrite cookie path `/` to `/vdr-suite/`;
- contain no public root `/api` location;
- contain no Uvicorn socket reference;
- use no `sub_filter`.

Packaging may install the snippet without activating it.

`install-runtime` must not silently activate or replace Nginx configuration.

## Runtime activation boundary

Repository implementation and runtime activation are separate approvals.

A later activation requires an exact reviewed procedure for:

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

No runtime, Nginx, service, package, database, or credential mutation is
approved by this documentation update.

## Exact next action

The next step is a bounded Slice 3A repository implementation review:

1. verify the documentation-only branch head and PR Draft state;
2. run the final bounded direct-URL search;
3. present the exact product-code file scope and diff plan;
4. obtain explicit approval before editing Slice 3A product code.

## Maintenance rule

Before ending any Phase 62 session:

1. update this file when repository, PR, runtime, routing, or next-action truth
   changed;
2. append durable non-secret evidence to the runtime-evidence document;
3. mark stale statements explicitly;
4. record the exact next permitted action and approval boundary;
5. keep secrets and production verifier material out of both documents.

A future chat should need minutes, not half its context window, to resume.
