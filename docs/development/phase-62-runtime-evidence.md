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

**VERIFIED on 2026-07-31:**

```text
Repository: hotzenplotz5/vdr-suite
Pull request: #117
PR title: feat(security): establish Phase 62 identity and authorization foundation
Head branch: phase-62-security-identity-foundation
Accepted source/runtime head: 2e0b31f671edf18393d7d48ea6e15697fc3a044d
Base branch: main
Recorded base SHA: cb77ff66e11dca7db2eafa36525762dcde35102d
PR state: open, Draft, not merged
Slice 2H CI: run 6559, completed successfully
CI URL: https://github.com/hotzenplotz5/vdr-suite/actions/runs/30627974107
```

The PR description is stale through Slice 2E.1. This runtime evidence and the
canonical handoff supersede its implementation, CI and remaining-work wording
until PR metadata is explicitly approved for update.

## Real yaVDR baseline

**VERIFIED from completed Slice 2H runtime acceptance on 2026-07-31:**

```text
Checkout: /home/yavdr/vdr-suite-phase62
Local branch: phase62-pr117
Accepted source/runtime head: 2e0b31f671edf18393d7d48ea6e15697fc3a044d

Daemon unit: vdr-suite-daemon.service
Installed executable: /usr/sbin/vdr-suite-daemon
Installed daemon SHA-256: ff7582b6fdb6a2faa7d0e29f6795ad634ea76d95a42280a6140e005e249cbf52
Installed deferred-runtime-loader.js SHA-256: e4860a2b7c613919f3a084fc625f398bd5f339191ae48133cfc76431c0189ca9
Guarded installation backup:
/var/backups/vdr-suite-phase62-slice2h-install-20260731-140438
Runtime-acceptance database backup:
/var/backups/vdr-suite-phase62-slice2h-runtime-20260731-142540
Database: /var/lib/vdr-suite/vdr-suite.db
Listener: 0.0.0.0:18080
```

At the acceptance point:

- the daemon service was active;
- installed files matched the fully tested repository artifacts;
- direct and public loader responses matched the accepted loader fingerprint;
- the controlled browser session was logged out and revoked;
- all temporary grant rows were restored exactly;
- SQLite `PRAGMA quick_check` returned `ok`;
- no real channel order was changed.

Process IDs, service timestamps and future working-tree state remain
**VOLATILE**.

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

### Persisted browser actor grants

**VERIFIED on the real installed yaVDR runtime:**

- browser authentication resolves an empty grant set successfully;
- an empty grant set permits authenticated ordinary reads but grants no
  business-mutation permission;
- active actor grants are loaded from
  `security_actor_permission_grants`;
- grants for separate backend scopes remain independently revocable;
- browser grants are not copied from legacy Basic, managed Basic or the issuing
  credential;
- active grants do not bypass `security_policy_not_migrated`;
- unavailable grant persistence returns
  `503 permission_grants_unavailable`;
- restoring persistence restores ordinary access for the same session;
- acceptance grant rows were removed after the pass.

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

## Fixed-role, Timer and Channel Move acceptance

**VERIFIED on the installed real runtime through Slice 2H:**

- `role.admin@<backend-id>` expands only to the explicit protected mutation
  catalogue;
- exact-scope `role.read-only@<backend-id>` denies protected mutations before
  direct or Admin grants;
- wildcard role rows do not become concrete backend-role assignments;
- Timer create/update/delete routes use their canonical permissions and
  browser-CSRF contract;
- both Channel Move aliases use `channels.move@<backend-id>`;
- query-string variants are classified with their exact base route;
- trailing-slash variants remain fail-closed;
- missing and invalid CSRF are rejected before permission evaluation;
- direct permission, wrong-scope, Admin, Read-only and cross-backend cases are
  enforced;
- authorized unknown-backend requests stop at backend policy;
- accountability uses canonical non-secret `channels.move` fields;
- logout revokes the acceptance browser session and replay is denied.

The Channel Move pass issued 22 controlled security requests. Every successful
controller request used `dryRun:true`.

```text
channel_move_requests=22
real_channel_moves=0
browser_session_active=0
grants_restored=yes
sqlite_quick_check=ok
service_state=active
```

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

- public root `/api/*` remains owned by the yaVDR backend;
- VDR-Suite does not take over that namespace;
- VDR-Suite is published below `/vdr-suite/`;
- the daemon retains its unchanged internal `/api/...` paths.

### Current Suite public origin

**VERIFIED installed contract:**

```text
/vdr-suite          -> 308 /vdr-suite/frontend/
/vdr-suite/         -> 308 /vdr-suite/frontend/
/vdr-suite/frontend -> 308 /vdr-suite/frontend/

Public Suite status:
  /vdr-suite/api/vdr/status

Internal daemon status:
  /api/vdr/status
```

The active repository-managed Nginx snippet is:

```text
/etc/nginx/snippets/vdr-suite.conf
```

The public prefix is stripped before proxying to `127.0.0.1:18080`. Cookie
paths are rewritten from `/` to `/vdr-suite/`. Public root `/api/*` remains
untouched.

The installed frontend owner remains:

```text
/usr/share/vdr-suite/web/frontend
```

Slice 3A public-origin activation and authenticated lifecycle acceptance are
complete. They must not be repeated unless a directly relevant routing,
frontend, daemon or configuration fingerprint changes.

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

Repository and installed-runtime acceptance are complete through Slice 2H.

Still open in Phase 62:

- remaining Recording, SearchTimer and administrative mutation migration;
- explicit safe classification for validation, preview and planning POSTs;
- completion/outcome accountability and transactional coupling/outbox;
- refresh, idle timeout, cleanup, concurrent-session policy and recovery;
- protected identity, credential, grant and role administration;
- native/service credential enrollment, rotation and revocation;
- generic role definitions and assignments beyond the fixed catalogue;
- common revision, idempotency and operation lifecycle;
- protected audit reads, export, redaction and retention;
- compatibility-retirement readiness and final Phase 62 closeout.

The next route family has not been selected by this documentation closeout.
Inspect the remaining POST inventory and plan exactly one bounded family before
implementation.

### Credential boundary

The managed Basic plaintext password used for earlier acceptance is not
available. Do not rotate or reprovision credentials implicitly. Existing
accepted credential configuration must remain unchanged unless separately
approved.

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
| Public-origin acceptance | No | Only after directly relevant routing or frontend mutation | No, unless its fingerprint changed |

## Maintenance rule

At the end of every Phase 62 work session:

1. update the canonical handoff if repository, PR, runtime, routing, or next-step
   truth changed;
2. append only durable non-secret evidence here;
3. record exact fingerprints when they decide whether acceptance must repeat;
4. mark stale statements explicitly;
5. record the next permitted action and approval boundary.

## Slice 2J SearchTimer Create Runtime Acceptance

**VERIFIED on 2026-08-01 at
`7a3c8a1a3e0e6902b6ec0fea8a48bd69428c93e4`:**

```text
Installed daemon SHA-256:
ccfc7c3c81300562da07b29a42b71e778439e805995abe7718dc702363a91a4c

Installed deferred loader SHA-256:
a43f04673bb85a4dac21b2918744ae0bca554367c4942a125886c301e3ff51e7

Verified backup:
/var/backups/vdr-suite-phase62-slice2j-20260801T105140Z-7a3c8a1a3e0e
```

The accepted protected aliases are:

```text
POST /api/searchtimers
POST /api/vdr/searchtimers
```

All authorized runtime requests used `{}` and stopped before the SearchTimer
command executor with `searchtimer name is required`.

The accepted evidence recorded 17 SearchTimer authorization events, including
two CSRF denials, two permission denials, four backend-scope denials, two
read-only denials and seven authorized dispatches. Both trailing-slash variants
failed closed.

No SearchTimer was created. The browser session was revoked, replay was denied,
target grants matched the backup state, the service process remained unchanged,
the database passed quick and foreign-key checks and accountability contained no
credential, cookie or CSRF secret.
