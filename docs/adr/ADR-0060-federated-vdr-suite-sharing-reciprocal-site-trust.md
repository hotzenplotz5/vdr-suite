# ADR-0060: Federated VDR-Suite Sharing and Reciprocal Site Trust

## Navigation

- [ADR Index](index.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Platform Productization Roadmap](../planning/platform-productization-roadmap.md)
- [ADR-0013 Permission Model](ADR-0013-permission-model.md)
- [ADR-0020 Multi-Source Federation Architecture](ADR-0020-multi-source-federation-architecture.md)
- [ADR-0039 Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0041 Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0046 Streaming Gateway and Media Session Boundary](ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0049 Audit and Security Event Model](ADR-0049-audit-security-event-model.md)
- [ADR-0061 Actor Permissions, Federation and Client Access](ADR-0061-actor-permissions-federation-client-access.md)

---

## Status

Accepted architecture / implementation pending.

Date: 2026-09-20

---

## Context and continuity

This ADR does **not** redefine MultiBackend.

ADR-0013 already established the intended federation model:

- an Actor may be a `remote VDR-Suite instance`;
- federation requires one VDR-Suite instance to act as a client of another VDR-Suite instance;
- remote access is granular rather than all-or-nothing;
- examples explicitly separate Recording viewing/streaming, Live TV and Timer rights.

ADR-0020 then established that a `BackendNode` may wrap a `remote VDR-Suite instance`, with capabilities and permissions evaluated per backend/source.

ADR-0039 through ADR-0041 subsequently implemented the secure multi-site **Backend Agent** foundation for one Control Plane managing site-local backends. ADR-0041 deliberately left `federation between independent Control Planes` as a non-goal of that phase.

This ADR fills that remaining product/architecture gap while preserving both models:

```text
Managed remote backend
  = one VDR-Suite Control Plane
    -> enrolled Backend Agent
    -> remote VDR site

Federated remote VDR-Suite
  = independent VDR-Suite A
    <-> explicit peer trust
    <-> independent VDR-Suite B
  with each side remaining authority for its own VDR/backends and grants
```

---

## Decision

VDR-Suite federation is a **directional, explicitly paired trust relationship between autonomous VDR-Suite installations**.

Pairing establishes machine/site identity and a protected transport. It does not grant unrestricted access.

Each site remains authoritative for:

- its own users and local authentication;
- its own VDR/backends;
- its own recordings, channels, EPG, timers and media sessions;
- the permissions it grants to the remote site and remote delegated users;
- its own audit/accountability evidence;
- revocation of the federation relationship.

A reciprocal relationship is therefore two independent grant directions:

```text
House A grants rights to House B
        !=
House B grants rights to House A
```

Either side may grant more, less or nothing in return.

---

## Federation actor and delegation

At minimum the paired remote VDR-Suite has a stable federation Actor identity, continuing ADR-0013.

A request arriving from Site B at Site A carries:

```text
authenticated peer identity: Site B / VDR-Suite B
requested operation
target backend/resource at Site A
optional delegated remote-user identity
correlation/accountability context
```

Site A performs the final authorization decision.

A remote site's local role name is never accepted as authority by itself. If per-remote-user sharing is implemented, Site A maps a stable delegated subject from Site B to explicit local grants or locally approved sharing policy.

This supports both:

- site-wide grants to a trusted household/site; and
- narrower grants for individual remote users.

Revoking the peer or a delegated subject invalidates future access without changing unrelated local identities.

---

## Protected federation transport

Independent VDR-Suite peers use an authenticated, encrypted, revocable Suite-to-Suite transport or stable public client contract.

Federation must **not** expose or forward permanent credentials for:

- RESTfulAPI;
- SVDRP;
- Streamdev;
- SuiteBridge;
- Backend Agent;
- local files/databases.

The owning site translates an authorized federated request into its normal local domain path.

Examples:

```text
remote recordings.play
  -> owner-site authorization
  -> owner-site MediaSession / Gateway
  -> owner-site provider lease
  -> protected media delivery to the paired Suite/client

remote recordings.cut
  -> owner-site authorization
  -> owner-site protected mutation Operation
  -> owner-site Agent / SuiteBridge / VDR
  -> authoritative readback / reconciliation

remote timers.create
  -> owner-site authorization
  -> owner-site TimerIntent/native fulfillment path
```

The remote site never becomes a raw controller of the neighbor's VDR transports.

---

## Relationship to BackendNode and local catalog

A remote VDR-Suite peer may appear to local product/UI code as an addressable federated source/backend in the sense established by ADR-0020.

That does not make the peer an enrolled Backend Agent and does not transfer ownership of the peer's VDRs to the local Control Plane.

Local backend catalog work remains useful for:

- local VDR backends;
- Agent-managed remote VDR backends;
- explicit federated peer/source registrations.

The product must distinguish these ownership/trust modes.

Registry order, `backendRuntimeContexts_.front()` or "first backend" never defines federation ownership or default policy.

---

## Federation lifecycle

A supported product flow includes:

```text
Generate pairing invitation / one-time trust material at Site A
  -> approve at Site B
  -> mutual site identity established
  -> capabilities exchanged
  -> no content permission granted automatically
  -> each side configures its own outbound grants
  -> optional remote-user mappings approved
  -> normal use
  -> rotate/revoke trust independently
```

Reachability alone never creates trust.

Pairing alone never creates content/action permission.

---

## Media and bandwidth

Live TV and Recording playback are explicit permissions, not side effects of pairing.

Remote media remains under the owner site's Phase-65 MediaSession/Gateway policy. The owner decides whether the requesting actor/site may stream that resource.

A federation implementation must preserve:

- short-lived media access;
- no permanent provider URLs;
- route/provider privacy;
- capacity and bandwidth policy;
- deterministic cleanup;
- explicit denial when remote streaming is not granted.

---

## Timers and orchestration

Manual permission to create a Timer on a remote site does not automatically authorize automatic scheduler/failover use of that site.

Those are separate policies.

Example:

```text
timers.create.remote-manual = allowed
timer-orchestration.remote-assignment = denied
```

If cross-site automatic Timer assignment is later enabled, Phase-64 ownership, assignment, generation, idempotency and reconciliation rules remain mandatory and the owner site must have explicitly granted that class of use.

---

## Acceptance

Federation is not accepted until two **independent** VDR-Suite installations prove:

- explicit pair/trust setup without sharing raw VDR/plugin credentials;
- Site A and Site B remain independently administrable;
- A->B and B->A grants can differ;
- revoking A->B access does not alter B->A grants;
- a remote Suite may list only resources permitted by the owner;
- allowed Recording streaming works through the owner Media Plane;
- denied Live TV is rejected even if technically available;
- allowed remote Timer creation succeeds through the owner Control Plane;
- denied Timer modify/delete never reaches native dispatch;
- allowed marks/cut operation follows owner-side protected mutation/readback;
- denied destructive Recording operations remain blocked;
- selected-folder/channel-group scopes can restrict visibility where configured;
- all protected remote actions are attributable in accountability evidence;
- peer outage or stale trust fails closed;
- no direct RESTfulAPI/SVDRP/Streamdev/SuiteBridge exposure becomes necessary.

---

## Non-Goals

This ADR does not make every paired site automatically writable, does not require symmetrical grants, does not merge databases, and does not turn Backend Agent trust into federation-user authority.

Exact wire/schema compatibility is finalized together with Phase 69 public/client compatibility or an explicitly versioned federation protocol.
