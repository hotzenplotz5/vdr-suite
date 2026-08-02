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

**VERIFIED on 2026-08-02:**

```text
Repository: hotzenplotz5/vdr-suite
Pull request: #117
PR title: feat(security): establish Phase 62 identity and authorization foundation
Head branch: phase-62-security-identity-foundation
Accepted source/runtime head: e84415fadb2587ff744ff8927f1f0113920ece2f
Canonical documentation closeout head: 45f1cc78d2c98f6db4d039a5ea7189f51bbcf8e9
Base branch: main
Recorded base SHA: cb77ff66e11dca7db2eafa36525762dcde35102d
PR state: open, Draft, not merged
Slice 2V source CI: VDR-Suite CI #6779, completed successfully
Run ID: 30741293079
CI URL: https://github.com/hotzenplotz5/vdr-suite/actions/runs/30741293079
Final closeout CI: pending on the current branch head
```

The PR description is refreshed through Slice 2V by the canonical closeout. The
PR must remain open, Draft and unmerged.

## Real yaVDR baseline

**VERIFIED from completed Slice 2V runtime acceptance on 2026-08-02:**

```text
Checkout: /home/yavdr/vdr-suite-phase62
Local branch: phase62-pr117
Accepted source/runtime head: e84415fadb2587ff744ff8927f1f0113920ece2f

Daemon unit: vdr-suite-daemon.service
Installed executable: /usr/sbin/vdr-suite-daemon
Installed/running daemon SHA-256:
e0b6f6de08527b6af49d526ca0118b14b6fb85ff3335fc607ca1b531cdee5f60
Installed deferred-runtime-loader.js SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
Restored daemon configuration SHA-256:
8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b
Runtime evidence:
/var/backups/vdr-suite-phase62-slice2v-20260802T092139Z-e84415fadb25
Runtime report SHA-256:
0a961fbc8b51158fd4a16aa24fc9afde7dafa9d5272e986a46ec73880c311f86
Database: /var/lib/vdr-suite/vdr-suite.db
Listener: 0.0.0.0:18080
Final service PID: 86549
```

At the acceptance point:

- the daemon service was active;
- the installed and running daemon matched the accepted Phase-62 artifact;
- the deferred loader matched the accepted fingerprint;
- the original daemon configuration was restored exactly;
- the runtime-only systemd drop-in was removed;
- the temporary idle environment was not present in the final unit;
- the isolated acceptance lifecycle was revoked;
- SQLite `PRAGMA quick_check` returned `ok`;
- foreign-key verification returned no rows;
- no VDR domain mutation occurred.

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

Repository, source CI and installed-runtime acceptance are complete through
Slice 2V.

Still open in Phase 62:

- documentation-only Slice-2V closeout CI;
- fresh post-2V gap analysis and bounded next-slice selection;
- physical browser-session cleanup and retention;
- outcome accountability beyond browser lifecycle operations;
- stronger transactional coupling or outbox semantics;
- protected identity, credential, grant and role administration;
- native/service credential enrollment, rotation and revocation;
- generic role definitions and assignments beyond the fixed catalogue;
- common revision, idempotency and operation lifecycle;
- protected audit reads, export, redaction and retention;
- compatibility-retirement readiness and final Phase 62 closeout.

No next implementation slice is selected by the Slice-2V runtime acceptance.

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

## Slice 2U Concurrent Browser-Session Limit Runtime Acceptance

**VERIFIED on 2026-08-02 at
`16ff04a4ba371aad32fc4a38bf82f9c0529c532d`:**

Acceptance marker:

```text
PHASE_62_SLICE_2U_RUNTIME_ACCEPTANCE=PASS
```

Source verification:

```text
VDR-Suite CI #6690
Run ID 30723297375
All five jobs successful
```

Installed runtime:

```text
Daemon SHA-256:
0e3ec0d57f4471804824247f712c2457015cc22ac9576df60d8d77ed8ddb3134

Deferred loader SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Final service PID:
79316
```

The guarded isolated acceptance configured a temporary per-actor limit of `1`
and proved:

- first browser-session issuance returned HTTP 200;
- second same-actor issuance returned HTTP 409
  `browser_session_limit_reached`;
- the denied request created no lifecycle row and emitted no cookie;
- the existing first session remained usable through an ordinary GET;
- logout returned HTTP 204 and released the slot;
- replacement issuance returned HTTP 200;
- replacement logout returned HTTP 204;
- replay of the revoked replacement cookie returned HTTP 401
  `credential_revoked`;
- two successful issue outcomes, one limit-reached failed outcome and two
  successful revoke outcomes were persisted;
- all isolated browser lifecycle rows and the source identity were revoked;
- the original daemon environment was restored byte-for-byte;
- SQLite quick check returned `ok`;
- foreign-key check returned no rows;
- accountability evidence contained no password, Authorization header, cookie
  or CSRF secret;
- the service remained active;
- zero VDR domain mutations occurred;
- automatic rollback was not required.

Durable secret-free evidence:

```text
/var/backups/vdr-suite-phase62-slice2u-20260802T041910Z-16ff04a4ba37/runtime-acceptance-slice2u
```

Runtime report:

```text
/var/backups/vdr-suite-phase62-slice2u-20260802T041910Z-16ff04a4ba37/runtime-acceptance-slice2u/runtime-acceptance-report.txt
```

Runtime report SHA-256:

```text
7c33b06d07beff6b17bad82153ff4a0f1e7c1f5c8d8f972406f8b0b9160f4c89
```

This acceptance closes only the concurrent effective browser-session limit.
Idle expiry, cleanup, refresh, eviction and session administration remain
separate gaps.

## Slice 2V Browser-Session Idle Expiry Runtime Acceptance

**VERIFIED on 2026-08-02 at
`e84415fadb2587ff744ff8927f1f0113920ece2f`:**

Acceptance marker:

```text
PHASE_62_SLICE_2V_RUNTIME_ACCEPTANCE=PASS
```

Source verification:

```text
VDR-Suite CI #6779
Run ID 30741293079
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30741293079
```

Installed runtime:

```text
Daemon SHA-256:
e0b6f6de08527b6af49d526ca0118b14b6fb85ff3335fc607ca1b531cdee5f60

Deferred loader SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Restored configuration SHA-256:
8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b

Final service PID:
86549
```

The guarded isolated acceptance configured a temporary idle timeout of `300`
seconds and proved:

- an ordinary browser-authenticated GET returned HTTP 200 before idle expiry;
- a due activity write advanced `last_seen_at`;
- a second request inside the 60-second interval did not rewrite
  `last_seen_at`;
- absolute `expires_at` remained unchanged;
- an idle-expired ordinary GET returned HTTP 401 `session_expired`;
- an idle-expired protected mutation returned HTTP 401 `session_expired` before
  domain dispatch;
- idle denial accountability remained exact and secret-free;
- logout of a replacement non-idle session returned HTTP 204;
- replay of the revoked replacement cookie returned HTTP 401
  `credential_revoked`;
- the isolated lifecycle had zero active rows after cleanup;
- the original daemon configuration was restored exactly;
- the temporary runtime systemd drop-in was removed;
- the temporary idle environment was not present in the final unit;
- SQLite quick check returned `ok`;
- foreign-key check returned no rows;
- the accepted Phase-62 daemon remained installed and active;
- zero VDR domain mutations occurred;
- automatic rollback was not required.

Durable secret-free evidence:

```text
/var/backups/vdr-suite-phase62-slice2v-20260802T092139Z-e84415fadb25
```

Runtime report:

```text
/var/backups/vdr-suite-phase62-slice2v-20260802T092139Z-e84415fadb25/runtime-acceptance-report.txt
```

Runtime report SHA-256:

```text
0a961fbc8b51158fd4a16aa24fc9afde7dafa9d5272e986a46ec73880c311f86
```

This acceptance closes only request-time idle expiry and throttled
`last_seen_at`. Physical cleanup, retention, eviction, refresh and session
administration remain separate gaps.
