# P2 First-Admin Bootstrap Boundary Audit

## Baseline

Audited against live `main`:

```text
4920d0e0fe531a66da697fd73bd91306c649fa1d
```

The Public-v1 Account collection is already on `main`. This slice deliberately does not modify its runtime, API, client or Phase-69 compatibility surfaces.

## Current deployment facts

The packaged daemon unit sources:

```text
EnvironmentFile=-/etc/default/vdr-suite-daemon
```

The packaged defaults file contains operational daemon settings but no first-admin claim lifecycle.

`SecurityConfiguration` still falls back to:

```text
VDR_SUITE_SECURITY_MODE -> legacy-basic
```

and the compatibility credential remains available when no migration configuration is supplied.

## Current persistence facts

The Suite security database already owns:

- Actors;
- Human Accounts;
- Devices;
- Sessions;
- Credentials;
- Basic credential verifiers;
- permission grants;
- accountability events.

`SecurityIdentityProvisioningRepository` can create identity rows and `CredentialVerifierRepository` persists one-way password verifiers.

These are reusable primitives, but neither repository defines first-admin claim ownership, single-use bootstrap lifetime, claim atomicity or local recovery policy.

## Missing product lifecycle

Current main does not provide a canonical lifecycle equivalent to:

```text
fresh install
 -> unclaimed
 -> local operator bootstrap issuance
 -> short-lived setup proof
 -> first Human Account + admin grants
 -> claimed
 -> bootstrap permanently invalid
```

There is also no canonical local Human Account recovery contract that can replace a lost credential without revealing the stored verifier.

## Root cause

The remaining blocker to retiring the compatibility default is not missing password verification.

It is missing lifecycle authority around:

- whether the server is claimed;
- who may issue setup authority;
- how bootstrap material expires and becomes single-use;
- how first Human Account creation is atomic;
- how administrator grants are established;
- how an operator recovers access;
- how upgrades avoid locking out existing installations.

Therefore changing `legacy-basic` today would be a deployment behavior change without a safe replacement path.

## First justified runtime successor

With ADR-0066 accepted, the smallest runtime slice is persistent claim/bootstrap state in the existing Suite database with:

- explicit unclaimed/claimed determination;
- short-lived bootstrap verifier metadata;
- single-use/expiry state;
- no plaintext bootstrap secret persistence;
- no HTTP endpoint yet;
- no authentication-default change yet.

The local issuance command and atomic first-admin claim service follow as separate slices.


## Implemented persistence foundation

This slice adds `FirstAdminBootstrapRepository` to the existing Suite security
database and initializes its schema from the daemon runtime.

The repository keeps bootstrap material separate from normal identity
credentials. It stores only a bootstrap identifier, a one-way verifier hash,
expiry and terminal lifecycle timestamps. It creates no normal Session or
Credential row and therefore does not turn bootstrap proof into reusable API
authority.

Server claim state remains derived from the existing persistent identity/account
authority: an active Human Account bound to an active `ActorType::User` actor
with global administrator authority (`role.admin@*` or the existing direct
wildcard grant). There is no second persisted claim-state flag.

Registration is fenced against an already-claimed server, rejects already
expired material and permits at most one effective unconsumed/uninvalidated
bootstrap issuance at a time. Lookup becomes fail-closed once the server is
claimed.

Consumption and invalidation require a caller-owned SQLite transaction. This is
intentional: the later atomic first-admin claim service can consume bootstrap
proof and create the Human Account, normal human credential, grants and
accountability evidence inside one transaction; rollback restores the
pre-claim bootstrap state instead of leaving a partial admin.

The Legacy Basic default remains unchanged. This slice adds no HTTP endpoint,
no local issuance command, no first-admin account mutation and no recovery
workflow.

## Implemented local trusted-operator bootstrap issuer

This slice adds the local trusted-operator bootstrap issuer without opening any
remote or browser-facing authority.

`FirstAdminBootstrapIssuanceService` generates a 128-bit bootstrap identifier,
a 256-bit raw setup secret and an independent verifier salt from Linux
`getrandom(2)`. The raw setup secret is returned only in the successful
in-memory issuance result; the repository receives only the bootstrap ID,
one-way verifier hash and expiry.

The installed `vdr-suite-first-admin-bootstrap` command is root-only and uses
the existing Suite security database by default. It prints the raw setup secret
once to the local operator, then wipes its in-memory result. The command writes
no secret file and adds no HTTP endpoint, browser session, normal credential or
permission grant.

Bootstrap lifetime is deliberately short and bounded to 300..3600 seconds with
a 900-second default. Existing repository fencing remains authoritative for an
already-claimed server and for an already-active bootstrap issuance.

The Legacy Basic default remains unchanged.

## Implemented atomic first-admin claim service

This slice adds the internal atomic first-admin claim service while keeping the
browser and HTTP boundary closed.

`FirstAdminClaimService` accepts the short-lived bootstrap proof plus the
requested first Human Account login, password and display name. It generates
opaque account, actor, credential and accountability identifiers from Linux
`getrandom(2)`. New human passwords are salted and hashed with yescrypt
(`$y# P2 First-Admin Bootstrap Boundary Audit

## Baseline

Audited against live `main`:

```text
4920d0e0fe531a66da697fd73bd91306c649fa1d
```

The Public-v1 Account collection is already on `main`. This slice deliberately does not modify its runtime, API, client or Phase-69 compatibility surfaces.

## Current deployment facts

The packaged daemon unit sources:

```text
EnvironmentFile=-/etc/default/vdr-suite-daemon
```

The packaged defaults file contains operational daemon settings but no first-admin claim lifecycle.

`SecurityConfiguration` still falls back to:

```text
VDR_SUITE_SECURITY_MODE -> legacy-basic
```

and the compatibility credential remains available when no migration configuration is supplied.

## Current persistence facts

The Suite security database already owns:

- Actors;
- Human Accounts;
- Devices;
- Sessions;
- Credentials;
- Basic credential verifiers;
- permission grants;
- accountability events.

`SecurityIdentityProvisioningRepository` can create identity rows and `CredentialVerifierRepository` persists one-way password verifiers.

These are reusable primitives, but neither repository defines first-admin claim ownership, single-use bootstrap lifetime, claim atomicity or local recovery policy.

## Missing product lifecycle

Current main does not provide a canonical lifecycle equivalent to:

```text
fresh install
 -> unclaimed
 -> local operator bootstrap issuance
 -> short-lived setup proof
 -> first Human Account + admin grants
 -> claimed
 -> bootstrap permanently invalid
```

There is also no canonical local Human Account recovery contract that can replace a lost credential without revealing the stored verifier.

## Root cause

The remaining blocker to retiring the compatibility default is not missing password verification.

It is missing lifecycle authority around:

- whether the server is claimed;
- who may issue setup authority;
- how bootstrap material expires and becomes single-use;
- how first Human Account creation is atomic;
- how administrator grants are established;
- how an operator recovers access;
- how upgrades avoid locking out existing installations.

Therefore changing `legacy-basic` today would be a deployment behavior change without a safe replacement path.

## First justified runtime successor

With ADR-0066 accepted, the smallest runtime slice is persistent claim/bootstrap state in the existing Suite database with:

- explicit unclaimed/claimed determination;
- short-lived bootstrap verifier metadata;
- single-use/expiry state;
- no plaintext bootstrap secret persistence;
- no HTTP endpoint yet;
- no authentication-default change yet.

The local issuance command and atomic first-admin claim service follow as separate slices.


## Implemented persistence foundation

This slice adds `FirstAdminBootstrapRepository` to the existing Suite security
database and initializes its schema from the daemon runtime.

The repository keeps bootstrap material separate from normal identity
credentials. It stores only a bootstrap identifier, a one-way verifier hash,
expiry and terminal lifecycle timestamps. It creates no normal Session or
Credential row and therefore does not turn bootstrap proof into reusable API
authority.

Server claim state remains derived from the existing persistent identity/account
authority: an active Human Account bound to an active `ActorType::User` actor
with global administrator authority (`role.admin@*` or the existing direct
wildcard grant). There is no second persisted claim-state flag.

Registration is fenced against an already-claimed server, rejects already
expired material and permits at most one effective unconsumed/uninvalidated
bootstrap issuance at a time. Lookup becomes fail-closed once the server is
claimed.

Consumption and invalidation require a caller-owned SQLite transaction. This is
intentional: the later atomic first-admin claim service can consume bootstrap
proof and create the Human Account, normal human credential, grants and
accountability evidence inside one transaction; rollback restores the
pre-claim bootstrap state instead of leaving a partial admin.

The Legacy Basic default remains unchanged. This slice adds no HTTP endpoint,
no local issuance command, no first-admin account mutation and no recovery
workflow.

## Implemented local trusted-operator bootstrap issuer

This slice adds the local trusted-operator bootstrap issuer without opening any
remote or browser-facing authority.

`FirstAdminBootstrapIssuanceService` generates a 128-bit bootstrap identifier,
a 256-bit raw setup secret and an independent verifier salt from Linux
`getrandom(2)`. The raw setup secret is returned only in the successful
in-memory issuance result; the repository receives only the bootstrap ID,
one-way verifier hash and expiry.

The installed `vdr-suite-first-admin-bootstrap` command is root-only and uses
the existing Suite security database by default. It prints the raw setup secret
once to the local operator, then wipes its in-memory result. The command writes
no secret file and adds no HTTP endpoint, browser session, normal credential or
permission grant.

Bootstrap lifetime is deliberately short and bounded to 300..3600 seconds with
a 900-second default. Existing repository fencing remains authoritative for an
already-claimed server and for an already-active bootstrap issuance.

The Legacy Basic default remains unchanged.

) before persistence; plaintext passwords and bootstrap secrets are
cleared from the service-owned request copy and are never stored.

The service acquires the existing per-database transaction lease and opens one
`BEGIN IMMEDIATE` SQLite transaction. Inside that single transaction it:

1. proves the server is still unclaimed;
2. loads and verifies the unexpired, unconsumed bootstrap verifier;
3. consumes the bootstrap proof;
4. creates or binds the `ActorType::User` actor and normal
   `human-password` credential through the existing identity repository
   authority;
5. creates or binds the explicit Human Account through
   `HumanAccountRepository`;
6. stores the one-way credential verifier;
7. grants `role.admin@*`;
8. appends the successful claim accountability event; and
9. commits.

Bootstrap consumption intentionally happens before the administrator grant:
once that grant exists the canonical claim-state projection becomes
`claimed`. Because all writes are in the same transaction, any later failure
rolls back the bootstrap consumption together with actor, account, credential,
verifier, grant and accountability writes. Regression coverage forces an
accountability persistence failure after the earlier mutations and proves that
no partial administrator survives.

The service adds no direct identity-table SQL outside the existing repository
owners and creates no parallel identity authority.

The Legacy Basic default remains unchanged.

## Next bounded slice

Browser completion remains the next justified slice. It may expose a narrowly
scoped first-admin completion route that accepts only valid bootstrap claim
material, calls this atomic service, and establishes normal post-claim
authentication/session state only after claim success.

That later HTTP slice must not turn bootstrap material into a browser session or
general API credential. Local recovery and authentication-default migration
remain later ADR-0066 slices.
