# MU.6A Human Account Lifecycle Authority Foundation

## Status

Completed by this bounded runtime slice.

Baseline live `main` at slice start:

```text
63c2dd0fdfa3b9c22b46ccef66eca23fe4d6b79a
```

The slice started with no open pull requests.

## Purpose

MU.6 implements the Human Account lifecycle administration boundary accepted by
ADR-0067. MU.6A deliberately establishes the server-side authoritative lifecycle
owner before Public-v1 mutation routes are added.

This slice does **not** expose Account mutation over HTTP and does **not** create
later Human Accounts yet.

## Runtime delivered

### Persisted Account revision

`security_human_accounts` now owns a monotone persisted `revision`.

Existing databases are migrated additively:

```text
revision INTEGER NOT NULL DEFAULT 1
```

The Human Account read model carries that revision internally.

Repository mutation requires the caller's expected revision and returns an
explicit revision conflict rather than silently overwriting concurrent state.

### Transactional display-name mutation

`HumanAccountAdministrationService` owns display-name mutation.

One transaction keeps these representations aligned:

```text
security_human_accounts.display_name
security_actors.display_name
```

A stale revision fails before commit.

### Transactional activation/deactivation

The lifecycle service owns active-state mutation.

Activation/deactivation is a synchronous Class-A Suite-database mutation under
ADR-0063; no Timer/Agent/job/saga lifecycle is introduced.

Deactivation:

1. checks the expected Account revision;
2. enforces the final-usable-administrator invariant;
3. sets the Human Account inactive and advances its revision;
4. revokes all still-active browser-session rows for the bound Actor;
5. revokes the canonical Session and browser-session Credential state;
6. appends secret-free accountability evidence;
7. commits all effects atomically.

Reactivation advances the Account revision but does not restore revoked browser
sessions.

### Final usable administrator protection

The deactivation transaction counts usable Human Account administrators using
the accepted ADR-0067 definition:

- active Human Account;
- active, non-revoked User Actor;
- active `role.admin@*` grant;
- active, unexpired, non-revoked `human-password` Credential;
- a login verifier bound to that Credential.

If the target is the final usable administrator, deactivation is denied and the
denial is audited without changing Account state.

### Browser-session defense in depth

`BrowserSessionAuthenticator` now optionally resolves current Human Account
state.

Production HTTP composition supplies the canonical `HumanAccountRepository`.
For a browser session whose Actor is an explicit Human Account:

- inactive Account -> session authentication is revoked;
- Account lookup/storage failure -> authentication fails closed;
- no Human Account binding -> existing generic/Managed-Basic Actor behavior is
  preserved.

This is intentionally in addition to transactional session revocation. A stale
browser-session row therefore cannot keep an inactive Human Account usable.

## Tests

The focused lifecycle service test proves:

- initial revision `1`;
- display-name mutation advances revision and keeps Actor display name aligned;
- stale revision is rejected;
- final usable administrator deactivation is rejected;
- a second usable administrator unblocks deactivation;
- deactivation revokes browser-session row, canonical Session and browser
  Credential;
- reactivation does not revive revoked sessions;
- already-active desired state is idempotent;
- revision-conflict, final-admin denial and successful deactivation are audited.

The browser-session authenticator test separately proves current Human Account
state is consulted even when the browser-session row itself remains active.

## MU.6 continuation

MU.6 remains in progress after MU.6A.

Next bounded sub-slices:

```text
MU.6A Account lifecycle authority foundation            [DONE]
MU.6B Public Account item + revision/ETag               [NEXT - NOT STARTED]
MU.6C Public display-name / activate / deactivate       [PLANNED]
MU.6D Atomic Account CREATE + durable idempotency       [PLANNED]
```

MU.6B should expose a secret-free stable Account item using the revision already
owned by this slice. MU.6C then maps accepted global administration permissions
and strong preconditions onto the lifecycle service. MU.6D adds the separate
atomic Account + initial human-password creation contract and bounded durable
create idempotency accepted by ADR-0067.

MU.7 grant administration, MU.8 credential/session administration and MU.9 UI
remain separate later slices.

Phase 70 remains not started.
