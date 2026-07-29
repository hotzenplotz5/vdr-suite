# Phase 62 Runtime Evidence

## Purpose

This document preserves durable, non-secret evidence from the real yaVDR
acceptance of Phase 62 identity and browser-session work. It exists so that a
new chat does not repeat completed repository analysis and runtime acceptance.

Read [the canonical new-chat handoff](../NEW-CHAT-HANDOFF.md) first. Consult
this file only when detailed evidence is needed.

Never store plaintext passwords, Authorization headers, cookies, CSRF tokens,
raw session secrets, production hashes, or process environments here.

## Evidence vocabulary

- **VERIFIED**: observed in repository, GitHub, or real-runtime evidence.
- **VOLATILE**: recheck before mutation; it does not invalidate completed
  acceptance merely because time passed.
- **OPEN**: not yet implemented or not yet accepted.
- **STALE**: older wording known to conflict with newer evidence.

## Repository and GitHub baseline

**VERIFIED on 2026-07-29:**

```text
Repository: hotzenplotz5/vdr-suite
Pull request: #117
PR title: feat(security): establish Phase 62 identity and authorization foundation
Head branch: phase-62-security-identity-foundation
Accepted code baseline: d0500da0aa43b11b57eca23d5d757d070b2beb98
Base branch: main
Recorded base SHA: cb77ff66e11dca7db2eafa36525762dcde35102d
Recorded divergence before this documentation update: 167 ahead, 0 behind
PR state: open, Draft, not merged
Recorded CI workflow run: 6469, completed successfully
```

The documentation commits that add this evidence and refresh the handoff are
expected descendants of the accepted code baseline. They do not change product
code or runtime behaviour.

The PR description still contains **STALE** wording that ordinary-route
installed-runtime browser-session acceptance is outstanding.

## Real yaVDR baseline

**VERIFIED from completed runtime acceptance on 2026-07-29:**

```text
Checkout: /home/yavdr/vdr-suite-phase62
Local branch: phase62-pr117
Accepted code baseline: d0500da0aa43b11b57eca23d5d757d070b2beb98

Daemon unit: vdr-suite-daemon.service
Installed executable: /usr/sbin/vdr-suite-daemon
Recorded backup: /usr/sbin/vdr-suite-daemon.pre-d0500da0-20260729-062203
Daemon configuration: /etc/default/vdr-suite-daemon
Database: /var/lib/vdr-suite/vdr-suite.db
Listener: 0.0.0.0:18080
```

At the acceptance point:

- the daemon service was active;
- the installed binary matched the branch build;
- the database remained structurally consistent;
- temporary acceptance lifecycle data was cleaned up.

Process IDs, service timestamps, active file hashes, working-tree state, and
active Nginx fingerprints are **VOLATILE**.

## Completed security acceptance

### Persistent identity lifecycle

**VERIFIED:**

- actor, device, session, and credential persistence;
- managed Basic verifier persistence;
- browser-session verifier persistence;
- request-time expiry and revocation enforcement;
- restart-safe lifecycle state;
- atomic browser-session issuance;
- rollback without intermediate lifecycle rows on issuance failure.

### Browser-session secret handling

**VERIFIED by implementation and tests:**

- independent session and CSRF secrets;
- verifier hashes instead of raw browser secrets in persistence;
- non-secret lookup token;
- one-time CSRF delivery in the login response;
- session-secret delivery only through `Set-Cookie`;
- explicit wiping of sensitive issuance buffers;
- no credential reflection in security errors.

### Exact lifecycle routes

**VERIFIED:**

```text
POST /api/security/browser-sessions
POST /api/security/browser-sessions/logout
```

The issue route exchanges an already authenticated Basic context for a browser
session. It does not accept a plaintext-password JSON login payload.

Logout requires a valid browser cookie and matching CSRF token.

### Ordinary application routes

**VERIFIED on the real installed yaVDR runtime:**

- valid browser sessions authenticate ordinary application requests;
- cookie authentication precedence behaves as designed;
- malformed cookies are rejected;
- duplicate cookies are rejected;
- revoked cookies are rejected;
- valid browser contexts reach centralized authorization;
- invalid or unauthenticated browser mutation contexts fail closed.

This acceptance is complete for the recorded code, binary, configuration, and
database fingerprints. Do not repeat it solely because a chat changed.

### Accountability

**VERIFIED on the real runtime:**

- successful session issuance is recorded;
- successful session revocation is recorded;
- authentication denials are recorded;
- CSRF denials are recorded;
- ordinary-route accepted and denied requests retain the expected actor,
  session, request, and correlation context.

### Logout and revocation

**VERIFIED on the real runtime:**

- CSRF is validated before revocation;
- missing or wrong CSRF does not revoke the session;
- successful logout atomically revokes the browser credential and canonical
  session;
- successful logout expires the hardened cookie;
- replay of the revoked session is denied.

### Database verification and cleanup

**VERIFIED on the real runtime:**

- lifecycle rows reflected issuance and revocation transitions;
- revocation remained persistent;
- database verification found no recorded structural damage;
- temporary acceptance records were cleaned up.

## Cookie contract

**VERIFIED daemon contract:**

```text
Path=/
Max-Age=28800
HttpOnly
Secure
SameSite=Strict
No Domain attribute
```

The planned public-origin proxy must rewrite only the cookie path:

```text
/ -> /vdr-suite/
```

This prevents a Suite session cookie from being sent to the unrelated public
yaVDR `/api` namespace.

## Public routing evidence

### yaVDR ownership

**VERIFIED:**

- public `/api/*` belongs to the yaVDR Uvicorn backend;
- the active upstream uses `/run/yavdr-backend/uvicorn.sock`;
- yaVDR already exposes `/api/vdr/*`;
- VDR-Suite internal daemon routes also use `/api/vdr/*`.

Therefore VDR-Suite must never take over public root `/api/`.

### Temporary Suite lifecycle exposure

**VERIFIED recorded owner:**

```text
/etc/nginx/snippets/vdr-suite-browser-sessions.conf
```

This manual unmanaged snippet exposes only the two exact lifecycle POST routes.
It is temporary acceptance plumbing, not the final public contract.

### Frontend ownership and collision

**VERIFIED:**

```text
Installed Suite frontend: /usr/share/vdr-suite/web/frontend
```

- the daemon serves the installed Suite frontend through explicit routes and
  asset tables;
- `/var/www/html/frontend/index.html` is not the Suite installation owner;
- `/var/www/html/frontend/api/client-api.js` is not the Suite installation
  owner;
- public yaVDR `/frontend/*` resolves through the yaVDR web application or SPA
  fallback rather than publishing the Suite installation.

A dedicated Suite public prefix is required.

## Slice 3A design target

**APPROVED DESIGN TARGET, NOT YET IMPLEMENTED:**

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

Planned Nginx behaviour:

- canonical redirect to `/vdr-suite/frontend/`;
- trailing-slash `proxy_pass` that strips `/vdr-suite`;
- proxy only to `127.0.0.1:18080`;
- forwarded public-prefix context;
- SSE-safe proxy behaviour;
- cookie path rewrite to `/vdr-suite/`;
- no public root `/api` location;
- no Uvicorn socket reference;
- no response `sub_filter`.

Planned frontend bootstrap:

- relative initial CSS, script, favicon, and logo references;
- first-loaded immutable `web/frontend/platform/public-url.js`;
- `window.VdrSuitePublicUrl.basePath`;
- `window.VdrSuitePublicUrl.resolvePath(path)`;
- no silent double-prefixing or acceptance of external/protocol-relative URLs.

## Established frontend URL owners

**VERIFIED repository analysis:**

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

Observed URL sinks include:

- JSON requests assembled from absolute `/api/...` paths;
- direct `EventSource('/api/vdr/live')`;
- direct `fetch()` calls outside the central client wrapper;
- dynamic `/frontend/...` script URLs;
- `/channel-logos/...` paths;
- `/recording-artwork/...` and API artwork paths;
- Remote raster asset paths;
- API-provided artwork copied into CSS `backgroundImage`.

Before implementation, perform one final bounded direct-URL search only. Do
not repeat the repository-wide architecture study.

## Architecture and packaging owners

**VERIFIED:**

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

Current repository packaging does not own a Suite Nginx snippet, and current
architecture checks do not enforce the public-origin contract.

## Remaining work

### Repository Slice 3A

- add the public URL resolver;
- update all URL owners;
- add daemon asset and installation ownership;
- add a repository-managed Nginx snippet;
- add architecture, Node, ownership, and staging tests;
- update focused documentation.

### Separate runtime activation

Requires separate explicit approval for:

- daemon/frontend installation;
- Nginx snippet installation and active include change;
- removal of the temporary exact lifecycle include;
- `nginx -t` and service reload;
- route-provenance and prefix checks;
- cookie-path verification;
- authenticated end-to-end public-origin acceptance through an approved
  credential path;
- rollback if needed.

### Credential blocker

The managed Basic plaintext password used for earlier acceptance is no longer
available. Do not rotate or reprovision credentials implicitly.

## Volatile recheck matrix

| Item | Before repository edit | Before runtime mutation | Repeat full acceptance |
| --- | --- | --- | --- |
| Local branch, HEAD, status | Yes | Yes | No |
| Remote PR, head, base, CI | Yes | Yes | No |
| Installed binary fingerprint | Only if relevant | Yes | Only if changed |
| Daemon PID and start timestamp | No | Yes | Only after relevant restart |
| Active Nginx fingerprints | For routing context | Yes | Only routing checks if changed |
| Database lifecycle state | No for Slice 3A code | Before lifecycle mutation | Only if contract changed |
| Browser-session acceptance | No | No | Only after relevant fingerprint change |
| Public-origin acceptance | Not yet available | Required after activation | Yes, first time |

## Maintenance rule

At the end of every Phase 62 work session:

1. update the canonical handoff if repository, PR, runtime, routing, or next-step
   truth changed;
2. append only durable non-secret evidence here;
3. record exact fingerprints when they decide whether acceptance must repeat;
4. mark stale statements explicitly;
5. record the next permitted action and approval boundary.
