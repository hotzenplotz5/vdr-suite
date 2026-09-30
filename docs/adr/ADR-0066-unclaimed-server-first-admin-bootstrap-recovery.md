# ADR-0066: Unclaimed Server, First-Admin Bootstrap and Local Recovery

## Status

Accepted architecture.

Date: 2026-09-29

## Context

VDR-Suite now has an explicit Human Account persistence/read foundation in the existing Suite security database, while Legacy Basic remains the runtime compatibility default when `VDR_SUITE_SECURITY_MODE` is absent.

The packaged daemon reads `/etc/default/vdr-suite-daemon`, but that file does not define a first-admin lifecycle. Managed Basic can provision a persistent verifier from environment configuration, yet it is still deployment configuration rather than a product claim/recovery workflow.

Changing the default from Legacy Basic to enforced authentication before a safe bootstrap and recovery contract exists would risk locking operators out of existing or fresh installations.

## Decision

### Server claim state

A fresh installation has an explicit security claim state:

```text
unclaimed
claimed
```

Claim state is derived from persistent Suite identity/account state, not from the presence of an environment variable, frontend cookie or package file.

An unclaimed server has no normal Human Account with administrator authority.

### Bootstrap authority

Only a local trusted operator may initiate first-admin bootstrap.

The initial product path is a root/operator-owned local command or equivalent local privileged mechanism. Remote anonymous callers cannot create bootstrap authority.

Bootstrap creates short-lived, single-purpose setup material. The raw secret is shown once to the local operator and is never persisted in plaintext.

### Bootstrap material

Bootstrap material:

- is cryptographically random;
- is short-lived;
- is single-use;
- is scoped only to completing first-admin claim;
- is stored only as a one-way verifier or equivalent non-recoverable proof;
- is invalidated after successful claim;
- cannot be converted into a reusable normal credential;
- cannot grant access after the server is already claimed.

The setup code is not a Human Account password and is not written to `/etc/default/vdr-suite-daemon` or another reusable package configuration file.

### First Human Account

Successful claim atomically creates or binds:

- one explicit Human Account;
- one `ActorType::User` actor;
- one normal human credential with a modern salted one-way password verifier;
- initial administrator grants;
- accountability evidence for the claim outcome.

All persistent identity state remains in the existing Suite security authority.

A partially created first admin must not survive failed claim completion.

### Authentication default migration

Legacy Basic retirement is a separate migration slice.

The runtime default must not switch to enforced/unclaimed behavior merely because this ADR is accepted.

A later migration may change fresh-install defaults only after:
- bootstrap runtime exists;
- first-admin claim succeeds end-to-end;
- local recovery exists;
- existing-install upgrade behavior is explicitly defined;
- rollback is proven.

### Local recovery

Local audited Human Account recovery is local-operator controlled and audited.

The implemented first recovery contract is a direct local reset of an explicitly
selected existing Human Account's normal `human-password` credential. It does
not introduce a second recovery store or issue another class of recovery secret.
The Account, its bound `ActorType::User` actor, the same `human-password`
credential and existing permission grants remain the persistent authorities.

The new password is stored only as a salted yescrypt verifier. The local
root/operator command never reveals the existing password or verifier and does
not accept replacement password plaintext as a command-line argument.

Changing only the verifier is insufficient session recovery. Existing browser
sessions retain `issued_from_credential_id`; while the same issuing
`human-password` credential remains active, those sessions would otherwise
remain valid. Recovery therefore revokes every still-active browser session
issued from the same human-password credential, including its browser-session
row, canonical Session and canonical browser Credential, in the same transaction
as verifier rotation and successful accountability evidence.

Recovery fails closed for an unknown or inactive Human Account, a non-user actor
binding, a missing credential, or any credential that is not the selected
account actor's active, unexpired and unrevoked `human-password` credential.
Technical, Legacy Basic and Managed Basic principals are not Human Account
recovery targets.

Remote anonymous recovery is forbidden. No Public-v1 recovery mutation is
opened, Bootstrap material is not reused, and recovery never silently creates a
second Human Account or administrator. Authentication-default migration remains
separate.

### Browser setup

A browser may complete first-admin setup only by presenting valid short-lived bootstrap material.

The browser receives normal post-claim authentication/session state only after successful account creation. Bootstrap material itself is not a browser session and does not inherit general API permissions.

### Public API boundary

Normal public-v1 account resources do not expose bootstrap or recovery secrets.

Bootstrap and recovery endpoints, if later added, are distinct security lifecycle contracts and are not ordinary Account CRUD.

## Consequences

The next runtime implementation can be divided into bounded slices:

1. persistent claim/bootstrap state and verifier repository;
2. local root/operator bootstrap issuance command;
3. atomic first-admin claim service;
4. trusted browser completion flow;
5. local audited recovery through direct selected Human Account credential reset;
6. only then fresh-install/default migration away from Legacy Basic.

## Non-goals

This ADR does not:
- change the current authentication default;
- remove Legacy Basic;
- implement a bootstrap endpoint;
- implement first-admin account creation;
- modify the open Public-v1 Account collection work;
- implement Profiles or pairing;
- create a second identity authority.
