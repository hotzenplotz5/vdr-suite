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
Administration architecture: ADR-0067 [ACCEPTED]
Current Multiuser runtime slice: none - MU.8 Credential and session administration [NOT STARTED]
Latest completed Multiuser runtime slice: MU.7 Backend access / permission grant administration [COMPLETED]
Completed MU.7 sub-slice: Grant-set read + desired-state grant mutation [COMPLETED - REAL YAVDR PASS / PR #413 MERGED]
Next product slice: MU.8 Credential and session administration [NOT STARTED]
```

## Binding architecture

The Multiuser workstream is built on the existing Suite security authority. The
binding accepted decisions are:

- [ADR-0061: Actor Permissions, Federation and Client Access](../adr/ADR-0061-actor-permissions-federation-client-access.md);
- [ADR-0065: Human Account, Profile and Device Identity Boundary](../adr/ADR-0065-human-account-profile-device-identity-boundary.md);
- [ADR-0066: Unclaimed Server, First-Admin Bootstrap and Local Recovery](../adr/ADR-0066-unclaimed-server-first-admin-bootstrap-recovery.md).

The administration boundary is accepted as
[ADR-0067: Human Account and Backend Access Administration](../adr/ADR-0067-human-account-backend-access-administration.md).
Durable acceptance evidence is in
[MU.5 Administration Architecture Acceptance](post-phase69-mu5-administration-architecture-acceptance.md).
ADR-0067 authorizes bounded successor implementation but does not itself claim
that MU.6 runtime has started or completed.

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

### MU.5 — Administration architecture contract [COMPLETED]

ADR-0067 is accepted.

The accepted contract freezes:

- the global Human Account administration permission vocabulary;
- atomic Human Account + initial human-password creation;
- Account revision / strong-precondition semantics;
- immediate session invalidation for Account deactivation;
- transactionally enforced final-usable-administrator protection;
- reuse of normalized grant persistence with product-level allowlisting;
- secret-free credential/session administration boundaries;
- Class-A mutation proportionality without Timer-style orchestration.

Durable evidence:
[MU.5 Administration Architecture Acceptance](post-phase69-mu5-administration-architecture-acceptance.md).

### MU.6 — Human Account lifecycle administration [COMPLETED]

Durable MU.6A evidence:
[MU.6A Human Account Lifecycle Authority Foundation](post-phase69-mu6-account-lifecycle-foundation.md).

Durable MU.6B evidence:
[MU.6B Public Account Item + Revision/ETag](post-phase69-mu6b-public-account-item.md).

Completed MU.6C evidence:
[MU.6C Public Account Lifecycle Mutation](post-phase69-mu6c-public-account-lifecycle-mutation.md).

Completed MU.6D evidence:
[MU.6D Atomic Account CREATE + Durable Idempotency](post-phase69-mu6d-account-create-idempotency.md).

Completed MU.7 evidence:
[MU.7 Backend Access / Permission Grant Administration](post-phase69-mu7-account-grant-administration.md).

```text
MU.6A Account lifecycle authority foundation            [DONE]
MU.6B Public Account item + revision/ETag               [DONE]
MU.6C Public display-name / activate / deactivate       [COMPLETED]
MU.6D Atomic Account CREATE + durable idempotency       [COMPLETED — PR #411 MERGED]
```

MU.6A delivers:

- persisted monotone Human Account revision with additive migration;
- revision-checked display-name mutation;
- Account/Actor display-name consistency in one transaction;
- revision-checked activate/deactivate;
- immediate canonical browser-session revocation on deactivate;
- browser-session fail-closed Account-state resolution;
- transactionally enforced final-usable-administrator protection;
- secret-free accountability for success/conflict/denial.

MU.6B adds the secret-free stable `GET /api/v1/accounts/{accountId}` item
under the existing `accounts.view@*` authority. It exposes the MU.6A revision
only through a strong opaque ETag, supports `If-None-Match`/304 and leaves the
existing Account collection unchanged without a collection ETag.

MU.6C exposes the bounded Public-v1 lifecycle mutation over the MU.6A
authority and is completed after real yaVDR acceptance and PR #410 merge.

MU.6D adds the bounded Atomic Account CREATE boundary: global
`accounts.create@*`, browser CSRF, a closed loginName/displayName/password
request, request-only yescrypt password handling and durable
actor+Idempotency-Key replay. It creates no role or backend grants. LOCAL
Local and real yaVDR acceptance passed and PR #411 merged. MU.6 is complete.

### MU.7 — Backend access / permission grant administration [COMPLETED — REAL YAVDR PASS / PR #413 MERGED]

The completed slice exposes a secret-free normalized grant-set read with a
deterministic strong ETag and desired-state ensure/revoke mutation with strong
If-Match. Global accounts.grants.view@* / accounts.grants.modify@* authority,
central browser CSRF, explicit server-owned permission/scope allowlisting and
transactional final-admin protection remain authoritative. Grant-set revision
is independent from Human Account revision.

### MU.8 — Credential and session administration [NOT STARTED]

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
  -> ADR-0067 Account/Backend Access Administration [ACCEPTED]
       -> MU.6 Account lifecycle administration [DONE]
            -> MU.6A lifecycle authority [DONE]
            -> MU.6B Account item/revision [DONE]
            -> MU.6C public lifecycle mutation [DONE]
            -> MU.6D Account CREATE/idempotency [DONE - PR #411]
       -> MU.7 Backend access/grant administration [COMPLETED - REAL YAVDR PASS / PR #413 MERGED]
       -> MU.8 Credential/session administration [NOT STARTED]
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
- ADR-0067 is accepted; MU.6A-D are completed runtime boundaries and MU.6D merged as PR #411. MU.7 grant administration is completed after REAL YAVDR acceptance and PR #413 merged. MU.8 credential/session administration remains not started.
- Phase 70 is not started by Multiuser work.
