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
Accepted code/runtime head: 47adb6577511209bfe7288ce8ce0fbe03b53a94c
Base branch: main
Recorded base SHA: cb77ff66e11dca7db2eafa36525762dcde35102d
PR state: open, Draft, not merged
Persisted browser-grant CI: run 6507, completed successfully
```

The PR description contains stale grant-runtime, CI and open-work wording.
The canonical handoff and runtime checkpoint supersede that wording until PR
metadata is explicitly approved for update.

## Real yaVDR baseline

**VERIFIED from completed runtime acceptance on 2026-07-29:**

```text
Checkout: /home/yavdr/vdr-suite-phase62
Local branch: phase62-pr117
Accepted code/runtime head: 47adb6577511209bfe7288ce8ce0fbe03b53a94c

Daemon unit: vdr-suite-daemon.service
Installed executable: /usr/sbin/vdr-suite-daemon
Installed SHA-256: 652dfc6a29f466fca977d34587db8a39bbc631509b735e02f8dd1942c46088e1
Slice 2C backup: /var/backups/vdr-suite-phase62-slice2c-20260729-171704
Daemon configuration: /etc/default/vdr-suite-daemon
Database: /var/lib/vdr-suite/vdr-suite.db
Listener: 0.0.0.0:18080
```

At the acceptance point:

- the daemon service was active/running with zero restarts;
- the running and installed binary matched the branch build;
- the additive browser-grant schema and index existed;
- no default grant rows existed;
- SQLite integrity and foreign-key checks passed;
- the controlled browser acceptance session was revoked;
- temporary acceptance grants were removed;
- Nginx, frontend and credential configuration were unchanged.

Process IDs, service timestamps and working-tree state remain **VOLATILE**.

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

The persisted browser-grant increment is complete.

Still open in Phase 62:

- permission and safe/mutating classification for every business POST;
- server-side browser CSRF enforcement before applicable business dispatch;
- frontend login/logout and memory-only CSRF handling;
- completion/outcome accountability and transactional coupling/outbox;
- refresh, idle timeout, cleanup, concurrent-session policy and recovery;
- protected role, assignment, grant and credential administration;
- migration of all protected routes away from compatibility;
- compatibility retirement closeout.

The next bounded implementation candidate is
`POST /api/vdr/remote/actions`. It must preserve legacy behaviour while
requiring valid browser CSRF before a browser-authenticated request reaches
Remote authorization or dispatch.

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
