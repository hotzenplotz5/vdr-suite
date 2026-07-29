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

The Slice 3A runtime checkpoint and this handoff are the newest authorities for
installation, routing, credentials, authenticated lifecycle, accountability-fix
acceptance, persisted browser-grant acceptance and anti-loop truth.

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
Configurable photorealistic VDR Remote (#115)

Current active phase:
Phase 62 - Identity, RBAC and Accountability Foundation
```

## Active workstream

```text
Repository: hotzenplotz5/vdr-suite
Checkout: /home/yavdr/vdr-suite-phase62
Local branch: phase62-pr117
Remote branch: phase-62-security-identity-foundation
Pull request: #117
Base: main @ cb77ff66e11dca7db2eafa36525762dcde35102d
Runtime-accepted source head: 47adb6577511209bfe7288ce8ce0fbe03b53a94c
Accountability-fix code head: e3a8b7815c5df06093656f4724e6001d22c5755a
Accountability-fix CI: run 6497, successful
Persisted browser-grant code/runtime head: 47adb6577511209bfe7288ce8ce0fbe03b53a94c
Persisted browser-grant CI: run 6507, successful
Installed daemon SHA-256: 652dfc6a29f466fca977d34587db8a39bbc631509b735e02f8dd1942c46088e1
Slice 2C daemon/database backup: /var/backups/vdr-suite-phase62-slice2c-20260729-171704
Earlier runtime-closeout documentation commit:
4b46f97d582f93dc00f35dd4d0347d2f1e32b7d8
```

PR #117 must remain open and Draft. Do not mark it ready, merge it, enable
auto-merge, rewrite its branch, or mutate review state without explicit
approval.

The PR description contains stale grant-runtime, open-work and CI wording. This
handoff and the runtime checkpoint are newer. Do not edit PR metadata without
explicit approval.

## Resume rule

Recheck only volatile state directly needed for the next approved operation:

1. local branch, HEAD, upstream, and clean/dirty state before local work;
2. remote head and PR Draft/base/CI state;
3. installed daemon hash only before a future daemon replacement;
4. service/listener/database state only around a future approved runtime change.

Do not repeat Slice 3A installation, Nginx activation, redirects, namespace
provenance, unauthenticated probes, credential rotation, accountability root
cause analysis, daemon-only fix installation, or browser-session lifecycle
acceptance merely because a chat changed.

## Completed Phase 62 foundation

**VERIFIED in repository and, where stated, on the real yaVDR runtime:**

- canonical actor, device, session, credential, authentication, request, and
  correlation context;
- centralized permission and backend-scope decisions;
- explicit legacy compatibility mode and fail-closed enforced-mode boundaries;
- persistent actor/device/session/credential lifecycle;
- managed Basic verification without managed defaults;
- browser-session verifier persistence;
- atomic session issuance with independent session and CSRF secrets;
- isolated HTTPS issue/logout lifecycle;
- ordinary-route browser-cookie authentication and precedence;
- malformed, duplicate, expired, unknown, and revoked-cookie denial;
- empty browser grant set without inherited compatibility grants;
- active actor grants loaded from additive backend-scoped persistence;
- fail-closed `permission_grants_unavailable` distinction and recovery;
- fail-closed browser business mutations until route policy and CSRF migration;
- append-only accountability and atomic revocation/replay denial;
- Suite-owned public namespace `/vdr-suite/` with unchanged daemon paths;
- canonical redirects, prefix-stripping proxy, forwarded prefix, and cookie path
  rewrite;
- daemon-owned authenticated frontend and asset delivery.

## Installed Slice 3A and accountability-fix runtime

**VERIFIED on 2026-07-29:**

```text
Original Slice 3A backup:
/var/backups/vdr-suite-phase62-slice3a-20260729-122447

Pre-fix daemon backup:
/var/backups/vdr-suite-phase62-accountability-fix-20260729-162026
SHA-256:
85b12e38a9f23fde7ae84c9773914b39874ca81edec5ea4ccecd997d77b3dc02

Installed current daemon:
/usr/sbin/vdr-suite-daemon
SHA-256:
652dfc6a29f466fca977d34587db8a39bbc631509b735e02f8dd1942c46088e1

Slice 2C pre-install daemon/database backup:
/var/backups/vdr-suite-phase62-slice2c-20260729-171704

Installed index SHA-256:
d5dc42df979b61115c3cf49e5682971cee9acfd1b11d8511fc751c86d30a75a8

Installed app.js SHA-256:
33c7c92a86747cfebea66e79f0d972723471aac90a986751e90b3aa8d9fbf93a

Installed public-url.js SHA-256:
7c9dbc35646e857cfd31b0ebf1f220fb94e1ef48e17c62909d745ee63e97a5c5

Installed Nginx snippet SHA-256:
b9f7114d35fcd79a49604da195f1f2c340c4d7f1f66bed24a448b349791acb01
```

The current daemon-only replacement completed successfully after a bounded
readiness wait. The service remained active/running with zero restarts, the
listener was present on port `18080`, unauthenticated access remained `401`,
and `PRAGMA quick_check` returned `ok`.

The first installation attempt rolled back safely because an immediate listener
probe ran before listener readiness. No product or schema fault was observed.

Nginx, frontend files and credential configuration were not changed. The only
planned database change was the additive browser-grant table and index.

## Completed persisted browser-grant acceptance

**VERIFIED on 2026-07-29 at `47adb6577511209bfe7288ce8ce0fbe03b53a94c`:**

```text
Installed daemon SHA-256:
652dfc6a29f466fca977d34587db8a39bbc631509b735e02f8dd1942c46088e1

Grant table after startup: present
Grant rows after startup: 0
Primary key: actor_id,permission,backend_id
Active-grant index: present

Browser-session issuance: 200
Empty-grant ordinary GET: 200
Browser business POST with empty grants:
  503 security_policy_not_migrated

Persisted grants:
  remote.control@default
  remote.control@phase62-acceptance-other

Default scope revoked independently: verified
Alternate scope preserved: verified
All active acceptance grants revoked: verified

Grant table unavailable:
  503 permission_grants_unavailable
Same session after table recovery:
  200

Logout: 204
Revoked-cookie replay:
  401 credential_revoked

Verifier/session/credential revocation: verified
Acceptance grant rows after cleanup: 0
PRAGMA quick_check: ok
Foreign-key violations: 0
Accountability append failures: 0
```

Persisted grants did not enable a browser mutation prematurely. Every
browser-authenticated business POST remains blocked until explicit route
classification and server-side CSRF enforcement are implemented.

## Completed public-origin acceptance

**VERIFIED:**

```text
/vdr-suite          -> 308 /vdr-suite/frontend/
/vdr-suite/         -> 308 /vdr-suite/frontend/
/vdr-suite/frontend -> 308 /vdr-suite/frontend/

Public yaVDR /api/                       -> 307
Public Suite /vdr-suite/api/vdr/status  -> 401 unauthenticated
Direct daemon /api/vdr/status           -> 401 unauthenticated
```

Public `/api/*` remains yaVDR-owned. Unauthenticated Suite HTML and assets return
`401` by design because the security gate runs before static delivery.

## Credential state

**VERIFIED:**

- managed-Basic username and password hash are configured;
- the managed-Basic plaintext password is unavailable;
- a non-default explicit `VDR_SUITE_BASIC_AUTH` is configured;
- its value must never be printed or committed;
- configuration rollback owner is
  `/root/.vdr-suite-phase62-auth-backup`.

Do not rotate Legacy Basic or Managed Basic again for this completed acceptance
path.

## Accountability blocker and accepted fix

The pre-fix daemon produced one controlled
`POST /vdr-suite/api/security/browser-sessions` response with
`503 accountability_unavailable`. The database remained healthy, but the old
daemon did not preserve the concrete SQLite result code.

Repository analysis proved that the accountability append used the daemon's
shared SQLite connection without acquiring the transaction lease used by other
writers. The accepted fix:

```text
a2dd3f2689eedf08d6b0df46c587a637b977fd33
fix(security): serialize accountability persistence

c0497d45264b78936beabc17d022b98f568407cc
test(security): cover accountability transaction serialization

e3a8b7815c5df06093656f4724e6001d22c5755a
test(security): verify non-secret accountability diagnostics
```

The append now acquires the shared transaction lease, checks SQLite stages
independently, and reports only primary and extended result codes. Focused tests,
daemon build, CI, real installation, and authenticated runtime acceptance all
passed.

## Authenticated public-origin acceptance

Exactly one approved authenticated pass was run after installing the fixed
daemon.

**VERIFIED:**

```text
Browser-session issuance: 200
Set-Cookie:
  Path=/vdr-suite/
  Max-Age=28800
  HttpOnly
  Secure
  SameSite=Strict

Authenticated frontend index: 200
Authenticated public-url.js: 200
Authenticated app.js runtime bundle: 200
Authenticated Suite logo: 200
Authenticated /api/vdr/status: 200

Logout without CSRF: 403
Session after CSRF denial: 200
Valid cookie-plus-CSRF logout: 204
Expired cookie with Path=/vdr-suite/: verified
Revoked-cookie replay: 401
PRAGMA quick_check: ok
Accountability append failures: 0
Temporary acceptance artifacts: removed
```

No further browser-session issuance is required for this fix.

### Static app.js contract

`/frontend/app.js` is intentionally a runtime bundle composed of installed
`app.js`, two newline separators, and `modules/remote.js`. A direct byte
comparison against only the installed `app.js` source file is invalid. The
repository and installed `app.js` source files were independently proven
byte-identical.

### Rejected cookie-jar helper

An ad-hoc cookie-jar parser reported a false failure although the actual
`Set-Cookie` contract and all authenticated lifecycle behaviour passed. Do not
reuse that helper or repeat issuance merely to debug it.

## Anti-loop boundary

Do **not** repeat any of the following unless its directly relevant fingerprint
changed:

1. repository-wide Phase 62 or Slice 3A analysis;
2. Slice 3A frontend/Nginx installation;
3. Nginx include replacement, testing, reload, redirects, or provenance probes;
4. unauthenticated frontend/resolver probes or body byte comparisons;
5. Legacy-Basic or Managed-Basic credential rotation or password recovery;
6. accountability transaction-lease root-cause analysis;
7. daemon-only installation of the accepted accountability fix;
8. browser-session issue/CSRF/logout/replay acceptance for that fix;
9. persisted browser-grant schema, empty-result, backend-scope, unavailable-store, recovery, logout and cleanup acceptance;
10. direct response-to-single-file comparison for `/frontend/app.js`;
11. the rejected ad-hoc cookie-jar parser.

## Exact next action

The persisted browser-grant repository, daemon installation and real-runtime
acceptance are complete.

The next bounded work is repository planning:

1. keep PR #117 open and Draft;
2. commit and push this documentation closeout only after explicit approval;
3. update the PR description only after separate explicit metadata approval;
4. define one bounded business-route classification and browser-CSRF slice,
   beginning with `POST /api/vdr/remote/actions`;
5. preserve the existing legacy-Basic route behaviour while requiring a valid
   browser CSRF token before a browser-authenticated Remote mutation can reach
   authorization or dispatch;
6. keep every other browser-authenticated business POST fail-closed;
7. preserve the future ADR title
   `Client Login, Local Account Authority and Instance Access Model`;
8. do not add canonical ADR-0052 until open Draft PR #116's ADR-0051 numbering
   state is resolved or the branch is deliberately rebased.

No additional runtime mutation, credential rotation, Nginx change, PR-ready
transition, merge or auto-merge is implied.

## Secret restrictions

Never print or store Authorization headers, passwords, cookies, CSRF tokens,
password hashes, verifier material, login response bodies containing secrets, or
`/proc/.../environ`. Preserve only non-secret status and fingerprints.

## Maintenance rule

Update this file whenever repository, PR, runtime, routing, blocker, or
next-action truth changes. Preserve durable non-secret evidence, mark stale
statements, record the next approval boundary, and keep all secrets out of the
repository.
