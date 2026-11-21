# P1 Identity Model Audit

## Baseline

Audited against live `main` at:

```text
70028fd98b575334385c1d40c4b4206604a3ddea
```

Closed pull requests and their branches are not implementation authority. This audit uses the current main tree only.

## Existing persistent authority

The existing Suite database already owns:

| Concept | Current implementation |
| --- | --- |
| Actor | `security_actors`, `ActorType::{User,Service,Agent,System}` |
| Device | `security_devices` bound to actor |
| Session | `security_sessions` bound to actor/device |
| Credential | `security_credentials` bound to actor |
| Basic verifier | `security_basic_credential_verifiers` |
| Browser session credential | `security_browser_session_credentials` |
| Permission grant | `security_actor_permission_grants`, backend scoped |
| Accountability | append-only `accountability_events` |
| Agent identity/trust | existing Phase-63 actor/device/credential lifecycle |

These are not separate authorities; they are repositories over the same Suite persistence boundary.

## Actor versus Human Account

`ActorType::User` does not mean Human Account.

The Legacy-Basic authenticator uses a User actor. Managed Basic also provisions a User actor. Therefore exposing all User actors as account resources would convert compatibility/security principals into fake human users.

No explicit Human Account entity or Account-to-Actor binding exists on current main.

## Profile gap

No persistent Profile identity exists in the audited security authority. A future Profile must remain separate from login/account identity and from generic Actors.

## Credentials and sessions

Managed Basic persists login name plus password verifier in `security_basic_credential_verifiers`; the verifier is not plaintext.

The authenticator uses libcrypt and accepts yescrypt (`$y$`) and SHA-512 crypt (`$6$`). Browser sessions use separate one-way session/CSRF hashes, issuer binding, expiry/revocation, optional idle timeout and retention cleanup.

## Security default

`SecurityConfiguration` currently defaults to:

```text
mode = legacy-basic
Authorization = Basic YWRtaW46dmRyLXN1aXRl
decoded compatibility credential = admin:vdr-suite
permission = *@*
```

If `VDR_SUITE_SECURITY_MODE` is absent, the code fallback remains `legacy-basic`. Phase-62 closeout also records that packaged daemon defaults did not require migration to `enforced`.

This is therefore a real compatibility/install default on current main, not merely dead documentation.

The target product model is an unclaimed-server bootstrap flow; this audit does not change the live default because safe bootstrap/recovery runtime does not yet exist.

## Configuration ownership

Deployment/environment owns mode and compatibility bootstrap inputs such as `VDR_SUITE_SECURITY_MODE`, `VDR_SUITE_BASIC_AUTH` and Managed-Basic bootstrap variables.

Persistent security lifecycle state belongs in the Suite DB.

Normal future Human Accounts, credential verifiers, grants, devices and sessions must remain in that persistent authority rather than becoming reusable password material under `/etc/vdr-suite`.

## Pairing reuse

Existing Device, Credential, Session, Actor and revocation primitives are reusable for later TV/app pairing.

Missing before a complete pairing stack:
- explicit Human Account identity;
- account-to-actor binding;
- device policy/ownership semantics;
- short-lived pairing request/bootstrap lifecycle;
- stable approval/read models.

Pairing therefore is not the first runtime slice.

## Root-cause decision

The missing multiuser product boundary is not security persistence. It is semantic identity separation.

```text
existing:
Actor + Credential + Device + Session + Grant + Audit

missing:
Human Account --explicit binding--> User Actor
Profile --later product identity--> Account/household context
```

Because a generic User Actor can already represent compatibility principals, an Account API built directly on `security_actors` would be architecturally incorrect.

## First justified successor

The smallest runtime successor after ADR-0065 acceptance is a Human Account persistence/read foundation in the existing Suite database:
- explicit account identifier;
- explicit binding to one primary User Actor;
- no duplicate identity DB;
- no password secret in read models;
- server-side authorization;
- focused repository/read-model tests;
- stable Public-v1 exposure only when representation/authorization are complete.

Account mutation, grant administration, Profiles, pairing and bootstrap remain separate later slices.
