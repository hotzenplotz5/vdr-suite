# Post-Phase-69 P1 Identity Model Audit

Date: 2026-09-29

## Purpose

This audit records the live repository state after completed Phase 69 and before
Human multiuser productization. It distinguishes implemented security
primitives from product concepts that do not yet exist.

It is evidence for [ADR-0065](../adr/ADR-0065-human-account-profile-device-identity-boundary.md).
It does not create runtime schema or claim that pending documentation PRs are
already merged.

## Live audit baseline

The audit started from live `main`
`c5e852ad8e8cda4a012d2f63a800799bbb58e4e8`.

At audit time:

- PR #388, the post-Phase-69 platform-documentation alignment, was open and not
  merged at exact head `080af7d600cae3e481054fbab2c8694313c0bddb`;
- PR #387, the unrelated HbbTV surface-lifecycle fix, was open at exact head
  `af03fd20729d16463b50c986c721d39f8d4ea9f0`;
- both PR heads had successful Hosted VDR-Suite CI;
- neither PR is treated as accepted `main` state by this audit.

The implementation facts below are taken from the audited `main` tree rather
than from those pending branches.

## Authority and persistence inventory

| Concept | Live implementation / authority | Product gap |
|---|---|---|
| Security Actor | `ActorIdentity`, `security_actors`, `SecurityIdentityRepository` | Actor is generic; no Human Account entity |
| Actor Type | `Anonymous`, `User`, `Service`, `Agent`, `System` | Type `User` does not prove a Human Account |
| Human User / Account | none | canonical `accountId` and Account-to-Actor binding missing |
| Profile | none | household/profile ownership and profile-scoped media state missing |
| Credential | `security_credentials` bound to Actor | Human-account credential administration missing |
| Credential verifier | `security_basic_credential_verifiers` | Managed Basic is a bootstrap/compatibility-era login mechanism, not a Human Account product |
| Device | `security_devices` bound to Actor | independent paired-device policy/shared-TV relationship missing |
| Session | `security_sessions` | administration/read model optional and not public-v1 |
| Browser Session | `security_browser_session_credentials` plus identity session/credential rows | generic app/device session contract not yet a public client contract |
| Role | fixed grant names `role.admin`, `role.read-only` | no general role-definition/role-assignment product |
| Permission | server-side `AuthorizationService` permission strings | broader admin vocabulary may grow with product slices |
| Grant | `security_actor_permission_grants` | no general Account/Profile administration API |
| Backend Scope | `backend_id` on grants | generalized content/folder/channel-group scope persistence missing |
| Accountability | append-only `accountability_events` | protected administration APIs for audit remain deferred |
| Agent identity/trust | same Actor/Device/Credential authority plus `backend_agents` and enrollment/rotation tables | not Human Account authority |
| Federation Actor | accepted ADR architecture | independent Control-Plane federation runtime still pending |
| Capability | backend/platform/media capability models | intentionally not permission |

### Repository ownership

The Control Plane owns the canonical security database state.

`SecurityIdentityRepository::ensureSchema()` creates and owns:

- `security_actors`;
- `security_devices`;
- `security_sessions`;
- `security_credentials`.

`CredentialVerifierRepository::ensureSchema()` owns:

- `security_basic_credential_verifiers`.

`SecurityPermissionGrantRepository::ensureSchema()` owns:

- `security_actor_permission_grants`.

`BrowserSessionCredentialRepository::ensureSchema()` owns:

- `security_browser_session_credentials`.

`AccountabilityEventRepository::ensureSchema()` owns:

- `accountability_events`, including append-only update/delete triggers.

Phase-63 Agent repositories add enrollment, Agent binding, credential generation,
capability and observation state in the same Suite database. Agent provisioning
reuses `SecurityIdentityProvisioningRepository` and the common Actor/Device/
Credential repositories.

There is no second user/account/profile database to preserve.

## Identity semantics

### Actor != Human Account

ADR-0013 says that Actor is intentionally broader than User. The runtime matches
that design.

The compatibility principal `legacy-local-web` and the Managed Basic principal
can both be `ActorType::User`, but no Account row exists that proves a human
identity.

Therefore:

```text
Actor != automatically Human Account
ActorType::User != automatically Human Account
```

A future Account read model cannot safely project every User Actor as an
Account.

### Human Account != Profile

No security/account Profile table or `profile_id` exists in the identity
runtime. Current code hits named `presentation_profile_id` and
`profileId` are media presentation/codec profiles, not people.

Current Recently Watched / Continue Watching persistence is Actor-scoped. It is
not evidence of per-Profile state.

### Device trust != User identity

`security_devices` has an owning `actor_id`; lifecycle checks ensure the
device belongs to the authenticated Actor.

That is a useful current binding, but it does not yet express a separately
trusted shared television plus a selected Human Account/Profile. The future
pairing/device policy layer must therefore extend the same authority rather than
pretend that the Device is the User.

### Capability != Permission

`AuthorizationService` evaluates permissions and backend scope. Backend/media
capability models answer technical availability. The two are independent.

## Authentication and security-mode audit

### Legacy Basic remains the actual default

`SecurityConfiguration` currently defaults to:

```text
mode = LegacyBasicCompatibility
Authorization = Basic YWRtaW46dmRyLXN1aXRl
actorId = legacy-local-web
deviceId = legacy-browser
sessionId = legacy-basic-session
credentialId = legacy-basic-credential
grants = *@*
```

The embedded Basic value decodes to `admin:vdr-suite`.

`VDR_SUITE_SECURITY_MODE` defaults to `legacy-basic` when no environment
override is present. This is therefore a live runtime default, not merely a
historical test credential.

### Package/source install behavior

The packaged systemd unit sets:

```text
EnvironmentFile=-/etc/default/vdr-suite-daemon
VDR_SUITE_DATABASE_PATH=/var/lib/vdr-suite/vdr-suite.db
```

The staged install copies `packaging/systemd/vdr-suite-daemon.default` to
`/etc/default/vdr-suite-daemon` only if that file does not already exist.

That defaults file currently contains no `VDR_SUITE_SECURITY_MODE` override.
Consequently the packaged daemon also inherits the C++ `legacy-basic` default.

A source-run daemon with no security override inherits the same
`SecurityConfiguration` default. Package and source therefore share the same
current security-mode lifecycle even though the production package supplies a
persistent database path and deployment configuration file.

### Managed Basic

Managed Basic is opt-in through environment configuration:

- `VDR_SUITE_MANAGED_BASIC_USERNAME`;
- `VDR_SUITE_MANAGED_BASIC_PASSWORD_HASH`;
- managed Actor/Device/Session/Credential IDs;
- managed permission grants.

At startup the verifier is persisted in
`security_basic_credential_verifiers`, keyed to a persistent
`security_credentials` row. The plaintext password is not persisted by that
repository.

`ManagedBasicAuthenticator` accepts libcrypt yescrypt (`$y$`) and SHA-512
crypt (`$6$`) verifiers. Yescrypt is suitable as the current generation
target for new Human Account password credentials; SHA-512 crypt remains
compatibility input and must not be the new-account generation target.

### Browser sessions

Browser login requires an already authenticated active Actor, Device and issuing
Credential.

Issuance creates:

- a new `security_sessions` row;
- a new `security_credentials` row of type `browser-session`;
- a `security_browser_session_credentials` row bound to the issuing credential.

Current policy:

- absolute lifetime default: 28,800 seconds / 8 hours;
- allowed absolute range: 300-86,400 seconds;
- active-session limit default: 0, compatibility-unlimited; maximum configured
  value: 64;
- idle timeout default: 0/off; enabled range: 300-86,400 seconds;
- retention default: 0/off; enabled retention range: 86,400-31,536,000 seconds.

Session and CSRF secrets are independently generated from kernel entropy. Only
salted hashes are stored. The current session-secret implementation uses
SHA-512 crypt with 10,000 rounds. This is an opaque high-entropy session-secret
verifier, not the policy for new human-chosen passwords.

The cookie is `HttpOnly; Secure; SameSite=Strict`. CSRF is returned separately
on successful login and browser mutations are checked server-side.

Issuer lifecycle remains bound at request time: revoking the issuing credential
invalidates the derived browser authentication path.

### Revocation

Actor, Device, Session and Credential lifecycle state is persisted and checked
during authentication/authorization.

The repositories support revocation and terminal cleanup. Browser logout revokes
the Session/Credential and expires the browser cookie.

Backend Agent enrollment/credentials add their own one-time enrollment,
generation, rotation and revocation state while reusing the common security
identity authority.

## Roles, permissions and scopes

Phase 62 intentionally implemented exactly two fixed role semantics:

- `role.admin`;
- `role.read-only`.

They are stored as ordinary persisted Actor grant rows. There is no general role
definition table or role-membership table.

`AuthorizationService` keeps permission and backend scope server-side.
`role.read-only` blocks the defined mutation permissions for its exact backend
scope. `role.admin` expands only the explicit server-known permission classes;
it is not a magical frontend flag.

The persisted generic scope today is `backend_id`. ADR-0013/ADR-0061 describe
richer resource scopes, but a normalized persisted content-scope dimension is
not yet implemented.

## Accountability linkage

`accountability_events` records immutable security/audit evidence including:

- Actor ID and Actor type;
- Device ID;
- Session ID;
- authentication state;
- permission and backend;
- operation/request/correlation IDs;
- action, decision, reason and outcome.

The current event struct does not carry a separate Human `accountId` or
`profileId`, because those product identities do not yet exist.

Future Account/Profile administration must preserve Actor accountability rather
than rewriting historical Actor identity.

## Agent identity and trust

Backend Agents are technical Actors, not users.

Agent enrollment stores only a bounded token hash in the Control-Plane database;
raw enrollment material is not the long-term database credential. The enrolled
Agent receives its own Actor, Device and Credential identity, backend binding and
credential generation. Rotation/revocation are fenced by that lifecycle.

ADR-0041 explicitly forbids treating Agent credentials as unrestricted user or
administrator credentials.

## Federation identity

ADR-0060 and ADR-0061 define a paired remote VDR-Suite as a federation Actor with
directional local grants and optional delegated remote-user identity.

The live audit found no implemented Human Account/federation-user persistence
that would make this a current Human Account source. Independent Control-Plane
federation remains architecture/implementation-pending and must reuse, not
replace, local authorization authority.

## Bootstrap gap

The current installation can start with permanent compatibility credentials and
full wildcard grants. It does not yet have:

- an unclaimed-server state;
- a one-time local operator bootstrap command;
- short-lived setup-code persistence/redemption;
- first Human Account creation;
- first-admin grant creation tied to that Account;
- permanent bootstrap invalidation;
- product recovery flow.

This is the concrete gap between current Phase-62 compatibility and a safe
fresh-install multiuser product.

The target bootstrap is bound by ADR-0065 and must be introduced as a later
focused runtime/deployment slice with upgrade protection.

## Pairing reuse audit

Useful existing building blocks for future TV/app pairing are already present:

- stable generic Actor identity;
- stable Device identity and revocation;
- stable Credential identity and revocation;
- Session lifecycle;
- one-time Agent enrollment as a proven pattern for hashed short-lived
  enrollment material;
- server-side permission grants;
- append-only accountability.

What is not yet present:

- pairing request/challenge resource;
- human approval flow;
- QR/human code contract;
- independently revocable paired-device policy intersected with a Human Actor;
- generic app/device credential type and public authentication contract;
- public administration read/mutation models for paired devices.

Therefore no complete pairing stack is justified in P1.

## Root-cause / architecture proof

The missing multiuser product is not caused by absence of security primitives.
The primitives are already substantial and persistent.

The actual root cause is a missing product identity layer between generic
Security Actor and Human Account/Profile semantics.

Creating an Account API before defining that layer would force one of two wrong
models:

1. alias every User Actor to a Human Account, including compatibility
   principals; or
2. introduce a second account database beside the Phase-62 authority.

ADR-0065 rejects both.

## First justified slice

The first justified P1/P2 boundary is this documentation/ADR slice.

No runtime code is changed because the audit proves that a binding Human
Account-to-Actor decision is a prerequisite for an honest Account read model.

After ADR-0065 is accepted, the smallest runtime successor is a bounded
Human-Account persistence/read-model foundation inside the existing Suite
database:

- explicit `accountId` -> primary User Actor binding;
- no password mutation yet unless required by that same slice;
- no Profile table yet;
- no pairing yet;
- no content-scope invention;
- no second database;
- read model contains no verifier/secret material;
- server-side administrative authorization;
- revision/accountability semantics follow existing contracts;
- public-v1 exposure only when the stable representation is justified.

That successor is not implemented by this P1 slice.

## Documentation consistency finding

Current `main` already states in several places that Phase 69 is complete, but
also retains stale lines that describe 69.F or 69.E as active. PR #388 corrects
those strategic inconsistencies and adds the P0-P10 productization wording, but
it was not merged at this audit baseline.

This audit therefore does not duplicate or overwrite PR #388's nine changed
files. It remains valid before or after that PR because it records the audited
baseline explicitly and binds only the newly proven Identity distinction.

## Acceptance boundary

This slice changes architecture/documentation validation only.

It does not:

- create Human Account/Profile tables;
- expose `/api/v1/accounts`;
- change authentication defaults;
- remove Legacy Basic;
- create default credentials;
- implement bootstrap;
- implement pairing;
- alter frontend behavior;
- start Phase 70.
