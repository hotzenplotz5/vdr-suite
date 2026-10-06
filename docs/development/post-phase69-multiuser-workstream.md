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
Current Multiuser runtime slice: MU.9F - Account CREATE UI [IMPLEMENTATION CANDIDATE]
Latest completed Multiuser runtime slice: MU.9E - Backend Grant Mutation UI [IMPLEMENTATION MERGED - PR #429 / FOCUSED YAVDR PASS / POST-MERGE CI #9763 GREEN]
Completed MU.7 sub-slice: Grant-set read + desired-state grant mutation [COMPLETED - REAL YAVDR PASS / PR #413 MERGED]
Current acceptance gate: MU.9F - Account CREATE UI [IMPLEMENTATION CANDIDATE]
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

### MU.8 — Credential and session administration [IMPLEMENTATION COMPLETE — MU.8A RUNTIME ACCEPTANCE PENDING]

MU.8A - Safe credential/session metadata [IMPLEMENTATION MERGED - PR #415 / RUNTIME ACCEPTANCE PENDING]

MU.8A adds only the secret-free Public-v1 Credential and Session collection
reads under `accounts.credentials.view@*` and `accounts.sessions.view@*`.
It reuses the existing canonical issuer/session lifecycle and does not expose
password verifiers, browser tokens, cookie material or CSRF/session hashes.
The MU.8A implementation merged as PR #415 with hosted CI green; real yaVDR
runtime acceptance remains pending.

MU.8B - Session Revoke [IMPLEMENTATION MERGED - PR #416 / FOCUSED YAVDR PASS / HOSTED CI GREEN]

MU.8B adds revisioned Session-item GET plus global
`accounts.sessions.revoke@*` POST on the same Session resource. The mutation
uses browser CSRF, a Session-owned strong lifecycle ETag that excludes passive
`lastSeenAt`, the canonical BrowserSessionLifecycleService revoke path, and
terminal-idempotent replay semantics. Focused acceptance on the real yaVDR
checkout passed without a local daemon build, install, restart or runtime
mutation. PR #416 and its post-merge push CI completed successfully.

MU.8C - Human-password Credential Revoke [IMPLEMENTATION MERGED - PR #417 / FOCUSED YAVDR PASS / HOSTED CI GREEN]

MU.8C owns revisioned Human Account Credential-item GET plus global
`accounts.credentials.revoke@*` POST on the same Credential resource. It
accepts only `human-password` credentials, uses browser CSRF and a
Credential-owned strong lifecycle ETag, preserves desired-state terminal
replay, protects the final usable administrator at credential granularity, and
synchronously fences Browser Sessions issued from the revoked credential via
the canonical BrowserSessionLifecycleService. Password reset/rotation and
client administration UI remain outside this slice. Focused real-yaVDR
acceptance passed; PR #417 merged and both PR CI and post-merge main CI are
green. MU.8 implementation is complete through MU.8C, while MU.8A retains its
separately documented real-runtime acceptance debt.

### MU.9 — Account and access administration UI [IN PROGRESS — MU.9F ACCOUNT CREATE UI CANDIDATE]

Build the browser administration surface over the accepted stable contracts.
This is the product prerequisite that unlocks the broad Timer Product UI.

MU.9A - Account Administration Read UI [IMPLEMENTATION MERGED - PR #418 / EXACT MERGED-MAIN YAVDR PASS / HOSTED CI GREEN]

MU.9A mounts a read-only Users & access surface in the existing Settings owner.
It reuses the accepted Public-v1 reference client for Account, grant,
Credential and Session reads, preserves server-side authorization, and adds no
mutation control. Account/grant/Credential/Session mutations remain later MU.9
slices. MU.9A merged as PR #418 and passed exact merged-main yaVDR acceptance;
PR and post-merge hosted CI are green.

MU.9B - Account Lifecycle Mutation UI [IMPLEMENTATION MERGED - PR #420 / REAL YAVDR PASS / HOSTED CI GREEN]

MU.9B adds display-name change and Account activate/deactivate controls to the
existing Settings owner. It reuses the accepted MU.6C Public-v1 Account
lifecycle mutations with strong Account ETag / If-Match fencing, active browser
CSRF, explicit deactivate confirmation and full selected-Account refresh after
mutation. Account CREATE, grant mutation, Credential revoke and Session revoke
remain later MU.9 slices. The implementation passed real yaVDR acceptance, merged as PR #420, and has green PR/post-merge hosted CI. This closeout does not select the successor MU.9 slice.

MU.9C - Session Revoke UI [IMPLEMENTATION MERGED - PR #426 / FOCUSED YAVDR PASS / HOSTED CI GREEN]

MU.9C adds explicit per-Session revoke controls to the existing selected Account
Settings surface. It reuses only the accepted MU.8B revisioned Session-item GET
and Session revoke contracts. The browser obtains the selected Session item's
strong ETag immediately before mutation, forwards browser CSRF, emits one revoke
request with `If-Match`, and refreshes the selected Account afterwards. A stale
`412` is surfaced after read refresh and is never retried implicitly. Account
CREATE, grant mutation and Credential revoke remain later MU.9 slices. MU.9C passed focused yaVDR acceptance and merged as PR #426 with green hosted PR CI.

MU.9D - Human-password Credential Revoke UI [IMPLEMENTATION MERGED - PR #428 / FOCUSED YAVDR PASS / HOSTED CI GREEN]

MU.9D adds explicit per-Credential revoke controls for non-terminal
`human-password` Credentials in the existing selected Account Settings
surface. It reuses only the accepted MU.8C revisioned Credential-item GET and
Credential revoke contracts. The browser obtains the selected Credential's
strong ETag immediately before mutation, forwards browser CSRF, emits one revoke
request with `If-Match`, and refreshes the selected Account afterwards. A stale
`412` is surfaced without automatic retry; final-usable-administrator `409`
remains server-owned. Issuer-session fencing, including possible invalidation of
the current browser Session, remains entirely server-owned under MU.8C. Account
CREATE, grant mutation, password reset/rotation, pairing and Profiles remain
outside MU.9D. Focused yaVDR acceptance passed; PR #428 merged, PR CI was green, and post-merge main CI run #9761 completed successfully with all six jobs green.

MU.9E - Backend Grant Mutation UI [IMPLEMENTATION MERGED - PR #429 / FOCUSED YAVDR PASS / POST-MERGE CI #9763 GREEN]

MU.9E exposes the accepted MU.7 desired-state grant mutation in the existing Account administration Settings surface. The browser uses the selected Account grant-set strong ETag, forwards browser CSRF, calls only `setAccountGrant(...)`, never automatically retries a stale `412`, and refreshes the complete selected Account after mutation. Existing returned tuples can be revoked directly. No Public-v1 permission-catalog discovery contract exists, so the browser does not copy or invent the server allowlist: ensure accepts an explicit permission plus backend/resource scope and leaves `422` support validation to the authoritative server. Focused real-yaVDR acceptance passed; PR #429 merged as `0bb84d5702508110960b4f8004f3fc9a31a1a744`, and post-merge CI #9763 ultimately completed with all six jobs green after GitHub-hosted runner degradation. Password setup/reset/rotation, general Credential creation, pairing, Profiles and Phase 70 remain outside MU.9E.

MU.9F - Account CREATE UI [IMPLEMENTATION CANDIDATE]

MU.9F exposes only the accepted MU.6D Public-v1 Account CREATE contract in the existing Account administration Settings owner. One explicit browser action supplies `loginName`, `displayName`, an initial request-only password and one caller-owned Idempotency-Key, forwards browser CSRF, emits exactly one `createAccount(...)` call and refreshes the Account list after success. It does not assign roles or backend grants automatically. Password reset/rotation, general Credential creation, pairing, Profiles and Phase 70 remain outside MU.9F.

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
       -> MU.8 Credential/session administration [IMPLEMENTATION COMPLETE - MU.8A RUNTIME ACCEPTANCE PENDING]
       -> MU.9 Account/access admin UI [IN PROGRESS - MU.9F ACCOUNT CREATE UI CANDIDATE]
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
- ADR-0067 is accepted; MU.6A-D are completed runtime boundaries and MU.6D merged as PR #411. MU.7 grant administration completed after REAL YAVDR acceptance and PR #413 merge. MU.8 implementation is complete through MU.8C: MU.8A merged as PR #415 with separately documented real-runtime acceptance still pending, MU.8B merged as PR #416 after focused yaVDR acceptance, and MU.8C merged as PR #417 after focused yaVDR acceptance with PR and post-merge main CI green. MU.9 remains in progress after MU.9A merged as PR #418 and MU.9B merged as PR #420 after real yaVDR acceptance; PR and post-merge hosted CI are green for both accepted UI slices. MU.9C Session Revoke UI merged as PR #426 after focused yaVDR acceptance and green hosted PR CI. MU.9D Human-password Credential Revoke UI merged as PR #428 after focused yaVDR acceptance; PR and post-merge hosted CI are green. MU.9E Backend Grant Mutation UI merged as PR #429 after focused real-yaVDR acceptance; post-merge main CI #9763 completed successfully with all six jobs green. MU.9F Account CREATE UI is now the active bounded implementation candidate over the accepted MU.6D Account CREATE contract.
- Phase 70 is not started by Multiuser work.
