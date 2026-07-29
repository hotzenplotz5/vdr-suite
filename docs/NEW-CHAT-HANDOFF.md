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
- [Slice 3A real-runtime checkpoint](development/phase-62-slice-3a-runtime-checkpoint.md)

The Slice 3A runtime checkpoint is the newest authority for the current
installation, routing, credential, and acceptance boundary.

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

Recheck only volatile state that is directly relevant to the next mutation:

1. local branch, HEAD, upstream, and clean/dirty state;
2. remote head and PR Draft/base/CI state;
3. installed binary only before replacing the binary;
4. active Nginx only before changing Nginx;
5. database state only before a database or lifecycle mutation.

Do not repeat completed browser-session acceptance, Slice 3A installation,
Nginx activation, redirects, namespace provenance, or unauthenticated frontend
probes merely because a chat changed.

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
Final green Slice 3A code head: 50bc66ba12dc663b77ebd141a85e167666218b09
Pre-runtime documentation head: 0da681b3603dcafbbba27c57f6ab8da30c5cf006
Runtime checkpoint commit: 3026a7ae9f49d94dcb2f016e5fb6aa80bdd93e84
Final pre-runtime CI: run 6487, completed successfully
```

PR #117 must remain open and Draft. Do not mark it ready, merge it, enable
auto-merge, rewrite its branch, or mutate review state without explicit
approval.

The PR description contains stale runtime and CI wording. This handoff and the
runtime checkpoint are newer.

## Real yaVDR security baseline

**VERIFIED on 2026-07-29:**

```text
Unit: vdr-suite-daemon.service
Executable: /usr/sbin/vdr-suite-daemon
Configuration: /etc/default/vdr-suite-daemon
Database: /var/lib/vdr-suite/vdr-suite.db
Listener: 0.0.0.0:18080
```

Completed earlier real-runtime security acceptance includes persistent identity
lifecycle, managed Basic verification, browser-session issuance/logout,
ordinary-route cookie authentication and precedence, malformed/duplicate/
revoked-cookie denial, fail-closed browser mutations, accountability, atomic
revocation, replay denial, database checks, and cleanup.

Do not relabel that acceptance as pending because a chat changed.

## Slice 3A repository state

**IMPLEMENTED AND CI-VALIDATED:**

- Suite-owned public namespace `/vdr-suite/`;
- internal daemon paths remain unchanged;
- canonical redirects to `/vdr-suite/frontend/`;
- prefix-stripping proxy to `127.0.0.1:18080`;
- forwarded prefix, SSE-safe settings, and cookie path rewrite;
- no public root `/api`, Uvicorn socket, or `sub_filter` ownership;
- immutable first-loaded `VdrSuitePublicUrl` resolver;
- relative initial HTML assets and direct-daemon compatibility;
- URL adaptation for fetch, SSE, dynamic scripts, DOM/CSS, logos, artwork, and
  Remote assets;
- daemon asset ownership, installation, architecture, Node, ownership, and
  packaging tests.

## Slice 3A installed runtime

**VERIFIED on 2026-07-29:**

```text
Runtime backup:
/var/backups/vdr-suite-phase62-slice3a-20260729-122447

Installed daemon SHA-256:
85b12e38a9f23fde7ae84c9773914b39874ca81edec5ea4ccecd997d77b3dc02

Installed index SHA-256:
d5dc42df979b61115c3cf49e5682971cee9acfd1b11d8511fc751c86d30a75a8

Installed public-url.js SHA-256:
7c9dbc35646e857cfd31b0ebf1f220fb94e1ef48e17c62909d745ee63e97a5c5

Installed Nginx snippet SHA-256:
b9f7114d35fcd79a49604da195f1f2c340c4d7f1f66bed24a448b349791acb01
```

The active site includes `snippets/vdr-suite.conf`. `nginx -t` passed, Nginx
reloaded, the daemon restarted successfully, the database opened, and the
listener is active.

## Completed public-origin acceptance

**VERIFIED:**

```text
/vdr-suite          -> 308 /vdr-suite/frontend/
/vdr-suite/         -> 308 /vdr-suite/frontend/
/vdr-suite/frontend -> 308 /vdr-suite/frontend/

Public yaVDR /api/                       -> 307
Public Suite /vdr-suite/api/vdr/status  -> 401
Direct daemon /api/vdr/status           -> 401
```

No accepted probe returned `502`. Public `/api/*` remains yaVDR-owned.

Unauthenticated Suite HTML and assets return `401` by design because the
security gate runs before static delivery. Their JSON bodies contain unique
request IDs and must not be byte-compared as asset content.

## Credential state

**VERIFIED:**

- managed-Basic username and password hash are configured;
- the managed-Basic plaintext password is unavailable;
- the previously absent explicit Legacy-Basic header exposed the repository
  compatibility fallback;
- after explicit approval, a new non-default `VDR_SUITE_BASIC_AUTH` was set;
- the daemon restarted and uses the explicit header;
- the value is secret and must never be printed or committed;
- the configuration rollback owner is
  `/root/.vdr-suite-phase62-auth-backup`.

Do not rotate credentials again merely to retry the same acceptance.

## Current authenticated-acceptance blocker

**VERIFIED on 2026-07-29:** one controlled public-prefix browser-session
issuance attempt returned:

```text
HTTP 503
error code: accountability_unavailable
error message: Security accountability persistence is unavailable
```

No cookie, CSRF token, or usable browser session was issued.

Database evidence immediately afterward:

```text
PRAGMA quick_check: ok
legacy-local-web: active, not revoked
legacy-browser: active, not revoked
legacy-basic-credential: active, not revoked, not expired
```

Historical `browser.session.issue` accountability rows include earlier allowed
issuance decisions. The exact reason the current accountability append failed
has not yet been captured. Do not guess a narrower root cause.

## Anti-loop boundary

Do **not** repeat any of the following unless its own fingerprint changed:

1. Slice 3A installation or daemon build/install;
2. Nginx snippet installation, include replacement, test, or reload;
3. redirect and namespace-provenance probes;
4. unauthenticated frontend or resolver probes;
5. unauthenticated error-body byte comparisons;
6. Legacy-Basic or Managed-Basic credential rotation;
7. browser-session issuance attempts before accountability persistence is
   diagnosed;
8. repository-wide Phase 62 or Slice 3A analysis.

## Exact next action

The next bounded task is repository/code diagnosis, not another runtime loop:

1. inspect the accountability append path and SQLite concurrency/error handling
   used by `BrowserSessionHttpGate`;
2. explain the observed `accountability_unavailable` without exposing secrets;
3. add diagnostic coverage and a regression test if product code is responsible;
4. run repository tests and CI;
5. request explicit approval before installing any fix or making another
   authenticated issuance attempt;
6. after a demonstrated fix, perform exactly one public-origin acceptance pass
   for issuance, cookie `Path=/vdr-suite/`, authenticated frontend/assets, CSRF
   denial, logout, revocation, and replay.

Do not change Nginx or credentials during diagnosis unless new evidence shows
that either is causal.

## Secret restrictions

Never print or store Authorization headers, passwords, cookies, CSRF tokens,
password hashes, verifier material, login response bodies containing secrets, or
`/proc/.../environ`. Preserve only non-secret status and fingerprints.

## Maintenance rule

Update this file whenever repository, PR, runtime, routing, blocker, or
next-action truth changes. Preserve durable non-secret evidence, mark stale
statements, record the next approval boundary, and keep all secrets out of the
repository.
