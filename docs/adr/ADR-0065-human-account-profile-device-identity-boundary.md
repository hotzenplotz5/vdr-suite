# ADR-0065: Human Account, Profile and Device Identity Boundary

## Navigation

- [ADR Index](index.md)
- [ADR-0013 Permission Model](ADR-0013-permission-model.md)
- [ADR-0037 Packaging, Install Layout and API Boundary](ADR-0037-packaging-install-api-boundary.md)
- [ADR-0041 Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0048 Public API Versioning, Error and Compatibility Contract](ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0049 Audit and Security Event Model](ADR-0049-audit-security-event-model.md)
- [ADR-0060 Federated VDR-Suite Sharing and Reciprocal Site Trust](ADR-0060-federated-vdr-suite-sharing-reciprocal-site-trust.md)
- [ADR-0061 Actor Permissions, Federation and Client Access](ADR-0061-actor-permissions-federation-client-access.md)
- [P1 Identity Model Audit](../development/post-phase69-p1-identity-model-audit.md)

---

## Status

Accepted architecture / implementation pending.

Date: 2026-09-29

## Context

Phase 62 already created one persistent Suite security authority for actors, devices,
sessions, credentials, credential verifiers, permission grants and accountability.
Phase 63 reused that authority for Backend Agent identity and trust.

That foundation deliberately did not define a complete human-user/account schema.
ADR-0013 is explicit that an Actor is broader than a User, while ADR-0041 leaves
the complete user database schema out of scope.

The live post-Phase-69 audit proves that this distinction is now a product
boundary rather than terminology:

- `security_actors` contains generic authorization/accountability principals;
- `ActorType::User` is also used by compatibility and managed-Basic principals;
- there is no canonical Human Account table or Account-to-Actor binding;
- there is no household/profile identity model;
- `security_devices` currently belongs to one Actor and therefore does not yet
  model the future intersection of independently revocable device trust and
  human-user authority;
- permission persistence is Actor plus backend scope, while content scope is not
  yet a general persisted grant dimension;
- capability is technical availability and is not authorization.

Publishing an `/api/v1/accounts` representation by relabeling existing Actors
would therefore create a false public contract.

## Decision

### One persistent identity authority

VDR-Suite keeps one Control-Plane identity authority in the existing Suite
persistent database and security repository boundary.

Human Accounts, future Profiles, human credentials, grants, registered devices,
sessions and bootstrap state extend that authority. They do not create a second
identity database, package-owned account store or frontend-owned identity store.

`/etc/vdr-suite` and `/etc/default/vdr-suite-daemon` remain deployment and
system configuration. They are not the product database for normal human
passwords, Accounts, Profiles, Grants, Devices or Sessions.

### Security Actor is not Human Account

A Security Actor is the stable principal used for authentication,
authorization and accountability.

An Actor may represent:

- a human account principal;
- a service or API client;
- a Backend Agent;
- a future paired remote VDR-Suite;
- another bounded technical identity.

A Human Account is a product identity for one human administrator or user. A
Human Account has its own stable Suite `accountId` and binds to exactly one
primary `ActorType::User` authorization principal. That binding is explicit and
persistent.

The converse is not true: an `ActorType::User` row is not sufficient evidence
that a Human Account exists. Compatibility principals must never become Human
Accounts merely because their Actor type is `User`.

Service, Agent, System and federation Actors do not require Human Account rows.

### Human Account is not Profile

A Profile is a later household/media persona owned by a Human Account or by a
future explicitly defined household relationship. It is not an authentication
principal and does not own a password or silently receive grants.

One Account may own zero or more Profiles. Profile-scoped playback/history/media
state therefore cannot be implemented by renaming `actor_id`.

Profile selection is authenticated Account/session context. Authorization still
derives from the authenticated Actor and server policy.

### Credential, Session and Device remain distinct

A Credential authenticates a principal. Password verifier material remains a
salted one-way verifier and is never returned by an administration read model.

A Session is revocable, expiring authentication state. Browser sessions retain
their existing issuer-credential binding, independent session/CSRF secrets,
absolute lifetime, optional idle expiry and retention policy.

A Device is a separately revocable client/device identity. Device trust is not
Human Account identity and is not Profile identity.

The current `security_devices.actor_id` ownership model is sufficient for the
existing browser/Agent foundations, but it is not declared sufficient for the
future shared-TV pairing model. P6 may add explicit device policy/binding state
inside the same identity authority so that authorization can evaluate a human
Actor and an independently trusted device without converting the Device into the
User.

### Permission, scope and capability remain separate

Permission grants continue to be server-enforced and Actor-owned.

The existing persisted scope dimension is `backend_id`. Future content scopes
such as Recording folders or Channel groups require an explicit persisted scope
model; they are not encoded into capabilities or display labels.

Effective client authorization follows this conceptual intersection:

```text
human/user actor permission
INTERSECT device policy when applicable
INTERSECT backend/content policy
INTERSECT technical capability
```

Capability answers whether a server/backend/client can technically perform an
operation. Capability never grants permission.

### Human password verifier policy

The existing Managed Basic verifier repository accepts yescrypt (`$y$`) and
SHA-512 crypt (`$6$`) for compatibility.

New Human Account password creation must use a modern salted, deliberately
password-oriented one-way verifier. On the current libcrypt boundary, newly
generated Human Account passwords use yescrypt; SHA-512 crypt is compatibility
input only and is not the generation target for new Human Accounts. A later
well-supported stronger password KDF may replace the generation target without
changing Account identity.

Plaintext or reversibly encrypted normal human passwords are never persisted in
the Suite database, `/etc/vdr-suite`, environment files, logs, API responses or
accountability events.

### Fresh-install bootstrap

The current permanent compatibility default `admin:vdr-suite` is not the
target fresh-install product model.

The target lifecycle is:

```text
fresh installation
-> unclaimed server
-> local root/operator starts a one-time bootstrap
-> short-lived setup code is shown through the local operator channel
-> server persists only bounded verifier/state needed to redeem it
-> trusted Web setup creates the first Human Account
-> the Human Account's User Actor receives the initial administrator grant
-> bootstrap material is consumed and permanently invalidated
```

Bootstrap material is not a normal user credential, is short-lived, is
single-use, is auditable and is never a permanent default. It does not live as a
normal password in `/etc/vdr-suite`.

Upgrade/migration from an existing Legacy Basic installation is a separate
deployment migration problem. Fresh-install hardening must not silently lock out
existing installations.

Recovery follows the same ownership rule: a local, short-lived, audited operator
flow may recover administration. Manual SQLite editing is not a product
workflow.

### Pairing reuses, rather than replaces, identity

Future TV/app pairing reuses the same Actor/Credential/Device/Session authority.

A pairing request or QR code is short-lived approval material, not a durable
credential. After approval the server registers a revocable device identity and
issues only the credential class defined for that device/client.

Pairing alone grants no administrator permission and no content permission.
User/account authorization, device policy and resource policy remain separately
revocable.

### Public administration boundary

A stable public Account administration resource may be introduced only after a
canonical Human Account record and explicit Account-to-Actor binding exist.

It must not synthesize Accounts from every `ActorType::User` row.

Administration read models expose only non-secret identity/lifecycle metadata,
stable revisions and effective safe grant summaries. Credential hashes, session
secrets, CSRF secrets, bootstrap material and raw authorization headers never
appear.

Mutations follow the existing server-side authorization, precondition,
idempotency and accountability contracts.

## Consequences

The existing Phase-62 security tables and repositories remain authoritative and
are extended rather than replaced.

The first runtime productization work after this decision may introduce the
minimal Human Account persistence/read boundary inside the existing database.
It must not add Profile, pairing, device-policy or credential-mutation behavior
unless that behavior is required by the same bounded slice.

Per-profile media state remains a later productization boundary. Personalized
recommendation work must not treat current Actor-scoped playback/history state
as already Profile-scoped.

Federation remains a peer/site Actor problem under ADR-0060/ADR-0061. A paired
remote Suite does not become a Human Account.

## Rejected alternatives

### Treat every User Actor as a Human Account

Rejected because compatibility and managed-Basic principals already use
`ActorType::User`, and Actor is deliberately broader than Human Account.

### Add a second users/profiles database

Rejected because it would split credential, grant, session and accountability
ownership across competing authorities.

### Store normal passwords in deployment configuration

Rejected because deployment configuration is not the Human Account credential
authority and should not contain reusable normal-user plaintext secrets.

### Make Device identity the User identity

Rejected because device trust and human authority require independent lifecycle
and revocation.

### Use capability as permission

Rejected by ADR-0013 and ADR-0061. Technical capability never grants access.

## Acceptance

This architecture slice is complete when:

1. the live P1 audit records the current implementation and persistence facts;
2. the ADR index assigns ADR-0065 canonically;
3. architecture checks protect Actor/Account/Profile/Device/Capability
   distinctions and the one-authority rule;
4. no runtime schema, account endpoint, bootstrap flow or pairing stack is
   invented by this documentation-only slice;
5. Hosted CI is green on the exact branch head.

Runtime implementation remains separately reviewable.
