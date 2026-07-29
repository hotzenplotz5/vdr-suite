# Phase 62 Slice 3A Runtime Checkpoint

## Purpose

This checkpoint records the durable, non-secret state of the real yaVDR
installation and acceptance of Phase 62 Slice 3A. It exists specifically to
prevent later chats or operators from repeating completed installation,
routing, credential, and unauthenticated provenance work.

Read the canonical [new-chat handoff](../NEW-CHAT-HANDOFF.md) first.

Never store plaintext passwords, Authorization headers, cookies, CSRF tokens,
raw browser-session values, password hashes, verifier material, or process
environments in this document.

## Status vocabulary

- **VERIFIED**: observed on the real yaVDR runtime.
- **OPEN**: not yet accepted or root-caused.
- **DO NOT REPEAT**: already completed and not useful to repeat unless a relevant
  fingerprint changes.
- **VOLATILE**: recheck only immediately before a related mutation.

## Repository baseline

**VERIFIED on 2026-07-29:**

```text
Repository: hotzenplotz5/vdr-suite
Pull request: #117
PR state: open, Draft, not merged
Base: main @ cb77ff66e11dca7db2eafa36525762dcde35102d
Branch: phase-62-security-identity-foundation
Local branch: phase62-pr117
Checkout: /home/yavdr/vdr-suite-phase62
Synced pre-checkpoint branch head: 0da681b3603dcafbbba27c57f6ab8da30c5cf006
Final green Slice 3A code head: 50bc66ba12dc663b77ebd141a85e167666218b09
Final pre-runtime CI: run 6487, completed successfully
```

The branch remains Draft. Do not mark it ready, merge it, enable auto-merge, or
mutate review state without explicit approval.

## Installed Slice 3A runtime

**VERIFIED on 2026-07-29:**

```text
Runtime backup:
/var/backups/vdr-suite-phase62-slice3a-20260729-122447

Installed daemon:
/usr/sbin/vdr-suite-daemon
SHA-256: 85b12e38a9f23fde7ae84c9773914b39874ca81edec5ea4ccecd997d77b3dc02

Installed frontend index:
/usr/share/vdr-suite/web/frontend/index.html
SHA-256: d5dc42df979b61115c3cf49e5682971cee9acfd1b11d8511fc751c86d30a75a8

Installed public URL runtime:
/usr/share/vdr-suite/web/frontend/platform/public-url.js
SHA-256: 7c9dbc35646e857cfd31b0ebf1f220fb94e1ef48e17c62909d745ee63e97a5c5

Installed Nginx snippet:
/etc/nginx/snippets/vdr-suite.conf
SHA-256: b9f7114d35fcd79a49604da195f1f2c340c4d7f1f66bed24a448b349791acb01
```

The installed files matched the repository byte-for-byte at installation time.
The daemon was restarted successfully, the database opened, and the HTTP
listener became available on `0.0.0.0:18080`.

## Active Nginx state

**VERIFIED on 2026-07-29:**

- the active site is `/etc/nginx/sites-enabled/default`;
- the old exact lifecycle include was replaced by exactly one include of
  `snippets/vdr-suite.conf`;
- `nginx -t` succeeded before reload;
- Nginx reloaded successfully;
- the daemon and Nginx remained active;
- the loaded configuration contains the three exact canonical redirects and
  `location ^~ /vdr-suite/`;
- the proxy target is `http://127.0.0.1:18080/` with prefix stripping;
- public root `/api/*` remains owned by yaVDR.

## Completed public-origin acceptance

### Canonical redirects

**VERIFIED:**

```text
/vdr-suite          -> 308 /vdr-suite/frontend/
/vdr-suite/         -> 308 /vdr-suite/frontend/
/vdr-suite/frontend -> 308 /vdr-suite/frontend/
```

### Namespace provenance

**VERIFIED:**

```text
Public yaVDR /api/                       -> 307
Public Suite /vdr-suite/api/vdr/status  -> 401
Direct daemon /api/vdr/status           -> 401
```

The public and direct Suite statuses matched, no request returned `502`, and the
Suite prefix did not take over yaVDR public root `/api/`.

### Unauthenticated frontend and assets

**VERIFIED EXPECTED BEHAVIOUR:**

- direct `/frontend/` returned `401`;
- public `/vdr-suite/frontend/` returned `401`;
- public `/vdr-suite/frontend/platform/public-url.js` returned `401`;
- the security gate runs before static frontend and asset delivery in the
  current `legacy-basic` compatibility mode;
- the JSON bodies differ only because each denial carries a new request ID.

Do not use byte comparisons against unauthenticated `401` bodies as a static
asset acceptance test.

## Credential state

**VERIFIED on 2026-07-29:**

- the daemon effectively runs in the default `legacy-basic` mode;
- the managed-Basic username and password hash are configured;
- the earlier managed-Basic plaintext password is unavailable;
- the previously absent `VDR_SUITE_BASIC_AUTH` allowed the public repository
  fallback to become effective;
- after explicit approval, a new non-default Legacy-Basic credential was set in
  `/etc/default/vdr-suite-daemon`;
- the daemon was restarted and its effective environment contains
  `VDR_SUITE_BASIC_AUTH`;
- the credential value is deliberately not recorded here;
- unauthenticated frontend access still correctly returns `401`.

Recorded configuration rollback owner:

```text
/root/.vdr-suite-phase62-auth-backup
```

Do not rotate either Legacy Basic or Managed Basic again merely to retry the
same acceptance path.

## Authenticated browser-session acceptance checkpoint

### One controlled issuance attempt

**VERIFIED on 2026-07-29:**

A single controlled public-prefix request was made to:

```text
POST /vdr-suite/api/security/browser-sessions
```

The effective new Legacy-Basic credential was accepted far enough to reach the
browser-session lifecycle gate. The response was:

```text
HTTP status: 503
error code: accountability_unavailable
error message: Security accountability persistence is unavailable
```

No browser-session cookie, CSRF token, or usable browser session was issued.
Therefore cookie-path, authenticated frontend/assets, logout, revocation, and
replay acceptance remain open.

### Database evidence after the failed attempt

**VERIFIED:**

```text
PRAGMA quick_check: ok
```

The compatibility identities remained valid:

```text
legacy-local-web        active, not revoked
legacy-browser          active, not revoked
legacy-basic-credential active, not revoked, not expired
```

Historical accountability rows still include earlier successful
`browser.session.issue` decisions. The new `503 accountability_unavailable`
means the current gate could not persist its accountability decision. The exact
SQLite failure reason has not yet been captured, so no narrower root cause is
claimed.

## Anti-loop boundary

The following work is **DO NOT REPEAT** unless a directly relevant fingerprint
changes:

1. repository-wide Slice 3A architecture analysis;
2. checkout synchronization to the implemented Slice 3A branch;
3. daemon/frontend installation;
4. Nginx snippet installation;
5. active include replacement;
6. `nginx -t` and Nginx reload;
7. daemon restart solely for Slice 3A installation;
8. canonical redirect checks;
9. yaVDR-versus-Suite namespace provenance checks;
10. unauthenticated `401` frontend/asset probes;
11. Legacy-Basic credential rotation;
12. managed-Basic password recovery attempts;
13. repeated browser-session issuance attempts without first explaining the
    accountability persistence failure.

A new chat must not restart from credentials, Nginx, installation, redirects,
or unauthenticated asset byte comparisons.

## Exact next action

The next bounded task is **not another runtime acceptance loop**. It is:

1. inspect the accountability append path and SQLite concurrency/error handling
   used by `BrowserSessionHttpGate`;
2. determine why the current allowed issuance decision returned
   `accountability_unavailable` despite a healthy database and active legacy
   identity;
3. add or improve non-secret diagnostic coverage and a regression test if the
   cause is in product code;
4. run repository tests and CI;
5. only after a demonstrated fix, install the changed code through a separately
   approved runtime procedure;
6. perform exactly one authenticated public-origin acceptance pass covering
   issuance, `Path=/vdr-suite/`, frontend/assets, CSRF denial, logout, and replay;
7. clean up the temporary session and acceptance files.

Do not alter Nginx or credentials during this diagnosis unless new evidence
shows they are causally involved.

## Approval boundary

Read-only repository and code analysis may proceed. Any further real-runtime
mutation, credential change, daemon restart, Nginx change, database write, or
new authenticated issuance attempt requires a fresh explicit approval tied to a
specific diagnosis or fix.
