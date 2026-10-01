# ADR-0067: Human Account and Backend Access Administration

## Status

Proposed architecture.

Date: 2026-10-01

This ADR is not runtime authorization until accepted.

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

### Read authority remains separate from mutation authority

The accepted `accounts.view@*` permission remains a read-only global Account
discovery permission.

Account lifecycle mutation and permission-grant administration require explicit
administrative authorization distinct from `accounts.view@*`. Acceptance of
this ADR must freeze the exact normalized mutation permission identifiers before
runtime implementation.

A backend-scoped administrator role must not implicitly gain global Human
Account administration merely because it can administer one backend.

### Account lifecycle is service-owned and fail-closed

The first Account administration runtime must be bounded to explicit Human
Account lifecycle operations and use a domain/service boundary over the
canonical repositories.

The first slice may support safe create/update/activate/deactivate semantics, but
must not add destructive hard deletion merely for CRUD symmetry.

Any operation that can remove the final usable administrator authority must fail
closed. Recovery from administrator lockout remains a local trusted-operator
boundary rather than an anonymous remote bypass.

Account mutation must preserve:

- explicit Account-to-Actor binding;
- User Actor type invariant;
- stable Account identity;
- existing credential/session/grant ownership semantics;
- transactional accountability evidence.

### Grant administration reuses the normalized grant model

Backend/content access administration must reuse the existing
`security_actor_permission_grants` authority and ADR-0061 operation/resource
scope model.

The administration surface may inspect and mutate supported grants, but it must
not invent a parallel role/ACL language in the browser.

Grant mutation must preserve:

- explicit target Actor;
- explicit normalized permission;
- explicit resource/backend scope;
- server-side authorization;
- accountability evidence;
- fail-closed rejection of unsupported permission/scope combinations.

### Credential and session administration is metadata-safe

Administrative surfaces may expose bounded safe metadata such as credential
type, lifecycle state, issue/expiry/revocation timestamps and session ownership
where explicitly authorized.

They must never return:

- password verifiers;
- browser cookies;
- CSRF values;
- bootstrap secrets;
- raw reusable credentials.

Password recovery/rotation keeps the accepted local audited recovery boundary
until a later explicitly accepted remote credential-management contract exists.

Session revoke/credential revoke operations must reuse canonical lifecycle
services rather than editing rows directly.

### Concurrency and mutation safety

Account and grant mutations must use the smallest concurrency contract justified
by the resource.

Where concurrent administrative edits can overwrite each other, stable public
resources require explicit revision/precondition semantics. Timer-style durable
operation/saga machinery is not required unless a concrete multi-step failure
model proves it necessary.

No blind retry after an ambiguous mutation outcome is allowed.

### Public API and browser boundary

Independent clients consume stable `/api/v1` contracts. The browser
administration UI is a client of the same server-owned product authority; it is
not a privileged direct database console.

Public administration contracts must:

- return secret-free resources;
- apply authenticated server-side authorization before dispatch;
- use CSRF protection for browser mutations;
- preserve Phase-69 compatibility/error semantics;
- expose backend/resource scope explicitly where relevant.

Local root/operator recovery remains a separate local administration contract
and is not converted into normal Public-v1 CRUD.

### Accountability

Successful and denied administrative mutations must append secret-free
accountability evidence sufficient to answer:

- who requested the mutation;
- which Human Account/Actor was targeted;
- which permission/scope changed where applicable;
- whether the action succeeded or was denied;
- which revision or lifecycle boundary was crossed where applicable.

## Planned implementation sequence

After this ADR is accepted, implementation proceeds as bounded Multiuser slices:

1. Human Account lifecycle administration;
2. backend access / permission grant administration;
3. safe credential and session administration;
4. Account and access browser administration UI.

Device/app pairing and Profiles remain later, separate slices under ADR-0065.

## Consequences

The broad Timer Product UI remains gated until required Account/backend access
administration is usable. Multiuser administration may proceed independently of
the numbered runtime sequence.

Phase 70 Recommendation and Content Knowledge Graph remains not started and is
not authorized by this ADR.

## Non-goals

This ADR does not itself:

- implement runtime Account mutation;
- implement grant mutation;
- implement an administration frontend;
- implement Profiles;
- implement TV/app pairing;
- implement federation;
- start Phase 70;
- replace local audited recovery;
- introduce a second identity or policy authority.
