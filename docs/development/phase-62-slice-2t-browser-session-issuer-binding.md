# Phase 62 Slice 2T — Browser-Session Issuing-Credential Lifecycle Binding

## Status

Repository implementation, focused verification, all five source CI jobs
and guarded real yaVDR runtime acceptance are complete.

Accepted implementation/runtime head:

```text
55876356e84b3e47e52911529b3f9bfa0e17f191
```

Accepted source CI:

```text
VDR-Suite CI #6666
Run ID: 30719552024
All five jobs successful
```

This documentation-only closeout commit and its CI are the remaining
closeout gate.

PR #117 remains open, Draft and unmerged.

---

## Why this slice follows Slice 2S

Slice 2S completed bounded post-operation accountability for browser-session
issue and revoke. A fresh post-2S lifecycle audit then traced every identity
checked during ordinary-route browser authentication and logout.

The existing `PersistentIdentityResolver` already revalidates the browser actor,
device, canonical session and browser credential. The remaining independent gap
is narrower: issuance stores `issued_from_credential_id` and validates that
credential at issue time, but later cookie and CSRF verification previously read
only the browser-session credential row.

A browser session could therefore survive later revocation, expiry, inactivation,
removal or actor reassignment of the credential that issued it.

---

## Exact Scope

Slice 2T binds every browser-session cookie to the current lifecycle of its
issuing credential.

`BrowserSessionCredentialRepository` adds one effective lookup:

```text
findResolvedByTokenId(tokenId)
```

The lookup reads the browser row and its `issued_from_credential_id` in one
SQLite statement using a left join to `security_credentials`.

An effective browser session is accepted only when the issuing credential:

- still exists;
- belongs to the same actor as the browser-session row;
- is active;
- is not revoked;
- is not expired.

Effective state mapping:

```text
issuer expired
  -> browser authentication state Expired

issuer missing, actor-mismatched, inactive or revoked
  -> browser authentication state Revoked
```

The existing browser-session expiry and revocation checks remain unchanged.

---

## Raw and Effective Persistence Boundary

The existing raw repository methods retain stored-row semantics:

```text
findByTokenId
findBySessionId
```

They do not synthesize issuer state and do not mutate rows.

The new resolved lookup is used only by the authentication path. Revoking an
issuer does not cascade writes into descendant browser rows in Slice 2T. This
preserves auditability and avoids introducing cleanup or retention policy.

There is no schema migration. The existing `issued_from_credential_id` foreign
key remains the lineage source.

---

## Request-Path Contract

Both secret-bearing browser checks use the effective lookup:

1. ordinary-route browser authentication;
2. CSRF verification for protected browser mutations and logout.

When the issuer is no longer valid:

- cookie authentication fails before grants or route authorization;
- ordinary routes return HTTP 401 with the established
  `credential_revoked` or `credential_expired` taxonomy;
- logout is denied as an authentication failure before CSRF evaluation;
- no Basic fallback occurs when the browser cookie is present;
- the raw browser row remains unchanged until separately revoked or cleaned up.

The existing `PersistentIdentityResolver` remains responsible for actor, device,
canonical session and browser-credential lifecycle checks. Slice 2T does not
duplicate that ownership.

---

## Verification Contract

The focused C++ test covers:

- active issuer acceptance for direct authentication and CSRF verification;
- active ordinary GET through `SecurityHttpGate`;
- active logout gate through `BrowserSessionHttpGate`;
- issuer revocation while the raw browser row remains active;
- effective revoked state and `credential_revoked` denial for GET and logout;
- logout denial before CSRF evaluation;
- issuer expiry mapping to `AuthenticationState::Expired`;
- issuer inactivation mapping to revoked;
- issuer actor mismatch mapping to revoked;
- missing issuer mapping to revoked;
- unknown token rejection;
- secret-free denial accountability.

A dedicated static architecture checker enforces repository, authenticator,
test, Make and documentation ownership.

---

## Accepted Real-Runtime Acceptance

The guarded pass ran against the real yaVDR installation after all five
source CI jobs were green.

It:

1. verified the exact local and remote runtime head;
2. backed up the installed runtime and SQLite database;
3. installed and started the accepted daemon;
4. issued one isolated browser test session;
5. proved ordinary-route browser authentication before issuer invalidation;
6. created one disposable same-actor issuer credential in revoked state;
7. redirected only the test browser row's
   `issued_from_credential_id`;
8. proved the raw browser row, canonical session and browser credential
   remained active and unrevoked before cleanup;
9. proved ordinary GET and logout failed with `credential_revoked`;
10. proved logout failed before CSRF verification;
11. fully revoked the test browser lifecycle and denied replay;
12. verified append-only accountability, database integrity and active
    service state;
13. left the original compatibility issuer unchanged;
14. performed zero VDR domain mutations.

Accepted results:

```text
service_pid_before=69610
service_pid_after_install=73034
service_pid_after_acceptance=73034
runtime_http_requests=5
login_http_status=200
active_get_http_status=200
revoked_issuer_get_http_status=401
revoked_issuer_logout_http_status=401
revoked_cookie_replay_http_status=401
login_authorized_events=1
login_succeeded_events=1
revoked_get_denials=1
revoked_logout_denials=1
revoked_logout_csrf_events=0
revoked_logout_operation_events=0
replay_denials=1
issuer_revocation_effective_without_cascade=yes
original_issuer_unchanged=yes
test_browser_session_revoked=yes
test_browser_credential_revoked=yes
accountability_secret_free=yes
vdr_domain_mutations=0
database_quick_check=ok
database_foreign_key_check=empty
service_state=active
automatic_rollback=not-required
```

Installed fingerprints:

```text
daemon:
34b80de4fd8f55b763c4483f0dcb50ee09e5cdc49de7f6e7c25e01ba50d84269

deferred-runtime-loader.js:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
```

Durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2t-20260801T223353Z-55876356e84b/runtime-acceptance-slice2t
```

Runtime report SHA-256:

```text
2ca7fcaefe21c1198e5d8ff88b3e17237b2e72a545780cc14f0200e7dd0ca983
```

The pass did not revoke or alter the original compatibility issuer and did
not contact or mutate VDR domain state.

---

## Explicitly Deferred

## Explicitly Deferred

Slice 2T does not implement:

- cascading descendant-session revocation writes;
- browser-session idle timeout or `last_seen` persistence;
- sliding expiry or refresh;
- expired-session cleanup or retention;
- concurrent-session limits;
- security administration APIs;
- credential enrollment, rotation or management UI;
- broader operation outcomes or a transactional outbox;
- route, permission, frontend or packaging changes;
- Android clients;
- Phase 63-67 runtime;
- PR Ready transition or merge.

## Acceptance Gate

All Slice-2T implementation and runtime gates passed:

1. the repository diff remained within the issuing-credential binding
   boundary;
2. focused and architecture tests passed;
3. all five PR source CI jobs passed;
4. guarded real yaVDR issuer-revocation acceptance succeeded;
5. the runtime test session and browser credential were revoked;
6. durable evidence is secret-free and hash-addressed;
7. database integrity and active service state were preserved;
8. no VDR domain mutation occurred.

The remaining closeout gate is documentation-only:

1. publish this bounded documentation closeout;
2. require all five closeout CI jobs;
3. keep PR #117 open, Draft and unmerged.

No next Phase-62 implementation slice is selected by this closeout. A fresh
post-2T gap analysis must occur only after full closeout CI.
