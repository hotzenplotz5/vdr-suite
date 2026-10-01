# Post-Phase-69 Multiuser Productization Workstream

## Status

**Active cross-cutting productization workstream.**

This workstream is deliberately not a numbered runtime phase. Phase 70 remains
separate and not started.

Current gate:

```text
Multiuser foundation: completed
Account read foundation: completed
First Admin / bootstrap / recovery: completed
Legacy Basic retirement: completed
Next Multiuser architecture gate: ADR-0067 [PROPOSED]
Next Multiuser runtime slice: blocked until ADR-0067 is accepted
```

## Binding architecture

The Multiuser workstream is built on the existing Suite security authority. The
binding accepted decisions are:

- [ADR-0061: Actor Permissions, Federation and Client Access](../adr/ADR-0061-actor-permissions-federation-client-access.md);
- [ADR-0065: Human Account, Profile and Device Identity Boundary](../adr/ADR-0065-human-account-profile-device-identity-boundary.md);
- [ADR-0066: Unclaimed Server, First-Admin Bootstrap and Local Recovery](../adr/ADR-0066-unclaimed-server-first-admin-bootstrap-recovery.md).

The next administration boundary is documented as
[ADR-0067: Human Account and Backend Access Administration](../adr/ADR-0067-human-account-backend-access-administration.md).
ADR-0067 is **proposed**, not accepted, and therefore does not yet authorize
runtime implementation.

## Completed Multiuser slices

### MU.0 — Identity authority audit and Human Account boundary [COMPLETED]

- proved that generic `ActorType::User` is not a Human Account;
- retained one Suite security authority;
- separated Human Account, Profile, Device and security Actor semantics;
- accepted ADR-0065.

Durable evidence:
[P1 Identity Model Audit](post-phase69-p1-identity-model-audit.md).

### MU.1 — Human Account persistence/read foundation [COMPLETED]

- explicit `security_human_accounts` persistence;
- explicit Account-to-User-Actor binding;
- secret-free Human Account read model;
- no synthesized accounts from generic User Actors.

Durable evidence:
[P2 Human Account Read Foundation](post-phase69-p2-human-account-read-foundation.md).

### MU.2 — Public-v1 read-only Account collection [COMPLETED]

- stable secret-free Account collection;
- global `accounts.view@*` authorization;
- no Account mutation or grant administration by implication.

Durable evidence:
[P2 Public Account Collection](post-phase69-p2-public-account-collection.md).

### MU.3 — First Admin, normal Human login and recovery [COMPLETED]

ADR-0066 was implemented through bounded runtime slices:

1. persistent claim/bootstrap state;
2. local root/operator bootstrap issuance;
3. atomic First Admin claim;
4. trusted browser completion;
5. normal Human Account password/browser-session authentication;
6. local audited Human Account credential recovery;
7. fresh-install authentication-default migration and enforced-mode fencing.

Durable evidence:
[P2 First Admin Bootstrap Audit](post-phase69-p2-first-admin-bootstrap-audit.md).

### MU.4 — Legacy Basic retirement [COMPLETED]

The guarded real-yaVDR migration/rollback acceptance completed before the
transitional Legacy Basic runtime authority was removed.

Durable evidence:
[P2 Legacy Basic Retirement Closeout](post-phase69-p2-legacy-basic-retirement-closeout.md).

## Planned continuation

The continuation is intentionally ordered so that the product gains usable
multiuser administration before later Profile/pairing personalization work.

### MU.5 — Administration architecture contract [CURRENT DOCUMENTATION GATE]

Accept or revise ADR-0067 before runtime implementation.

ADR-0067 must freeze:

- Human Account lifecycle mutation authority;
- administrator lockout/last-admin safety;
- grant-management authorization and scope;
- safe credential/session administration metadata;
- revision/concurrency and accountability requirements;
- stable Public-v1 versus local-operator boundaries.

### MU.6 — Human Account lifecycle administration [PLANNED]

After ADR-0067 acceptance, add the smallest bounded account-management slice.
It must preserve explicit Human Account identity, one security authority and
fail-closed administrator safety.

### MU.7 — Backend access / permission grant administration [PLANNED]

Expose inspection plus bounded grant/revoke operations through the existing
server-owned normalized grant model. Backend scope remains explicit; the UI
must not become a second policy authority.

### MU.8 — Credential and session administration [PLANNED]

Expose only safe administrative metadata and bounded revoke/rotation/session
management. Password verifiers, cookies, CSRF material and other secrets remain
non-readable.

### MU.9 — Account and access administration UI [PLANNED]

Build the browser administration surface over the accepted stable contracts.
This is the product prerequisite that unlocks the broad Timer Product UI.

### MU.10 — Device/app pairing [LATER]

Reuse Actor/Credential/Device/Session authority and the ADR-0065 short-lived
pairing bootstrap rule. Pairing grants no administrator rights by itself.

### MU.11 — Profiles / household personalization [LATER]

Profiles remain separate from Human Account identity, credentials, Devices and
permission Grants. Profile work must not be smuggled into Account administration.

## Product dependency

```text
ADR-0065 Human Account boundary [ACCEPTED]
  -> MU.1 Human Account persistence/read [DONE]
  -> MU.2 Public Account read [DONE]
  -> ADR-0066 First Admin/bootstrap/recovery [ACCEPTED + IMPLEMENTED]
  -> MU.4 Legacy Basic retirement [DONE]
  -> ADR-0067 Account/Backend Access Administration [PROPOSED]
       -> MU.6 Account lifecycle administration
       -> MU.7 Backend access/grant administration
       -> MU.8 Credential/session administration
       -> MU.9 Account/access admin UI
            -> Broad Timer Product UI unblocked

Later:
  -> MU.10 Device/app pairing
  -> MU.11 Profiles/personalization

Independent numbered track:
  -> Phase 70 Recommendation and Content Knowledge Graph [NOT STARTED]
```

## Hard boundaries

- No second identity or authorization database.
- A generic User Actor is not silently promoted to a Human Account.
- Profile is not a login identity.
- Device trust is not user identity or permission.
- Capability never grants authorization.
- No client/UI owns authorization policy.
- No Account/permission mutation runtime is authorized by this document alone;
  ADR-0067 must first be accepted.
- Phase 70 is not started by Multiuser work.
