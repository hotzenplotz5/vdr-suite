# ADR-0061: Actor Permissions, Federation and Client Access

## Navigation

- [ADR Index](index.md)
- [Platform Productization Roadmap](../planning/platform-productization-roadmap.md)
- [ADR-0013 Permission Model](ADR-0013-permission-model.md)
- [ADR-0020 Multi-Source Federation Architecture](ADR-0020-multi-source-federation-architecture.md)
- [ADR-0042 Safe Mutation, Revision and Idempotency Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0049 Audit and Security Event Model](ADR-0049-audit-security-event-model.md)
- [ADR-0059 VDR-Native Recording Editing, Marks and Cutting Authority](ADR-0059-vdr-native-recording-editing-marks-cutting-authority.md)
- [ADR-0060 Federated VDR-Suite Sharing and Reciprocal Site Trust](ADR-0060-federated-vdr-suite-sharing-reciprocal-site-trust.md)

---

## Status

Accepted architecture / implementation pending.

Date: 2026-09-20

---

## Context

ADR-0013 already defines the core rule:

```text
operation allowed
  = source/backend has capability
  AND actor has permission
```

It also explicitly defines federation examples where `Remote Suite B` may see selected recordings from House A while Live TV and Timer creation are denied.

This ADR makes that long-standing direction product-ready for both reciprocal household/site sharing and ordinary client-only access.

---

## Decision

Permissions are **grants over explicit operations and resource scopes**. In federation, the owning site remains the final authorization authority for its resources.

A frontend name is not itself a permission, but a frontend or device may authenticate as an Actor/device/application identity and receive grants. A client does **not** need to provide a VDR, backend, media source or reciprocal federation capability in order to consume VDR-Suite resources it is allowed to access.

This explicitly includes pure clients such as the Web UI, a future VDR output/living-room client, Android app, television app, Kodi/mobile client or other API client. They may browse, stream, create timers or perform other granted operations without offering any backend of their own.

The same server-side permission model is enforced whether access comes from a local client, a remote pure client, or a user delegated through a paired VDR-Suite.

### Core operation families

The permission vocabulary must distinguish at least these semantic capabilities; exact public field names are finalized by the stable API/federation contract:

```text
Channels / EPG
  channels.view
  epg.view
  epg.search

Recordings
  recordings.view
  recordings.stream
  recordings.modify_metadata_or_title   [only where separately supported]
  recordings.marks
  recordings.cut
  recordings.move
  recordings.rename
  recordings.trash
  recordings.restore
  recordings.delete_or_purge

Live TV
  livetv.view

Timers
  timers.view
  timers.create
  timers.modify
  timers.delete

SearchTimers / automation
  searchtimers.view
  searchtimers.create_or_modify
  searchtimers.delete

Compatibility / remote control where implemented
  osd.view
  osd.control
```

Operations that have different risk or side effects must not be collapsed into one broad `write` switch.

In particular:

- viewing/streaming a Recording is separate from changing it;
- marks/cutting is separate from deletion/purge;
- viewing Live TV is separate from Timer creation;
- reading Timers is separate from changing them;
- OSD viewing is separate from OSD control.

### Resource scopes

Grants may be scoped by the owner to resources such as:

- one backend or a set of backends;
- selected Recording folders/collections;
- selected channels or channel groups;
- specific operation families;
- time/expiry where appropriate.

This continues the scoped examples already present in ADR-0013.

### Pure client access

A pure client is a consumer only:

```text
Android / TV app / output client / browser
  -> authenticate or pair as client/device/user
  -> receive explicit grants
  -> consume permitted VDR-Suite resources
  -> provides no BackendNode, VDR source or reciprocal content
```

A client may therefore have, for example, `recordings.stream` and `livetv.view` while having no server, VDR or federation peer behind it.

Federation is an additional actor/source relationship, not a prerequisite for permissioned client access.

### Directional reciprocal grants

Example:

```text
House A -> House B

recordings.view       allow
recordings.stream     allow
recordings.marks      allow
recordings.cut        allow
recordings.delete     deny
livetv.view           deny
timers.view           allow
timers.create         allow
timers.modify         deny
timers.delete         deny


House B -> House A

recordings.view       allow
recordings.stream     allow
recordings.cut        deny
livetv.view           allow
timers.create         deny
```

These matrices are independent.

There is no "friend/neighbor = full access" shortcut.

### Site grant versus delegated-user grant

The owner may support two levels:

1. a grant to the paired remote VDR-Suite/site Actor;
2. a narrower or overriding grant to an approved stable delegated user from that site.

A delegated remote user does not inherit arbitrary local roles. The owner site decides what that remote subject may do.

### Capability and permission remain separate

A permission grant does not fabricate capability.

If Site A grants `livetv.view` but the target backend currently has no usable Live provider, the operation remains unavailable.

If capability exists but the grant is denied, the operation is denied.

### Owner-side enforcement

Every remote protected operation is enforced by the site that owns the resource.

Frontend hiding/disabling is only a presentation aid.

For mutations, authorization happens before durable dispatch and the existing revision/idempotency/generation/readback/reconciliation contracts remain in force.

---

## Product UI

The sharing administration UI should present permissions in human terms, grouped by domain, for example:

```text
Freigabe für "Nachbar / House B"

Aufnahmen
  [x] sehen
  [x] abspielen
  [ ] umbenennen/verschieben
  [x] Schnittmarken bearbeiten
  [x] schneiden
  [ ] löschen

Live-TV
  [x] ansehen

Timer
  [x] ansehen
  [x] anlegen
  [ ] ändern
  [ ] löschen
```

Advanced scopes may then restrict folders, channel groups or backends.

The server stores/enforces the normalized grant model; the UI does not invent policy.

---

## Accountability

Remote actions must preserve:

- owning site;
- requesting peer/site identity;
- delegated remote-user identity when present;
- effective authorization decision;
- target backend/resource;
- normalized operation;
- outcome and relevant operation/job identities.

This makes it possible to answer who at which paired site requested a remote cut, Timer mutation or media session.

---

## Acceptance

Client and federation acceptance must prove server-side allow/deny behavior. Pure-client tests must prove that an Android/TV/output-style client can receive scoped rights without providing any backend. Real two-site federation acceptance must additionally prove independent allow/deny behavior for at least:

- Recording list/view;
- Recording stream;
- Live TV;
- Timer view;
- Timer create;
- Timer modify/delete denial;
- Recording marks/cut;
- Recording delete denial;
- a scoped Recording folder or channel group;
- revocation;
- per-user delegation where that subfeature is enabled.

No UI-only permission test is sufficient.

---

## Non-Goals

This ADR does not define a social/friend network, public anonymous sharing, automatic Internet discovery or a universal all-powerful remote role.
