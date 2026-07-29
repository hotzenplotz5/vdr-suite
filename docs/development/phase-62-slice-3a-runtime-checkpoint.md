# Phase 62 Slice 3A Runtime Checkpoint

## Purpose

This checkpoint records the durable, non-secret state of the real yaVDR
installation and acceptance of Phase 62 Slice 3A. It exists specifically to
prevent later chats or operators from repeating completed installation,
routing, credential, unauthenticated provenance, or repository diagnosis work.

Read the canonical [new-chat handoff](../NEW-CHAT-HANDOFF.md) first.

Never store plaintext passwords, Authorization headers, cookies, CSRF tokens,
raw browser-session values, password hashes, verifier material, or process
environments in this document.

## Status vocabulary

- **RUNTIME VERIFIED**: observed on the real yaVDR runtime.
- **REPOSITORY VERIFIED**: demonstrated by code analysis, regression tests, and CI.
- **OPEN**: not yet accepted on the current installed runtime.
- **DO NOT REPEAT**: already completed and not useful to repeat unless a directly
  relevant fingerprint changes.
- **VOLATILE**: recheck only immediately before a related mutation.

## Repository and PR state

**REPOSITORY VERIFIED on 2026-07-29:**

```text
Repository: hotzenplotz5/vdr-suite
Pull request: #117
PR state: open, Draft, not merged
Base: main @ cb77ff66e11dca7db2eafa36525762dcde35102d
Branch: phase-62-security-identity-foundation
Local branch: phase62-pr117
Checkout: /home/yavdr/vdr-suite-phase62
Slice 3A runtime-install source head: 0da681b3603dcafbbba27c57f6ab8da30c5cf006
Accountability serialization fix head: e3a8b7815c5df06093656f4724e6001d22c5755a
Fix validation CI: run 6497, completed successfully
```

The branch remains Draft. Do not mark it ready, merge it, enable auto-merge, or
mutate review state without explicit approval.

## Installed Slice 3A runtime

**RUNTIME VERIFIED on 2026-07-29:**

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

The installed daemon predates the accountability serialization fix. The fix is
repository-validated but is **not yet installed or runtime-accepted**.

## Active Nginx state

**RUNTIME VERIFIED on 2026-07-29:**

- the active site is `/etc/nginx/sites-enabled/default`;
- exactly one include of `snippets/vdr-suite.conf` is active;
- `nginx -t` succeeded before the completed reload;
- Nginx and the daemon remained active;
- the loaded configuration contains the canonical redirects and
  `location ^~ /vdr-suite/`;
- the proxy target is `http://127.0.0.1:18080/` with prefix stripping;
- public root `/api/*` remains owned by yaVDR.

The accountability fix does not change Nginx. Do not reinstall, edit, test, or
reload Nginx for that fix.

## Completed public-origin acceptance

### Canonical redirects

**RUNTIME VERIFIED:**

```text
/vdr-suite          -> 308 /vdr-suite/frontend/
/vdr-suite/         -> 308 /vdr-suite/frontend/
/vdr-suite/frontend -> 308 /vdr-suite/frontend/
```

### Namespace provenance

**RUNTIME VERIFIED:**

```text
Public yaVDR /api/                       -> 307
Public Suite /vdr-suite/api/vdr/status  -> 401
Direct daemon /api/vdr/status           -> 401
```

The public and direct Suite statuses matched, no accepted probe returned `502`,
and the Suite prefix did not take over yaVDR public root `/api/`.

### Unauthenticated frontend and assets

**RUNTIME VERIFIED EXPECTED BEHAVIOUR:**

- direct `/frontend/` returned `401`;
- public `/vdr-suite/frontend/` returned `401`;
- public `/vdr-suite/frontend/platform/public-url.js` returned `401`;
- the security gate runs before static frontend and asset delivery in the
  current compatibility mode;
- denial JSON bodies differ because each request carries a new request ID.

Do not byte-compare unauthenticated `401` bodies as static asset content.

## Credential state

**RUNTIME VERIFIED on 2026-07-29:**

- the managed-Basic username and password hash are configured;
- the earlier managed-Basic plaintext password is unavailable;
- after explicit approval, a non-default Legacy-Basic credential was set in
  `/etc/default/vdr-suite-daemon`;
- the running daemon uses the explicit `VDR_SUITE_BASIC_AUTH` value;
- the credential value is deliberately not recorded here;
- unauthenticated frontend access still correctly returns `401`.

Recorded configuration rollback owner:

```text
/root/.vdr-suite-phase62-auth-backup
```

Do not rotate Legacy Basic or Managed Basic again for this acceptance path.

## Observed authenticated-acceptance blocker

### One controlled issuance attempt

**RUNTIME VERIFIED on 2026-07-29:**

A single controlled request was made to:

```text
POST /vdr-suite/api/security/browser-sessions
```

The effective explicit Legacy-Basic credential reached the browser-session
lifecycle gate. The response was:

```text
HTTP status: 503
error code: accountability_unavailable
error message: Security accountability persistence is unavailable
```

No browser-session cookie, CSRF token, or usable browser session was issued.
Cookie-path, authenticated frontend/assets, logout, revocation, and replay
acceptance therefore remain open for the fixed daemon.

### Database evidence after the failed attempt

**RUNTIME VERIFIED:**

```text
PRAGMA quick_check: ok
legacy-local-web        active, not revoked
legacy-browser          active, not revoked
legacy-basic-credential active, not revoked, not expired
```

Historical accountability rows still include earlier successful
`browser.session.issue` decisions. The exact SQLite result code for the observed
503 was not captured by the installed daemon, so this checkpoint does not claim
that a specific SQLite code was observed.

## Repository diagnosis

**REPOSITORY VERIFIED:**

1. `BrowserSessionHttpGate` persists the allow/deny accountability decision
   before browser-session issuance and returns `accountability_unavailable` when
   that append fails.
2. The daemon owns one shared `Database` object while EPG and recording workers
   perform background writes.
3. Transactional cache and metadata writers acquire
   `Database::acquireTransactionLease()` around their transactions.
4. `AccountabilityEventRepository::append()` previously wrote through the same
   SQLite connection without acquiring that lease.
5. The old append path reduced prepare/bind/step/finalize failures to `false` and
   did not preserve a non-secret SQLite result code for diagnosis.

This is a concrete repository concurrency and observability defect. It permits
an accountability insert to interleave with a transaction owned by another
runtime task. It is consistent with the observed 503. Because the old daemon did
not expose the SQLite result code, the exact runtime failure code remains
unknown and is not guessed.

## Repository fix

**REPOSITORY VERIFIED:**

```text
Implementation:
a2dd3f2689eedf08d6b0df46c587a637b977fd33
fix(security): serialize accountability persistence

Lease regression test:
c0497d45264b78936beabc17d022b98f568407cc
test(security): cover accountability transaction serialization

Non-secret diagnostics test:
e3a8b7815c5df06093656f4724e6001d22c5755a
test(security): verify non-secret accountability diagnostics
```

The fix:

- acquires the shared database transaction lease for every accountability
  append;
- prevents an append from entering another task's leased transaction;
- checks prepare, bind, step, and finalize separately;
- logs only the SQLite primary and extended result codes;
- does not log event IDs, actor IDs, request context, credentials, or secrets.

The regression test holds the transaction lease in one thread and proves that a
concurrent append remains blocked until lease release. A control run without the
lease fails that assertion. A separate constraint failure verifies that the
new diagnostic contains SQLite codes but no event or actor identifier.

GitHub Actions run 6497 passed:

- documentation checks;
- frontend regression contracts;
- strict Make and test inventory;
- fast regression tests, including the security repository test;
- daemon build;
- packaging and install staging.

## Runtime delta

The repository fix has **not** changed the real yaVDR runtime:

- installed daemon hash remains the pre-fix value recorded above;
- no daemon restart was performed for the fix;
- no new browser-session issuance attempt was made;
- Nginx, frontend files, credentials, and database were not changed;
- no session, cookie, or CSRF secret was produced by this diagnosis.

## Anti-loop boundary

The following work is **DO NOT REPEAT** unless its own directly relevant
fingerprint changes:

1. repository-wide Slice 3A architecture analysis;
2. Slice 3A frontend or Nginx installation;
3. active include replacement, `nginx -t`, or Nginx reload;
4. canonical redirect and namespace-provenance probes;
5. unauthenticated frontend or resolver probes;
6. unauthenticated error-body byte comparisons;
7. Legacy-Basic or Managed-Basic credential rotation;
8. managed-Basic password recovery attempts;
9. the accountability append/transaction-lease root-cause analysis;
10. another authenticated issuance attempt before the fixed daemon is installed.

A new chat must resume from the validated repository fix, not from credentials,
Nginx, installation, redirects, or broad Phase 62 analysis.

## Exact next action

The next bounded operation is a separately approved **daemon-only fix
installation**, not another investigation loop:

1. fast-forward the real checkout to the final green documentation descendant of
   `e3a8b7815c5df06093656f4724e6001d22c5755a`;
2. verify the local branch is clean and tracks the remote branch;
3. build the daemon and run the focused security test from that exact checkout;
4. capture the current installed daemon hash and make a timestamped daemon
   backup;
5. replace only `/usr/sbin/vdr-suite-daemon` with the fixed build;
6. restart only `vdr-suite-daemon.service` and verify the listener;
7. perform exactly one authenticated public-origin acceptance pass covering
   issuance, cookie `Path=/vdr-suite/`, authenticated frontend/assets, CSRF
   denial, logout, revocation, and replay;
8. clean up temporary session and acceptance files;
9. rollback only the daemon if the fixed acceptance fails.

No Nginx, frontend, credential, or planned database mutation belongs to this
installation procedure.

## Approval boundary

Repository diagnosis, implementation, tests, commits, and CI are complete.
Any real-runtime file replacement, daemon restart, database write through a new
issuance attempt, or authenticated acceptance requires a fresh explicit approval
for the exact daemon-only procedure.
