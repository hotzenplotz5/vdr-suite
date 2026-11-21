# ADR-0067: Human Account and Backend Access Administration

## Status

Accepted architecture.

Date: 2026-10-01

Acceptance evidence:
[MU.5 Administration Architecture Acceptance](../development/post-phase69-mu5-administration-architecture-acceptance.md).

This ADR accepts the administration architecture only. It does not itself claim
MU.6 or any later runtime implementation.

## Context

VDR-Suite now has:

- one persistent Suite security authority;
- explicit Human Account persistence and Account-to-User-Actor binding;
- stable read-only Public-v1 Account discovery guarded by `accounts.view@*`;
- normal Human Account password authentication and browser sessions;
- First Admin bootstrap/claim;
- local audited Human Account recovery;
- retired Legacy Basic runtime compatibility.

ADR-0065 deliberately left Account mutation and grant administration for later
bounded work. ADR-0066 completed bootstrap/recovery and the authentication
migration required before Legacy Basic retirement.

The missing Multiuser product boundary is now administration: an authorized
administrator must be able to manage Human Accounts and their effective backend
access without creating another identity authority, exposing credentials or
letting the browser invent policy.

The MU.5 live-code audit also establishes two concrete implementation facts on
the accepted baseline:

1. `HumanAccountRepository` owns explicit Account persistence/read but has no
   general Account lifecycle mutation service;
2. `SecurityPermissionGrantRepository` already owns normalized
   `actor + permission + backend` ensure/revoke persistence, so grant
   administration must reuse it rather than create another ACL store.

The audit also found a security-relevant lifecycle gap that this ADR freezes
before MU.6: Human-password login already checks effective Human Account active
state, while an already-issued browser session currently resolves from its
session/issuing-credential lifecycle without consulting
`security_human_accounts.active`. Account deactivation therefore must not be
implemented as a lone Account-row flag update.

## Decision

### One security authority remains authoritative

Account administration extends the existing Suite security persistence and
services. It must not introduce:

- a second account database;
- a frontend-owned permission store;
- package-file user accounts;
- direct HTTP/UI SQLite mutation;
- VDR/SuiteBridge identities as Human Account authority.

A Human Account continues to be an explicit product identity bound to one
existing `ActorType::User` Actor. Generic User Actors are never synthesized into
Accounts.

The Account active flag remains a Human Account lifecycle state. Deactivation
does not silently retype, revoke or repurpose the bound generic security Actor.

### Frozen administration permission vocabulary

The following normalized permissions are the accepted Multiuser administration
vocabulary.

Read permissions:

```text
accounts.view
accounts.grants.view
accounts.credentials.view
accounts.sessions.view
```

Mutation permissions:

```text
accounts.create
accounts.modify
accounts.activate
accounts.deactivate
accounts.grants.modify
accounts.credentials.revoke
accounts.sessions.revoke
```

All of these administration permissions use global authorization scope `*`.
The backend ID carried by a managed grant is target data; it is not the scope of
the administrator's own authority to modify Human Account access.

Consequences:

- `accounts.view@*` remains read-only Account discovery;
- `accounts.modify@*` changes Account metadata such as `displayName`; it does
  not change login names, credentials, grants or active state;
- activation and deactivation are separate permissions because they alter login
  availability;
- grant inspection is separate from grant mutation;
- credential/session inspection is separate from revocation;
- there is no generic `accounts.write` permission;
- there is no public password-reset/credential-rotation permission in this ADR.

An explicit `role.admin@*` may satisfy these administration permissions through
the existing `AuthorizationService` role vocabulary when each permission is
wired into the appropriate read/mutation family.

A backend-scoped `role.admin@backend-id` must **not** satisfy any global
`accounts.*@*` administration request. Backend-local administration therefore
never becomes global Human Account administration by implication.

A matching `role.read-only@*` must continue to deny all accepted
`accounts.*` mutation permissions through the normal mutation-policy path.

### Account creation is one atomic Human Account transaction

The first bounded remote Account creation contract is authorized by
`accounts.create@*`.

One successful create transaction atomically provisions:

- one server-generated Human Account ID;
- one server-generated `ActorType::User` Actor;
- one active `human-password` credential;
- one unique login verifier for that credential;
- one active Human Account binding to that Actor;
- successful secret-free accountability evidence.

The initial password is request-only material. It is hashed with the existing
yescrypt human-password boundary, wiped after use and never returned, logged or
persisted in plaintext/reversible form.

Account creation does **not** grant `role.admin`, `role.read-only` or backend
permissions automatically. Access is a separate MU.7 administration decision.

The first bounded contract may use an administrator-supplied initial password.
User-owned invite/password-setup and self-service password-change workflows are
later explicit product contracts, not hidden requirements for MU.6.

A repeated Account-create delivery must not create a second Human Account after
a response is lost. MU.6 therefore requires a bounded durable create
idempotency binding keyed by authenticated actor plus `Idempotency-Key`. The
binding stores only the resulting non-secret resource identity and normalized
non-secret request fields; it must not persist the submitted password or a
reversible/password-derived request fingerprint. A replay of the same key and
same non-secret request returns the original creation result without
reprocessing or changing the password. Reuse with different non-secret fields
is an idempotency conflict.

This is a synchronous Class-A mutation. Account creation does not require a
Timer-style asynchronous operation, Agent job, dispatch lifecycle or saga.

### Account lifecycle mutation is revision-safe

A mutable Human Account has a monotonic persisted Account revision. The stable
API representation exposes that revision only as an opaque resource revision /
strong ETag.

`accounts.modify@*`, `accounts.activate@*` and
`accounts.deactivate@*` require the caller's observed Account revision before
mutation. A stale revision fails with the existing public precondition/conflict
semantics rather than silently overwriting concurrent administration.

The Account revision changes when Account-owned lifecycle/metadata changes, at
minimum:

- `displayName`;
- active/inactive state.

Grant, credential and session lifecycle do not silently mutate the Account
revision; those resources own their own concurrency/state contract.

These are synchronous authoritative Class-A mutations. No durable asynchronous
operation lifecycle is introduced merely because ADR-0042 mutation safety
applies.

### Deactivation takes effect immediately

`accounts.deactivate@*` is not a cosmetic Account-row change.

In the same authoritative mutation boundary, deactivation must:

1. set the Human Account inactive and advance its Account revision;
2. prevent all subsequent Human Account password login;
3. revoke all still-active browser sessions owned by the Account's bound Actor,
   including their canonical Session/browser-Credential lifecycle state;
4. append secret-free accountability evidence.

Browser-session authentication/resolution must also consult current effective
Human Account state for Human Account sessions. An inactive, missing or invalid
Human Account binding must fail closed even if a stale browser-session row
survived cleanup or crossed a race boundary.

Reactivation:

- does not revive revoked browser sessions;
- leaves the existing normal human-password credential unchanged unless another
  credential lifecycle operation changed it;
- requires a new normal login/session after activation.

The bound generic User Actor is not deactivated merely to represent Account
inactive state.

### Final usable administrator is protected transactionally

Remote administration must never remove the final usable Human Account
administrator.

For this invariant, a **usable administrator** is one explicit Human Account
for which all of the following are true at the mutation transaction boundary:

- the Human Account is active;
- its bound Actor exists, is `ActorType::User`, active and not revoked;
- the Actor has an active `role.admin@*` grant;
- the Actor owns at least one active, unexpired, unrevoked
  `human-password` credential;
- at least one login verifier resolves to such a credential.

Any administration mutation that would make the post-commit count of usable
administrators zero must fail closed **inside the same database transaction**
before commit.

This applies at least to:

- Human Account deactivation;
- revocation of `role.admin@*`;
- revocation of the final usable human-password credential;
- any later administration path capable of revoking/deactivating the bound
  Actor or its final login authority.

The accepted local root/operator Human Account recovery path can rotate a
credential but does not silently restore a removed administrator grant.
Therefore local recovery is not a substitute for this invariant.

### Grant administration reuses the normalized grant model

Backend/content access administration reuses
`security_actor_permission_grants` and ADR-0061 operation/resource scope
semantics.

`accounts.grants.view@*` may inspect the supported normalized grants for an
explicit Human Account/Actor.

`accounts.grants.modify@*` may ensure or revoke a supported grant tuple:

```text
target actor
+ normalized permission
+ explicit backend/resource scope
```

The administration service, not the generic persistence repository, owns the
allowlist of grant permission/scope combinations that may be exposed as product
administration. Arbitrary strings accepted by the low-level repository are not
thereby valid public permissions.

Grant-set reads expose an opaque revision/ETag derived deterministically from the
normalized effective grant set for the target Actor. Grant mutation is a
synchronous desired-state mutation:

- stale grant-set state fails closed unless the requested target tuple is
  already in the requested final state;
- repeated ensure of an already-active identical grant is successful and does
  not create a duplicate;
- repeated revoke of an already-revoked/absent identical grant is terminal and
  does not resurrect state;
- no MutationOperation/Agent/saga lifecycle is introduced.

A mutation that changes `role.admin@*` must apply the final-usable-admin
invariant above in the same transaction.

### Credential and session administration is metadata-safe

`accounts.credentials.view@*` and `accounts.sessions.view@*` expose only
bounded safe lifecycle metadata needed for administration.

They must never return:

- password verifier hashes;
- browser session secrets/cookies;
- CSRF values or hashes usable as credentials;
- bootstrap secrets;
- raw reusable credentials.

`accounts.credentials.revoke@*` and `accounts.sessions.revoke@*` are
terminal synchronous lifecycle mutations. Repeating an already-terminal revoke
returns the terminal state rather than recreating authority.

Revoking a human-password credential must also fence/revoke browser sessions
issued from that credential through the existing canonical lifecycle services.
Revoking the final usable administrator credential is forbidden by the
final-usable-admin invariant.

Normal password recovery/rotation remains the accepted local audited
root/operator boundary from ADR-0066. This ADR does not introduce remote
administrator password reset, password disclosure or self-service password
change.

### Public API and browser boundary

Independent clients consume stable `/api/v1` contracts. The browser
administration UI is a client of the same server-owned authority; it is not a
privileged direct database console.

The existing read-only `/api/v1/accounts` collection remains a stable
secret-free Account discovery contract. Later administration resources may add
Account item, grant, credential and session representations, but must not
silently broaden that collection to expose login/verifier/session secrets.

Public administration contracts must:

- apply authenticated server-side authorization before dispatch;
- use global `*@*` administration scope exactly as defined above;
- use CSRF protection for browser mutations;
- use strong revision/precondition semantics for mutable resources;
- preserve Phase-69 compatibility/error/request-correlation semantics;
- keep backend/resource scope explicit in grant representations;
- return secret-free resources.

Local root/operator recovery remains a separate local administration contract
and is not converted into ordinary Public-v1 CRUD.

### Accountability

Successful and denied administrative mutations append secret-free accountability
evidence sufficient to answer:

- who requested the mutation;
- which Human Account/Actor was targeted;
- which normalized permission/scope changed where applicable;
- whether the action succeeded, conflicted or was denied;
- which revision/lifecycle boundary was crossed where applicable;
- whether final-administrator protection rejected the request.

Passwords, password hashes, browser cookies, CSRF material and bootstrap secrets
never enter accountability payloads.

## Planned implementation sequence

With this ADR accepted, the bounded Multiuser continuation is:

1. **MU.6 — Human Account lifecycle administration**
   - Account revision/item read;
   - atomic Account + initial human-password creation;
   - display-name mutation;
   - activate/deactivate;
   - immediate session invalidation on deactivate;
   - final-usable-admin guard for Account lifecycle.
2. **MU.7 — Backend access / permission grant administration**
   - grant-set read/revision;
   - supported ensure/revoke;
   - global administration authorization;
   - final-usable-admin guard for `role.admin@*`.
3. **MU.8 — Safe credential and session administration**
   - secret-free metadata;
   - bounded credential/session revoke;
   - issuer/session fencing;
   - final-usable-admin guard for human-password revocation.
4. **MU.9 — Account and access browser administration UI**
   - first-party UI over the accepted stable server contracts.

Device/app pairing and Profiles remain later, separate slices under ADR-0065.

## Consequences

The broad Timer Product UI remains gated until required Account/backend access
administration is usable.

Multiuser administration may proceed independently of the numbered runtime
sequence.

MU.6 is the first justified runtime successor after this architecture
acceptance. Its implementation must remain Class A/local-authority unless live
evidence proves a larger failure model.

Phase 70 Recommendation and Content Knowledge Graph remains not started and is
not authorized by this ADR.

## Non-goals

This ADR does not itself:

- implement MU.6 runtime Account mutation;
- implement MU.7 grant mutation;
- implement MU.8 credential/session administration;
- implement an administration frontend;
- expose password hashes or reusable secrets;
- implement remote administrator password reset;
- implement self-service password change;
- hard-delete Human Accounts;
- implement Profiles;
- implement TV/app pairing;
- implement federation;
- start Phase 70;
- replace local audited recovery;
- introduce a second identity or policy authority.
