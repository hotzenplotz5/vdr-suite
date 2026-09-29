# ADR-0065: Human Account, Profile and Device Identity Boundary

## Status

Accepted architecture.

Date: 2026-09-29

## Context

VDR-Suite already has one persistent security authority in the Suite database. The accepted Phase-62/63 implementation contains generic Actors, Devices, Sessions, Credentials, Basic credential verifiers, backend-scoped permission grants, browser-session credentials, append-only accountability evidence and Backend Agent trust/lifecycle state.

The existing Actor model is deliberately broader than a human login. `ActorType::User` is currently used by compatibility and Managed-Basic principals and therefore cannot be treated as proof of a Human Account.

The product now needs real multiuser administration without creating a second identity authority or conflating these concepts:

```text
Security Actor != Human Account
Human Account != Profile
Device trust != User identity
Capability != Permission
```

## Decision

### One identity authority

Human Accounts, their credential bindings, Devices, Sessions and permission Grants extend the existing Suite security persistence. No second identity database, package-owned account store or client-owned authorization authority is introduced.

### Human Account is explicit

A Human Account is a product identity with a stable account identifier and an explicit binding to one primary existing `ActorType::User` security Actor.

Generic User Actors are not automatically Human Accounts. Technical/compatibility principals remain Actors without becoming account resources.

A future Account read model must read only explicit Human Account records and their actor bindings. It must never synthesize accounts by enumerating all User Actors.

### Profile is separate

A Profile is a later household/media-personalization concept. It may be owned by or selectable by a Human Account, but it is not a credential, Actor, Device or permission Grant.

Per-profile playback/history/recommendation state must not be attached globally to a generic security Actor by assumption.

### Credentials and password verifiers

Human passwords are represented by credentials in the existing authority and stored only as salted one-way verifiers. Plaintext or reversibly encrypted human passwords under `/etc/vdr-suite` are forbidden as a product model.

The existing libcrypt verifier boundary accepts yescrypt (`$y$`) and SHA-512 crypt (`$6$`). New human password generation should target yescrypt; legacy accepted verifier formats are migration compatibility, not a reason to generate new weaker hashes.

### Installation/bootstrap

The target fresh-install lifecycle is:

```text
install
  -> unclaimed server
  -> local root/operator requests one-time bootstrap
  -> short-lived setup code
  -> trusted browser setup
  -> first Human Account with administrator grants
  -> bootstrap material permanently invalidated
```

`/etc/vdr-suite` remains deployment/system configuration. It is not the persistent store for normal human accounts or reusable human passwords.

Legacy Basic remains a compatibility mechanism until a separately implemented migration retires it. This ADR does not silently change current authentication defaults.

### Devices and pairing

Future TV/app pairing reuses the existing Actor/Credential/Device/Session authority.

A pairing request and QR/human code are short-lived bootstrap material only. They must not contain a durable device credential and approval must not imply administrator rights.

Effective authorization remains server-side:

```text
user permission
INTERSECT device policy
INTERSECT backend/content policy
INTERSECT capability availability
```

Capability describes technical ability such as codec, HDR or resolution; it never grants authorization.

### Public API boundary

Independent clients consume stable `/api/v1` contracts. Human-account administration must not expose private VDR, SuiteBridge, Agent, provider, RESTfulAPI or SVDRP identities as public client authority.

Explicit Human Account persistence now exists in the Suite security authority. A
stable read-only Account collection may therefore expose the secret-free Account
read model through `/api/v1`, provided the Security gate authorizes the global
`accounts.view@*` permission before dispatch. This does not authorize Account
mutation, credential exposure, Profile conflation or direct SQLite access from
the HTTP layer.

## Consequences

The first justified runtime successor is a minimal Human Account persistence/read foundation in the existing Suite database, with an explicit Account-to-Actor binding and no secrets in read models.

Profile, pairing, account mutation, grant administration and security-default migration are later bounded slices.

## Non-goals

This ADR does not implement:
- a second identity database;
- a public Account API;
- account mutation;
- Profiles;
- pairing;
- a new authentication default;
- first-admin bootstrap runtime;
- Phase 70 recommendations.
