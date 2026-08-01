# Phase 62 Slice 2T — Browser-Session Issuing-Credential Lifecycle Binding

## Status

Repository implementation is prepared. Full CI and guarded real yaVDR runtime
acceptance remain pending.

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

## Planned Real-Runtime Acceptance

Real-runtime acceptance may run only after all five PR CI jobs are green.

The guarded pass will:

1. verify the exact accepted branch head and installed fingerprints;
2. back up the runtime and SQLite database;
3. install and start the accepted daemon;
4. issue one bounded test browser session;
5. create one disposable same-actor issuer credential in revoked state;
6. redirect only the test browser row's `issued_from_credential_id` to that
   disposable credential;
7. prove the raw browser row itself remains active and unrevoked;
8. prove ordinary browser GET and logout are denied with
   `credential_revoked` before CSRF dispatch;
9. prove denial accountability is secret-free;
10. revoke the test browser lifecycle rows, deny replay, verify database
    integrity and leave the service active.

The pass will not revoke a production or compatibility issuer and will not
contact or mutate VDR domain state.

---

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

Slice 2T is not complete until:

1. the repository diff remains within the issuing-credential binding boundary;
2. focused and architecture tests pass;
3. all five PR CI jobs pass;
4. guarded real yaVDR issuer-revocation acceptance succeeds;
5. the runtime test session is revoked and durable evidence is documented.
