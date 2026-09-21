# Phase 62 Slice 3A Runtime Checkpoint

## Purpose

This checkpoint records the durable, non-secret repository and real-yaVDR state
for Phase 62 Slice 3A. It prevents later chats or operators from repeating
completed installation, routing, credential, authenticated-session, or
accountability diagnosis work.

Read the canonical [new-chat handoff](../NEW-CHAT-HANDOFF.md) first.

Never store plaintext passwords, Authorization headers, cookies, CSRF tokens,
raw browser-session values, password hashes, verifier material, login response
bodies containing secrets, or process environments in this document.

## Status vocabulary

- **RUNTIME VERIFIED**: observed on the real yaVDR runtime.
- **REPOSITORY VERIFIED**: demonstrated by code analysis, tests, and CI.
- **OPEN**: not yet implemented or accepted.
- **DO NOT REPEAT**: completed and not useful to repeat unless a directly
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
Runtime-accepted source head: 47adb6577511209bfe7288ce8ce0fbe03b53a94c
Accountability fix code head: e3a8b7815c5df06093656f4724e6001d22c5755a
Fix validation CI: run 6497, successful
Persisted browser-grant code/runtime head: 47adb6577511209bfe7288ce8ce0fbe03b53a94c
Persisted browser-grant CI: run 6507, successful
```

The branch remains Draft. Do not mark it ready, merge it, enable auto-merge, or
mutate review state without explicit approval.

## Installed runtime

**RUNTIME VERIFIED on 2026-07-29:**

```text
Original Slice 3A backup:
/var/backups/vdr-suite-phase62-slice3a-20260729-122447

Pre-fix daemon backup:
/var/backups/vdr-suite-phase62-accountability-fix-20260729-162026
SHA-256: 85b12e38a9f23fde7ae84c9773914b39874ca81edec5ea4ccecd997d77b3dc02

Installed current daemon:
/usr/sbin/vdr-suite-daemon
SHA-256: 652dfc6a29f466fca977d34587db8a39bbc631509b735e02f8dd1942c46088e1

Slice 2C pre-install daemon/database backup:
/var/backups/vdr-suite-phase62-slice2c-20260729-171704

Installed frontend index:
/usr/share/vdr-suite/web/frontend/index.html
SHA-256: d5dc42df979b61115c3cf49e5682971cee9acfd1b11d8511fc751c86d30a75a8

Installed frontend app.js:
/usr/share/vdr-suite/web/frontend/app.js
SHA-256: 33c7c92a86747cfebea66e79f0d972723471aac90a986751e90b3aa8d9fbf93a

Installed public URL runtime:
/usr/share/vdr-suite/web/frontend/platform/public-url.js
SHA-256: 7c9dbc35646e857cfd31b0ebf1f220fb94e1ef48e17c62909d745ee63e97a5c5

Installed Nginx snippet:
/etc/nginx/snippets/vdr-suite.conf
SHA-256: b9f7114d35fcd79a49604da195f1f2c340c4d7f1f66bed24a448b349791acb01
```

The current daemon is installed. `vdr-suite-daemon.service` completed the
bounded readiness check, remained active with zero recorded restarts, listened
on port `18080`, and the database `PRAGMA quick_check` returned `ok`.

No Nginx, frontend or credential configuration mutation was part of the
Slice 2C daemon-only installation. The grant table and index were created
additively with zero default grant rows.

## Active Nginx and public-origin state

**RUNTIME VERIFIED:**

- the active site is `/etc/nginx/sites-enabled/default`;
- exactly one include of `snippets/vdr-suite.conf` is active;
- the loaded configuration contains the canonical redirects and
  `location ^~ /vdr-suite/`;
- the proxy target is `http://127.0.0.1:18080/` with prefix stripping;
- public root `/api/*` remains owned by yaVDR;
- Nginx was not changed or reloaded for the accountability fix.

Canonical redirects:

```text
/vdr-suite          -> 308 /vdr-suite/frontend/
/vdr-suite/         -> 308 /vdr-suite/frontend/
/vdr-suite/frontend -> 308 /vdr-suite/frontend/
```

Namespace provenance:

```text
Public yaVDR /api/                       -> 307
Public Suite /vdr-suite/api/vdr/status  -> 401 when unauthenticated
Direct daemon /api/vdr/status           -> 401 when unauthenticated
```

Unauthenticated Suite HTML and assets return `401` by design because the
security gate runs before static delivery.

## Credential state

**RUNTIME VERIFIED:**

- the managed-Basic username and password hash are configured;
- the earlier managed-Basic plaintext password is unavailable;
- an explicit non-default `VDR_SUITE_BASIC_AUTH` is configured;
- the credential value is deliberately not recorded;
- configuration rollback owner remains
  `/root/.vdr-suite-phase62-auth-backup`.

Do not rotate Legacy Basic or Managed Basic again for this completed acceptance
path.

## Historical blocker and repository diagnosis

The pre-fix daemon returned one controlled
`POST /vdr-suite/api/security/browser-sessions` response with:

```text
HTTP status: 503
error code: accountability_unavailable
error message: Security accountability persistence is unavailable
```

No browser session was issued by that failed attempt. The database remained
healthy, and the old daemon did not expose the concrete SQLite result code.

Repository analysis established that:

1. `BrowserSessionHttpGate` persists accountability before issuance;
2. the daemon shares one `Database` object across HTTP and background workers;
3. transactional writers use `Database::acquireTransactionLease()`;
4. the old accountability append wrote through that connection without the
   lease;
5. the old path collapsed prepare, bind, step, and finalize failures to `false`.

This was a concrete concurrency and observability defect consistent with the
observed `503`. The exact old-runtime SQLite result code remains unknown and is
not guessed.

## Repository fix

**REPOSITORY VERIFIED:**

```text
a2dd3f2689eedf08d6b0df46c587a637b977fd33
fix(security): serialize accountability persistence

c0497d45264b78936beabc17d022b98f568407cc
test(security): cover accountability transaction serialization

e3a8b7815c5df06093656f4724e6001d22c5755a
test(security): verify non-secret accountability diagnostics
```

The fixed append acquires the shared transaction lease, checks prepare/bind/
step/finalize independently, and logs only SQLite primary and extended result
codes. Regression coverage proves that a concurrent append waits for lease
release and that diagnostics contain no event or actor identifier.

## Daemon-only installation acceptance

**RUNTIME VERIFIED on 2026-07-29:**

Preconditions:

```text
Focused accountability repository test: exit 0
Daemon build: exit 0
Built daemon SHA-256:
1dcc23439685240407faa7113dd2c2c0754b6d03c3fb61ec4f1026adf7e01832
Installed pre-fix daemon SHA-256:
85b12e38a9f23fde7ae84c9773914b39874ca81edec5ea4ccecd997d77b3dc02
Repository head:
efafac6a6f06ae207371fa537955a3b613510ed4
Working tree: clean and tracking the remote branch
```

Post-installation checks:

```text
Installed daemon SHA-256:
1dcc23439685240407faa7113dd2c2c0754b6d03c3fb61ec4f1026adf7e01832
Service: active/running
Listener 18080: present
Public unauthenticated frontend status: 401
PRAGMA quick_check: ok
Rollback: not required
```

## Authenticated public-origin acceptance

Exactly one approved authenticated acceptance pass was executed after installing
the fixed daemon.

**RUNTIME VERIFIED:**

```text
Browser-session issuance: 200
CSRF response contract: valid
Set-Cookie contract:
  Path=/vdr-suite/
  Max-Age=28800
  HttpOnly
  Secure
  SameSite=Strict

Authenticated frontend index: 200
Authenticated public-url.js: 200
Authenticated app.js bundle: 200
Authenticated Suite logo: 200
Authenticated /api/vdr/status: 200

Logout without CSRF: 403
Session after CSRF denial: still valid, 200
Valid cookie-plus-CSRF logout: 204
Expired cookie contract: valid with Path=/vdr-suite/
Revoked-cookie replay: 401
PRAGMA quick_check: ok
Accountability append failures during acceptance: 0
Temporary acceptance artifacts: removed
```

The fixed accountability append therefore permits issuance under the real
concurrent daemon runtime and remains fail-closed without generating an
accountability persistence error.

## Persisted browser-grant runtime acceptance

Exactly one controlled browser-session pass was executed after installing the
Slice 2C daemon.

**RUNTIME VERIFIED on 2026-07-29 at `47adb6577511209bfe7288ce8ce0fbe03b53a94c`:**

```text
Installed daemon SHA-256:
652dfc6a29f466fca977d34587db8a39bbc631509b735e02f8dd1942c46088e1

Grant schema:
  additive table present
  active-grant index present
  actor_id,permission,backend_id primary key
  zero startup/default rows

Browser-session issuance: 200
Empty-grant ordinary GET: 200
Empty-grant business POST:
  503 security_policy_not_migrated

Persisted backend scopes:
  remote.control@default
  remote.control@phase62-acceptance-other

Independent default-scope revocation: verified
Alternate scope preservation: verified
All active acceptance grants revoked: verified

Grant table unavailable:
  503 permission_grants_unavailable
Same session after recovery:
  200

Cookie-plus-CSRF logout: 204
Revoked-cookie replay:
  401 credential_revoked
Verifier/session/credential revocation: verified

Acceptance grant cleanup: zero rows
PRAGMA quick_check: ok
Foreign-key violations: zero
Accountability append failures: zero
```

The test did not dispatch a Remote operation. Active browser grants remain
intentionally unable to bypass the unmigrated browser-POST and CSRF boundary.

### Static asset clarification

`/frontend/app.js` is intentionally not byte-identical to the installed
`app.js` file. The daemon serves a runtime bundle composed of:

```text
app.js
+ two newline separators
+ modules/remote.js
```

The installed and repository `web/frontend/app.js` files were separately proven
byte-identical with SHA-256
`33c7c92a86747cfebea66e79f0d972723471aac90a986751e90b3aa8d9fbf93a`.
Do not treat a direct response-to-single-file byte comparison as a failure.

### Cookie-jar helper clarification

One ad-hoc cookie-jar parsing assertion reported `FAILED`, although the actual
`Set-Cookie` header contract passed and the same jar authenticated all expected
frontend/API requests, survived the CSRF denial, completed logout, and then
failed replay with `401`. The helper assertion was therefore not a valid product
failure and must not be reused as acceptance evidence. No second issuance was
performed merely to debug that helper.

## Anti-loop boundary

The following work is **DO NOT REPEAT** unless its own directly relevant
fingerprint changes:

1. repository-wide Phase 62 or Slice 3A architecture analysis;
2. Slice 3A frontend or Nginx installation;
3. Nginx include replacement, testing, or reload;
4. canonical redirects and namespace-provenance probes;
5. unauthenticated frontend or resolver probes;
6. Legacy-Basic or Managed-Basic credential rotation or recovery attempts;
7. accountability append/transaction-lease root-cause analysis;
8. daemon-only installation of the accepted fix;
9. another browser-session issuance or lifecycle acceptance for the accountability fix;
10. persisted browser-grant schema, empty-result, backend-scope, unavailable-store, recovery, logout or cleanup acceptance;
11. direct byte comparison of `/frontend/app.js` with only the installed
    `app.js` source file;
12. reuse of the rejected ad-hoc cookie-jar parser.

## Exact next action

The Slice 3A public-origin, accountability and persisted browser-grant runtime
blocks are complete.

The next bounded Phase 62 work is repository planning:

1. keep PR #117 open and Draft;
2. close out the runtime evidence in repository documentation;
3. update PR metadata only after explicit approval;
4. classify `POST /api/vdr/remote/actions` as the first bounded
   browser-authenticated business mutation;
5. require browser CSRF before Remote permission/scope authorization and
   dispatch;
6. leave every other browser-authenticated business POST fail-closed;
7. do not introduce roles, grant administration, frontend login, ADR-0052 or
   later-phase runtime into that slice.

No new runtime mutation, browser-session issuance, credential rotation, Nginx
change, PR-ready transition, or merge is implied by this next action.
