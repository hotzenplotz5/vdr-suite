# VDR-Suite New Chat Handoff

## Purpose

This is the canonical entry point for every new VDR-Suite chat. Read it before
repeating repository-wide analysis or real-runtime acceptance. A new chat alone
is not a reason to start over.

Trust completed items marked **VERIFIED** unless a directly relevant repository,
binary, configuration, database, routing, or behaviour fingerprint changed.

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

The Slice 3A runtime checkpoint is the newest authority for installation,
routing, credential, blocker, repository-fix, and approval-boundary truth.

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

Recheck only volatile state directly needed for the next approved operation:

1. local branch, HEAD, upstream, and clean/dirty state before local work;
2. remote head and PR Draft/base/CI state;
3. installed daemon hash immediately before replacing the daemon;
4. service and listener state immediately before and after daemon restart;
5. database lifecycle state only around the one approved acceptance attempt.

Do not repeat Slice 3A installation, Nginx activation, redirects, namespace
provenance, unauthenticated frontend probes, credential rotation, or the
accountability concurrency diagnosis merely because a chat changed.

## Active workstream

```text
Repository: hotzenplotz5/vdr-suite
Checkout: /home/yavdr/vdr-suite-phase62
Local branch: phase62-pr117
Remote branch: phase-62-security-identity-foundation
Pull request: #117
Base: main @ cb77ff66e11dca7db2eafa36525762dcde35102d
Accepted security runtime baseline: d0500da0aa43b11b57eca23d5d757d070b2beb98
Slice 3A runtime-install source head: 0da681b3603dcafbbba27c57f6ab8da30c5cf006
Validated accountability fix code head: e3a8b7815c5df06093656f4724e6001d22c5755a
Accountability-fix CI: run 6497, completed successfully
Runtime checkpoint update: ec9a2b24452e75b332b56b2cf9bf6030d2af716b
```

PR #117 must remain open and Draft. Do not mark it ready, merge it, enable
auto-merge, rewrite its branch, or mutate review state without explicit
approval.

The PR description contains stale runtime and CI wording. This handoff and the
runtime checkpoint are newer. Do not edit PR metadata without explicit approval.

## Completed earlier Phase 62 security acceptance

**VERIFIED on the real yaVDR runtime:**

- persistent actor, device, session, and credential lifecycle;
- managed Basic verification;
- browser-session issuance and logout;
- ordinary-route cookie authentication and precedence;
- malformed, duplicate, expired, and revoked-cookie denial;
- fail-closed browser mutations;
- accountability persistence;
- atomic revocation and replay denial;
- database consistency checks and temporary-data cleanup.

Do not relabel this earlier acceptance as pending because a chat changed.

## Slice 3A repository implementation

**VERIFIED AND CI-VALIDATED:**

- Suite-owned public namespace `/vdr-suite/`;
- unchanged internal daemon paths;
- canonical redirects to `/vdr-suite/frontend/`;
- prefix-stripping proxy to `127.0.0.1:18080`;
- forwarded prefix, SSE-safe settings, and cookie path rewrite;
- no ownership of public root `/api`, the Uvicorn socket, or `sub_filter`;
- immutable first-loaded `VdrSuitePublicUrl` resolver;
- relative initial HTML assets and direct-daemon compatibility;
- URL adaptation for fetch, SSE, scripts, DOM/CSS, logos, artwork, and Remote
  assets;
- daemon asset ownership, installation, architecture, Node, ownership, and
  packaging tests.

## Installed Slice 3A runtime

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

The active site includes `snippets/vdr-suite.conf`. Nginx and the daemon are
active and the listener is on `0.0.0.0:18080` at the recorded acceptance point.

The installed daemon predates the accountability serialization fix. The fixed
repository code is not yet installed on the real yaVDR runtime.

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

Public `/api/*` remains yaVDR-owned. Unauthenticated Suite HTML and assets return
`401` by design because the security gate runs before static delivery. Their
error bodies carry unique request IDs and must not be byte-compared as asset
content.

## Credential state

**VERIFIED:**

- managed-Basic username and password hash are configured;
- the managed-Basic plaintext password is unavailable;
- after explicit approval, a non-default `VDR_SUITE_BASIC_AUTH` was set;
- the running installed daemon uses that explicit value;
- the value must never be printed or committed;
- configuration rollback owner:
  `/root/.vdr-suite-phase62-auth-backup`.

Do not rotate Legacy Basic or Managed Basic again for this acceptance path.

## Observed authenticated-acceptance blocker

**VERIFIED on the installed pre-fix daemon:**

One controlled request to
`POST /vdr-suite/api/security/browser-sessions` returned:

```text
HTTP 503
error code: accountability_unavailable
error message: Security accountability persistence is unavailable
```

No cookie, CSRF token, or usable browser session was issued.

The database remained healthy and the legacy compatibility actor, device, and
credential remained active, unrevoked, and unexpired. The installed daemon did
not expose the concrete SQLite result code, so no specific runtime SQLite code
is claimed.

## Accountability root cause and repository fix

**REPOSITORY VERIFIED:**

- `BrowserSessionHttpGate` requires the accountability decision to persist
  before session issuance;
- the daemon shares one `Database` object across HTTP and background workers;
- transactional cache/metadata writers use `Database::acquireTransactionLease()`;
- the old accountability append wrote through the same SQLite connection without
  that lease;
- the old append path discarded prepare/bind/step/finalize result details.

This is a concrete concurrency and observability defect. It allows an audit
insert to enter a transaction leased by another runtime task. It is consistent
with the observed `accountability_unavailable`; the exact old-runtime SQLite
result code remains unknown.

Repository fix commits:

```text
a2dd3f2689eedf08d6b0df46c587a637b977fd33
fix(security): serialize accountability persistence

c0497d45264b78936beabc17d022b98f568407cc
test(security): cover accountability transaction serialization

e3a8b7815c5df06093656f4724e6001d22c5755a
test(security): verify non-secret accountability diagnostics
```

The fixed append acquires the shared transaction lease and reports only SQLite
primary and extended result codes. Regression coverage proves a concurrent
append waits for lease release and verifies that diagnostics contain no event or
actor identifier.

GitHub Actions run 6497 passed documentation, frontend, Make inventory, fast
regression, daemon build, and packaging/install staging.

## Runtime delta after the repository fix

No real-runtime mutation was made for this diagnosis or fix:

- installed daemon hash is still the pre-fix value above;
- no daemon restart was performed;
- no new browser-session issuance was attempted;
- Nginx, frontend files, credentials, and database were unchanged;
- no session, cookie, or CSRF secret was produced.

## Anti-loop boundary

Do **not** repeat any of the following unless its directly relevant fingerprint
changed:

1. repository-wide Phase 62 or Slice 3A analysis;
2. Slice 3A frontend/Nginx installation;
3. Nginx include replacement, testing, or reload;
4. redirect and namespace-provenance probes;
5. unauthenticated frontend or resolver probes;
6. unauthenticated error-body byte comparisons;
7. Legacy-Basic or Managed-Basic credential rotation;
8. managed-Basic password recovery attempts;
9. accountability append/transaction-lease root-cause analysis;
10. browser-session issuance before the fixed daemon is installed.

## Exact next action

The next bounded operation is a separately approved **daemon-only fix
installation**:

1. fast-forward the real checkout to the final green branch head descending from
   `e3a8b7815c5df06093656f4724e6001d22c5755a`;
2. verify clean branch and tracking state;
3. build the daemon and run the focused security test;
4. fingerprint and back up the currently installed daemon;
5. replace only `/usr/sbin/vdr-suite-daemon`;
6. restart only `vdr-suite-daemon.service` and verify port `18080`;
7. perform exactly one authenticated acceptance pass for issuance, cookie
   `Path=/vdr-suite/`, authenticated frontend/assets, CSRF denial, logout,
   revocation, and replay;
8. clean temporary acceptance files and roll back only the daemon if necessary.

No Nginx, frontend, credential, or planned database mutation belongs to this
fix-install procedure.

## Approval boundary

Repository diagnosis, implementation, tests, commits, and CI are complete.
Any real-runtime binary replacement, daemon restart, or new authenticated
issuance attempt requires fresh explicit approval for the exact daemon-only
procedure.

## Secret restrictions

Never print or store Authorization headers, passwords, cookies, CSRF tokens,
password hashes, verifier material, login response bodies containing secrets, or
`/proc/.../environ`. Preserve only non-secret status and fingerprints.

## Maintenance rule

Update this file whenever repository, PR, runtime, routing, blocker, or
next-action truth changes. Preserve durable non-secret evidence, mark stale
statements, record the next approval boundary, and keep all secrets out of the
repository.
