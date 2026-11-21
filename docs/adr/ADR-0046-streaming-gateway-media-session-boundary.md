# ADR-0046: Streaming Gateway and Media Session Boundary

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Current State](../CURRENT.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Strict Roadmap](../planning/roadmap.md)
- [ADR-0017: Live Transport Boundary](ADR-0017-live-transport-boundary.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0045: Canonical EPG Event Identity and Provenance](ADR-0045-canonical-epg-event-identity-provenance.md)

---

## Status

Accepted

Date: 2026-07-16

---

## Context

VDR-Suite must provide live television and Recording playback for Web, Windows, Android, iOS and television clients without turning Streamdev, RESTfulAPI, SVDRP, a VDR plugin port, a recording path or a remote-site address into the public media API.

The repository already contains important foundations:

- a backend-neutral historical Stream Provider decision;
- explicit separation between live state-update transport and media transport;
- backend identities, capabilities and access modes;
- a Control Plane and Backend Agent boundary;
- authenticated client and Agent trust paths;
- stable Recording and ProgramEvent identity directions;
- multi-site requirements in which remote VDR-internal ports remain private;
- source-audit evidence for Streamdev and VDR-native behavior.

These foundations do not yet define a production media-session contract.

The current architecture must still answer:

```text
Who is allowed to watch?
Which exact resource is being watched?
Which backend and provider supply the bytes?
How long is access valid?
How is access revoked?
How are capacity and tuner resources reserved?
What does seek mean for a Recording that is still growing?
What happens when a remote Agent disconnects?
Can a live session move to another backend?
Which URLs and credentials are public?
Which layer owns proxying, remuxing or transcoding?
```

Directly returning an internal provider URL would create several failures:

- the client could bypass Control Plane authorization after the URL is issued;
- internal hostnames, IP addresses, ports and credentials would leak;
- Streamdev would accidentally become a public security boundary;
- a remote site would need inbound exposure of VDR-internal ports;
- backend replacement or multi-site routing would become a client concern;
- session expiry and revocation would be difficult to enforce;
- client logs, browser history, referrers or copied links could disclose reusable access;
- provider-specific path and protocol details would become a compatibility contract;
- concurrent-session, bandwidth and tuner limits would have no central enforcement point;
- audit could not reliably connect an actor, resource, route and stream outcome.

ADR-0017 deliberately excludes media streaming from live update transport. Snapshot SSE or WebSocket events therefore remain separate from the byte path defined here.

ADR-0039 places local stream-provider access below the Backend Agent boundary. ADR-0041 requires authenticated VDR-Suite streaming sessions instead of permanent Streamdev URLs and forbids placing user session identifiers in URL paths or query strings.

This ADR completes the architecture decision for gap G-19. Runtime implementation remains Phase 65 work.

---

## Decision

VDR-Suite introduces a logical Streaming Gateway and explicit Media Sessions.

The public flow is:

```text
Client
  -> authenticated VDR-Suite Client API
  -> Control Plane authorization and admission decision
  -> MediaSession
  -> Streaming Gateway
  -> MediaRoute
  -> Backend Agent or embedded Agent boundary
  -> internal StreamProvider
  -> VDR / Streamdev / Recording source / future provider
```

The client receives a VDR-Suite-controlled playback surface. It does not receive permanent direct credentials or stable internal URLs for Streamdev, VDR plugins, remote Agents or backend files.

The architecture separates these concepts:

```text
MediaResourceRef
  the Suite resource the actor wants to play

MediaSession
  the authorized, time-bounded playback intent and lifecycle

MediaRoute
  the selected backend, Agent, provider and delivery topology

ProviderStreamLease
  the bounded local reservation or provider handle

MediaAccessGrant
  short-lived proof that a client connection belongs to the session

PlaybackConnection
  one concrete HTTP, HLS, WebSocket or future media-plane connection
```

These identities and lifecycles are not interchangeable.

The Control Plane owns authorization, session policy, stable resource identity, route policy, limits, revocation and audit.

The Streaming Gateway owns public media-plane enforcement, connection handling and byte delivery.

The Backend Agent owns local provider selection, provider credentials, backend-generation checks, local stream establishment and local cleanup.

Streamdev, a future native plugin bridge and other providers remain internal adapters. They do not authenticate end users and do not issue the public platform contract.

---

## Logical Components

### Media Session Service

The Media Session Service:

- accepts an authenticated playback request;
- resolves the stable Suite resource;
- evaluates actor, backend and resource permissions;
- performs admission and policy checks;
- creates and persists or durably records the MediaSession lifecycle;
- requests a MediaRoute;
- issues bounded MediaAccessGrants;
- renews, revokes and terminates sessions;
- records audit-visible decisions and outcomes.

It does not read bytes from VDR while holding domain or database locks.

### Streaming Gateway

The Streaming Gateway:

- exposes the public media endpoint;
- validates the MediaAccessGrant for every new connection or protocol-defined request;
- binds the connection to one active MediaSession and MediaRoute;
- proxies, relays, remuxes or invokes a separately governed transcoding path according to the selected profile;
- enforces expiry, revocation, connection and bandwidth policy;
- supports protocol-appropriate disconnect, range and seek behavior;
- hides internal provider endpoints and credentials;
- emits bounded operational and audit events.

The Streaming Gateway may be deployed with the Control Plane for a local installation or as a separate service. The logical boundary remains the same.

### Backend Agent media broker

The Backend Agent media broker:

- validates that a route targets its enrolled backend and current generation;
- checks local capabilities and availability;
- opens a local ProviderStreamLease;
- protects Streamdev and other provider credentials;
- establishes or accepts the approved media-plane topology;
- reports route and provider state;
- closes provider resources when the lease ends;
- never exposes a permanent VDR-internal endpoint as the public client contract.

### Internal StreamProvider

A StreamProvider is a backend-local adapter that can offer one or more normalized stream profiles for a MediaResourceRef.

Possible providers include:

- Streamdev;
- a future `vdr-plugin-suite-bridge` media service;
- a VDR Recording file adapter;
- an IPTV, HLS or RTSP source adapter;
- TVHeadend or another future backend;
- a local remux or transcoding worker.

Provider-specific URLs, credentials, path formats, error payloads and object lifetimes stay behind the Agent boundary.

---

## Identity Model

The media architecture uses these identities:

| Identity | Meaning |
| --- | --- |
| `mediaSessionId` | Stable opaque Suite identity for one authorized playback session. |
| `mediaRouteId` | Stable identity for one selected delivery route within a session. |
| `providerStreamLeaseId` | Bounded local identity for one provider reservation or stream handle. |
| `mediaAccessGrantId` | Identity of one short-lived client access grant. |
| `playbackConnectionId` | Identity of one concrete client media-plane connection. |
| `actorId` | Authenticated user, service or client actor. |
| `clientInstanceId` | Optional registered or observed client instance. |
| `backendId` | Stable Suite backend identity. |
| `backendGeneration` | Agent/backend generation used to fence stale routes. |
| `siteId` | Site trust scope where applicable. |
| `resourceId` | Stable Suite Channel, ProgramEvent or Recording identity. |
| `resourceRevision` | Opaque revision used when the resource requires stale-state protection. |
| `nativeResourceId` | Backend-local binding kept behind the Agent/provider boundary. |

These identities are never replaced by:

- a hostname or IP address;
- a Streamdev channel URL;
- a local Recording pathname;
- a VDR pointer or file descriptor;
- a raw provider session identifier;
- a title, channel number or filename;
- a user login session cookie;
- a bearer token copied into a public resource ID.

`mediaSessionId` is an identifier, not a credential.

Possession of a `mediaSessionId` alone never authorizes playback.

---

## MediaResourceRef

A MediaResourceRef identifies the Suite-owned playback target.

Initial resource types are:

```text
live_channel
recording
```

Future additive types may include:

```text
programme_preview
live_event
radio_channel
timeshift_buffer
transcoded_asset
```

A live resource reference contains at least:

- stable Suite channel or service identity when available;
- optional `programEventId` and revision as descriptive current-programme context;
- allowed backend scope or route policy;
- requested playback profile.

A Recording resource reference contains at least:

- stable Suite Recording identity;
- expected Recording revision when required;
- backend/native binding evidence;
- whether the Recording is complete, growing, moved, trashed or unavailable;
- requested playback profile and start position.

A native channel number, Recording path or Streamdev URL is not a public MediaResourceRef.

### Playback is separate from download

Permission to play a Recording does not automatically grant permission to obtain an unrestricted permanent file download.

The policy vocabulary distinguishes at least:

```text
media.live.play
media.recording.play
media.recording.download
media.session.manage_own
media.session.manage_all
```

The final public permission names may be refined by ADR-0048 and Phase 62, but the separation is mandatory.

---

## MediaSession Contract

A MediaSession records one authorized playback intent independently from individual network connections.

It carries at least:

| Field | Meaning |
| --- | --- |
| `mediaSessionId` | Stable Suite session identity. |
| `sessionRevision` | Opaque revision for lifecycle changes. |
| `actorId` | Authenticated actor that requested playback. |
| `clientInstanceId` | Optional client instance identity. |
| `resourceType` | Live channel, Recording or future media type. |
| `resourceId` | Stable Suite resource identity. |
| `resourceRevision` | Bound revision where required. |
| `requestedProfile` | Client-requested protocol and media characteristics. |
| `selectedProfile` | Authorized and capability-supported profile. |
| `authorizationDecisionId` | Reference to the permission decision. |
| `policyRevision` | Policy revision used for admission. |
| `state` | Current MediaSession lifecycle state. |
| `createdAt` | Creation time. |
| `authorizedUntil` | Latest time current authorization remains valid without renewal. |
| `idleDeadline` | Optional idle timeout. |
| `absoluteDeadline` | Hard maximum lifetime. |
| `revokedAt` | Revocation time where applicable. |
| `endReason` | Deterministic final reason. |
| `currentMediaRouteId` | Active route when present. |
| `correlationId` | Request and diagnostics correlation. |

The exact persistence schema may normalize these fields. Their ownership and semantics are mandatory.

### MediaSession lifecycle

Canonical states are:

| State | Meaning |
| --- | --- |
| `requested` | An authenticated playback request exists but authorization or admission is incomplete. |
| `authorized` | Policy allows playback, but no usable route is ready. |
| `provisioning` | A route and provider lease are being established. |
| `ready` | A route is ready for an authorized client connection. |
| `active` | At least one accepted PlaybackConnection is using the session. |
| `reconnecting` | The previous connection or route failed and bounded recovery is in progress. |
| `draining` | New connections are denied while existing delivery is being closed. |
| `ended` | Session ended normally. |
| `expired` | Authorization, idle or absolute lifetime elapsed. |
| `revoked` | Access was explicitly withdrawn. |
| `failed` | Provisioning or delivery failed with a terminal classified reason. |

A session state change creates a new `sessionRevision` or equivalent durable history.

A client disconnect does not necessarily end the MediaSession immediately. Reconnect policy may retain it for a short bounded grace interval.

An ended, expired, revoked or failed session cannot be revived by reusing an old access grant.

---

## Authorization and Admission

Media authorization is evaluated by the Control Plane before route provisioning.

The minimum decision is:

```text
actor authenticated
AND actor may play this resource type
AND actor may access the selected backend or site
AND backend policy permits media access
AND resource is visible to the actor
AND provider capability is available
AND session and resource limits allow admission
AND current security policy allows the requested profile
```

Authorization is separate from capability and capacity.

A user can be authorized but receive `capacity_unavailable` because no tuner, provider slot or route is available.

A backend can be administratively read-only for mutations while still allowing media playback. Read-only mutation policy therefore does not automatically mean `media denied`; media access is an explicit independent permission and backend policy.

Authorization must be reevaluated when required by policy, including:

- MediaSession renewal;
- route replacement;
- backend or site change;
- permission or account revocation;
- resource visibility change;
- administrator termination;
- long-running session checkpoints.

A later denial moves the session to draining or revoked according to policy. It does not allow new connections.

### Parental and content policy

The contract permits future parental-rating, watershed, location and device policy checks.

Unknown rating data must have an explicit policy outcome. It must not silently bypass a configured restriction.

This ADR does not define final parental-control product rules.

---

## MediaAccessGrant

A MediaAccessGrant proves that a concrete client may connect to an active MediaSession.

A grant is:

- short-lived;
- scoped to one MediaSession;
- scoped to one actor and optionally one client instance;
- limited to allowed protocol actions;
- revocable through session or grant state;
- protected against replay according to protocol needs;
- useless after expiry, route fencing or session termination;
- excluded from normal logs and diagnostics.

A grant carries or cryptographically binds at least:

| Field | Meaning |
| --- | --- |
| `mediaAccessGrantId` | Grant identity. |
| `mediaSessionId` | Bound session. |
| `actorId` | Bound actor. |
| `clientInstanceId` | Optional client binding. |
| `allowedActions` | Connect, read, range, seek, renew or protocol-specific subset. |
| `issuedAt` | Issue time. |
| `expiresAt` | Short expiry. |
| `routeEpoch` | Route fencing value where applicable. |
| `nonce` | Replay-protection input where applicable. |
| `grantVersion` | Versioned grant semantics. |

### URL and credential rule

User login session identifiers and `mediaSessionId` values are not placed in URL paths or query strings as bearer credentials.

The normal browser model is:

- same-origin or explicitly trusted Gateway endpoint;
- secure, scoped cookie or protected request header;
- relative HLS or similar child-resource references that reuse the protected context;
- no provider credential in the manifest.

The normal native-client model is:

- protected transport;
- authorization header or equivalent protocol metadata;
- no reusable credential embedded in copied media URLs.

A URL-only compatibility mode is not part of the initial contract. If a future external player cannot send headers or scoped cookies, it requires an explicit compatibility decision with one-time or extremely short-lived single-purpose grants, leakage analysis and redaction rules. It may not reuse the user session or reveal an internal provider URL.

A non-secret route locator may appear in a public URL only when it is insufficient to authorize access by itself.

---

## MediaRoute

A MediaRoute binds one MediaSession to a concrete backend and delivery topology.

It carries at least:

| Field | Meaning |
| --- | --- |
| `mediaRouteId` | Stable route identity. |
| `mediaSessionId` | Owning session. |
| `routeEpoch` | Monotonic fencing value for route replacement. |
| `backendId` | Selected backend. |
| `backendGeneration` | Required active generation. |
| `agentId` | Selected Agent identity where applicable. |
| `siteId` | Site scope. |
| `providerType` | Internal provider category. |
| `providerCapabilityRevision` | Capability state used for selection. |
| `deliveryMode` | Central relay, Agent tunnel, trusted edge gateway or future mode. |
| `selectedProfile` | Protocol and media profile. |
| `state` | Route lifecycle state. |
| `leaseExpiresAt` | Route/provider lease expiry. |
| `createdAt` | Creation time. |
| `endedAt` | End time. |
| `endReason` | Deterministic termination reason. |

The provider's internal URL is not a required Control Plane field and is never a public client field.

### Route lifecycle

Canonical route states are:

```text
selected
provisioning
ready
active
degraded
replacing
draining
closed
failed
```

A new route uses a higher `routeEpoch`.

The Gateway and Agent reject bytes, control messages or lease renewals for an obsolete route epoch.

Backend generation and route epoch are separate fences:

- `backendGeneration` fences stale Agent/backend instances;
- `routeEpoch` fences a replaced delivery route within the MediaSession.

### Delivery topologies

Supported architecture directions are:

#### Central relay

```text
Client -> central Gateway -> Agent media tunnel -> local provider
```

The remote site initiates or participates in an authenticated outbound-capable path. No public Streamdev port is required.

#### Trusted site-edge gateway

```text
Client -> authenticated site-edge Gateway -> local Agent/provider
```

The edge Gateway is a VDR-Suite component with explicit trust, certificates, policy and public media enforcement. It is not a raw Streamdev exposure.

#### Embedded local route

```text
Client -> local Gateway -> embedded Agent boundary -> provider
```

Useful for one-host deployments while preserving logical boundaries.

The ADR does not mandate one topology for every deployment. Route selection remains policy- and capability-driven.

A raw redirect to a permanent internal Streamdev URL is forbidden.

---

## ProviderStreamLease

A ProviderStreamLease represents the local reservation needed to supply media.

It may reserve:

- a VDR receiver or tuner path;
- one Streamdev connection;
- a Recording file descriptor or reader;
- a remux worker;
- a transcoding worker;
- local bandwidth or connection capacity;
- a timeshift buffer in a future implementation.

The lease is bounded by:

- backend identity and generation;
- MediaSession and MediaRoute identity;
- resource binding;
- provider capability revision;
- expiry or heartbeat;
- cleanup behavior.

The Agent owns local lease creation and cleanup. The Control Plane owns the session and route decision.

A provider lease is not a durable public resource and is not reused by another actor merely because the original client disconnected.

Provider cleanup must be idempotent.

A lost Agent connection eventually expires or closes the lease according to deterministic policy.

---

## Stream Profiles and Capability Negotiation

A client requests desired playback characteristics rather than a provider URL.

A profile can describe:

- protocol family;
- container;
- video and audio codec preferences;
- maximum resolution and bitrate;
- subtitle preference;
- original/pass-through acceptance;
- range and seek requirements;
- low-latency preference;
- device capability profile.

The selected profile is the intersection of:

```text
client capability
AND actor policy
AND Gateway capability
AND Agent capability
AND provider capability
AND current capacity
```

Initial implementations may support only pass-through transport-stream playback. The contract must report that truthfully.

Possible capability IDs include:

```text
media.live.play
media.recording.play
media.recording.range
media.recording.seek
media.recording.growing
media.pass_through.ts
media.remux.hls
media.remux.fmp4
media.transcode
media.subtitles
media.timeshift
media.route.central_relay
media.route.agent_tunnel
media.route.edge_gateway
```

Capabilities have version, state, constraints and degradation reasons.

A capability is not advertised merely because a provider binary exists.

### Pass-through, remux and transcode

These are distinct operations:

- pass-through forwards provider media with minimal transformation;
- remux changes container or segmentation without re-encoding elementary streams;
- transcode decodes and re-encodes one or more streams.

The Gateway may coordinate these operations, but slow or expensive work must not execute inside VDR callbacks or while VDR locks are held.

A future transcoding worker follows job, capacity and lifecycle contracts appropriate to streaming. This ADR does not select FFmpeg, hardware encoders or one mandatory codec ladder.

---

## Live Playback Semantics

A live session targets a stable Suite channel or service identity, not merely a channel number on one backend.

Route selection considers:

- backend access permission;
- channel availability and mapping;
- current backend health and generation;
- provider and tuner capability;
- configured backend preference;
- site and bandwidth policy;
- requested profile;
- current capacity.

The current ProgramEvent may be included as display and audit context. It does not replace the channel playback identity.

### Live failover

A live session may replace its route only when:

- the session policy permits route replacement;
- the actor remains authorized for the new backend/site;
- the same canonical channel/service is available;
- capability and capacity checks pass;
- the new route receives a higher `routeEpoch`;
- the old route is fenced and closed;
- the discontinuity is reported to the client and audit stream.

Live failover does not promise seamless codec, timestamp or packet continuity.

If continuity cannot be guaranteed, the client receives an explicit reconnect or playback-discontinuity outcome rather than corrupted hidden switching.

A route must not silently switch to a different regional channel or unrelated stream based only on a display name.

---

## Recording Playback Semantics

A Recording session targets a stable Suite Recording identity and its authoritative native binding.

The Gateway and Agent must not resolve a Recording from an arbitrary client-supplied filesystem path.

The route verifies:

- Recording identity and visibility;
- backend/native binding;
- expected revision where required;
- current state such as available, growing, moved, trashed or deleted;
- provider access and file safety;
- allowed playback or download policy.

### Range and seek

Range and seek behavior is capability-driven.

For completed recordings, the provider may expose deterministic byte-range or time-seek semantics.

For growing recordings:

- current length may change;
- a previously valid end offset may become stale;
- duration can be provisional;
- seek beyond currently available content is rejected or waits only under explicit bounded policy;
- the response and client contract identify growing-resource behavior.

A provider that cannot support reliable range or seek must not advertise it.

HTTP success from opening a file is not proof that the requested range or media timestamps were delivered correctly.

### Recording relocation

If a Recording moves or its native path changes while the stable Recording identity remains valid, a new route may be provisioned after authoritative binding readback.

The client is not given the new path.

A Recording route must not be rebound based only on filename similarity.

### Shared and remote storage

A second Agent may serve the same Recording only when VDR-Suite has explicit shared-storage identity and ownership evidence. Merely seeing the same pathname on two hosts is insufficient.

This ADR does not define the final shared-storage model.

---

## Timeshift Boundary

Timeshift is not implied by ordinary live playback.

A future timeshift implementation requires explicit ownership of:

- buffer identity;
- storage location;
- retention and quota;
- start, live edge and seek window;
- backend or Gateway responsibility;
- reconnect behavior;
- cleanup after session termination;
- rights and audit.

Until those fields and capabilities exist, a live session is pass-through live playback only.

The plugin does not become a global timeshift coordinator.

---

## Connection and Reconnect Model

A PlaybackConnection is one concrete media-plane connection under a MediaSession.

It carries:

- `playbackConnectionId`;
- `mediaSessionId`;
- `mediaRouteId` and `routeEpoch`;
- authenticated grant identity;
- protocol;
- accepted range or start position;
- connection start and end time;
- bounded byte and error counters;
- end reason.

Multiple short protocol connections may belong to one MediaSession, for example manifest and segment requests.

Concurrency policy determines whether multiple simultaneous player connections are allowed for one session.

Reconnect behavior is bounded by:

- access-grant expiry;
- session state;
- reconnect grace interval;
- route state;
- actor and client binding;
- maximum attempts or time;
- capacity policy.

Reconnect does not create a new unlimited session automatically.

An obsolete grant or route epoch is rejected.

---

## Expiry, Renewal and Revocation

Every MediaSession has:

- a short authorization horizon;
- an idle timeout where appropriate;
- an absolute maximum lifetime;
- explicit renewal rules;
- deterministic revocation behavior.

Renewal requires a current authenticated client context and reevaluates policy.

A MediaAccessGrant expires sooner than or at the MediaSession authorization horizon.

Revocation may be triggered by:

- user logout or credential revocation according to policy;
- permission or role change;
- backend/site access revocation;
- administrator action;
- resource deletion or visibility change;
- security event;
- concurrency or abuse policy;
- Agent/backend trust revocation;
- route-generation conflict.

The Gateway must stop accepting new connections immediately after revocation is visible to it.

Existing byte delivery is closed promptly according to protocol-safe draining policy. Revocation is not postponed indefinitely to preserve playback.

Control Plane, Gateway and Agent clocks must follow bounded skew rules defined with the final security protocol.

---

## Capacity and Admission Control

Media playback consumes real resources even though it is not a domain mutation.

Admission may account for:

- VDR receiver and tuner availability;
- concurrent Streamdev connections;
- Recording reader limits;
- Gateway connection limits;
- Agent and site bandwidth;
- central relay bandwidth;
- remux or transcode workers;
- per-user and per-client limits;
- global and backend-specific quotas;
- priority between live viewing and recordings.

Capacity decisions are explicit and observable.

A session is not reported `ready` until the required route and provider lease exist.

A provider disappearance changes the route or session to degraded, reconnecting or failed. It must not leave a false ready state.

Capacity reservations expire and are cleaned up after abandoned provisioning.

The final tuner-priority policy remains a later implementation decision and must preserve VDR-native correctness.

---

## Multi-Site Media Plane

Remote-site media must preserve the trust boundary established by ADR-0039 through ADR-0041.

Rules:

- Streamdev and VDR plugin ports remain private to the site;
- the Agent or trusted edge component uses authenticated machine identity;
- backend ID, generation, Agent ID, site ID and route epoch are validated;
- media-plane tunnels are encrypted and integrity protected;
- client credentials are not forwarded to Streamdev;
- Agent credentials are not exposed to clients;
- one compromised site cannot mint sessions for another site;
- session revocation propagates to the Gateway and route owner;
- reconnect after Agent loss requires current lease and generation validation;
- permanent inbound access to arbitrary VDR ports is not required.

The Agent control connection and media byte path may use separate channels or connections. They still share identity, route and lifecycle contracts.

A VPN may be an additional transport layer but is not by itself authorization or Agent identity.

---

## Public and Internal Boundary

### Public client-visible contract

Clients may receive:

- MediaSession state and expiry;
- stable Suite resource identity;
- selected profile and supported playback controls;
- VDR-Suite Gateway origin or endpoint description;
- non-secret connection metadata;
- classified errors and reconnect instructions;
- bounded playback diagnostics appropriate to the actor.

Clients do not receive:

- internal Streamdev URLs;
- VDR plugin ports;
- Agent control endpoints;
- provider usernames or passwords;
- local Recording paths;
- backend certificates or credentials;
- raw provider handles;
- VDR pointers or file descriptors;
- unrestricted filesystem download paths.

### Internal Agent/provider contract

The Agent may receive:

- session and route identity;
- backend and generation fence;
- normalized resource binding;
- selected profile;
- provider lease deadline;
- required diagnostics and correlation context;
- no end-user reusable credential beyond the bounded authorization facts required for local enforcement.

Provider-specific data remains local unless a bounded normalized fact is required by the route contract.

---

## Control Plane, Gateway, Agent and Plugin Ownership

### Control Plane owns

- actor authentication context and authorization decision;
- MediaSession identity and lifecycle authority;
- stable media-resource identity;
- backend/site selection policy;
- concurrency and admission policy;
- route decision and route epoch authority;
- MediaAccessGrant policy and revocation;
- public session state;
- audit and security-event linkage;
- cross-backend live failover policy;
- final client-visible outcome.

### Streaming Gateway owns

- public media endpoint enforcement;
- grant validation at connection time;
- connection and segment authorization;
- byte relay and protocol termination;
- range and seek enforcement at the public boundary;
- connection limits and immediate revocation enforcement;
- hiding internal endpoints;
- media-plane metrics without secret leakage;
- route-epoch enforcement on the Gateway side.

### Backend Agent owns

- local stream-provider discovery and capability reporting;
- current backend-generation enforcement;
- ProviderStreamLease lifecycle;
- local provider credentials;
- local media tunnel or edge routing participation;
- native Recording/channel binding resolution;
- local cleanup and bounded diagnostics;
- route-epoch enforcement on the Agent side.

### `vdr-plugin-suite-bridge` or another VDR-local adapter owns

- minimal safe VDR-native access needed by a local provider;
- copying stable channel, receiver, Recording or stream facts under correct locks;
- truthful native capabilities;
- non-blocking callback behavior;
- deterministic local start/stop integration;
- bounded native errors and current-state observations.

The plugin does not own:

- end-user authentication;
- MediaSession persistence;
- public access grants;
- cross-site routing;
- global bandwidth or concurrency policy;
- public HTTP or HLS compatibility;
- audit retention;
- a general transcoding farm;
- client playback state;
- permanent external listener exposure.

A plugin media capability begins disabled or unavailable until source review, contract tests, live VDR acceptance and rollback prove it safe.

---

## VDR Native Safety Rules

Any VDR-native media integration obeys these rules:

- raw VDR pointers and receiver objects never cross the plugin boundary;
- VDR locks are held only while copying the minimal required values or acquiring a native-safe local reference according to VDR rules;
- no network write, client wait, TLS operation, remux, transcode or large serialization occurs while a VDR lock is held;
- VDR callbacks do not synchronously provision MediaSessions or wait for Gateway/Agent responses;
- receiver and device allocation follows VDR-native priority and lifecycle semantics;
- plugin shutdown disables callbacks and closes local resources without indefinite external waits;
- route and provider cleanup is idempotent;
- a disconnected client cannot leave an unbounded receiver or worker active;
- provider capability loss is reported truthfully;
- live acceptance proves channel, Timer, Recording, replay and setup state remain unchanged except for the explicitly expected temporary receiver use.

Media playback is read-only at the domain level but can consume or temporarily allocate native runtime resources. Tests must therefore verify both non-mutation and cleanup.

---

## Audit and Observability

Media events are correlated by:

```text
correlationId
mediaSessionId
mediaRouteId
routeEpoch
playbackConnectionId
actorId
backendId
backendGeneration
resourceType
resourceId
```

Audit-relevant events include:

- playback requested;
- authorization allowed or denied;
- admission allowed or denied;
- session created;
- route selected and replaced;
- provider lease opened and closed;
- access grant issued, expired or revoked;
- connection accepted and ended;
- range or seek request classified;
- backend/Agent disconnect;
- live route failover;
- administrator termination;
- suspicious replay or token use;
- terminal session outcome.

Normal high-volume byte counters and segment telemetry are operational metrics, not automatically permanent security-audit records.

Logs and metrics must not contain:

- access-grant secrets;
- cookies or authorization headers;
- internal credentials;
- full internal provider URLs with secrets;
- local Recording paths unless a privileged redacted diagnostic explicitly requires them;
- unbounded media payload data.

The final audit schema is defined by ADR-0049.

---

## Error and Outcome Vocabulary

The shared semantic categories include:

```text
unauthenticated
permission_denied
resource_not_found
resource_revision_conflict
backend_unavailable
backend_generation_conflict
agent_unavailable
provider_unavailable
profile_unsupported
capacity_unavailable
route_provision_failed
route_replaced
grant_expired
grant_revoked
grant_replay_rejected
range_unsupported
range_unsatisfiable
seek_unsupported
resource_growing
session_expired
session_revoked
client_disconnected
upstream_disconnected
media_delivery_failed
```

These are semantic categories, not final HTTP status or public error-envelope decisions. ADR-0048 maps them into the public API contract.

The client-visible outcome distinguishes:

- authorization denial;
- admission/capacity denial;
- route provisioning failure;
- connection failure before bytes;
- delivery interruption after bytes began;
- normal end of stream;
- explicit revocation;
- expiry;
- recoverable reconnect instruction.

A response that began streaming successfully is not later rewritten as an ordinary synchronous API error. The protocol exposes termination through connection outcome, manifest state, event delivery or a subsequent session read.

---

## Interaction With Live Update Transport

MediaSession state changes may be published through the existing live update architecture.

Examples:

```text
media.session.ready
media.session.active
media.session.reconnecting
media.session.expired
media.session.revoked
media.session.failed
```

The live update transport remains a consumer of domain events and does not carry the media bytes.

The media byte path does not become the owner of snapshots, EPG state or general frontend event delivery.

SSE disconnection does not automatically terminate playback unless explicit client/session policy says so.

---

## Existing Implementation Mapping

The existing repository components remain valid foundations:

- ADR-0017 continues to define live state-update transport and explicitly excludes media;
- the historical Stream Provider strategy remains the backend-neutral provider direction;
- ADR-0039 keeps Streamdev and provider access below the Agent boundary;
- ADR-0041 supplies authenticated client and Agent trust prerequisites;
- backend registry, access modes and capabilities remain inputs to authorization and routing;
- stable Recording and ProgramEvent identities remain MediaResourceRef inputs;
- Streamdev remains a possible internal provider.

They do not yet implement:

- MediaSession persistence or lifecycle;
- Streaming Gateway;
- MediaAccessGrants;
- public media authorization;
- MediaRoute and route epochs;
- ProviderStreamLeases;
- central relay or Agent media tunnel;
- range and seek contracts;
- live failover;
- transcoding or timeshift;
- production audit and security events.

Acceptance of ADR-0046 must not be reported as Phase 65 completion.

---

## Implementation Sequence

Phase 65 should proceed in this order after its prerequisites are complete:

1. MediaResourceRef and MediaSession domain contracts;
2. session persistence and lifecycle state machine;
3. authorization and admission service boundary;
4. versioned StreamProvider capability contract;
5. MediaRoute and route-epoch model;
6. fake-provider Gateway integration tests;
7. local Recording pass-through playback;
8. local live pass-through playback;
9. expiry, revocation, reconnect and cleanup hardening;
10. range and seek behavior;
11. Backend Agent media broker and authenticated remote tunnel;
12. multi-site routing and failure tests;
13. optional remux profiles;
14. later explicit transcoding and timeshift slices;
15. operational documentation, migration and rollback acceptance.

No remote-site streaming is enabled before Agent authentication, backend generation and lease enforcement exist.

No public player receives a raw internal provider URL during migration.

---

## Test and Acceptance Strategy

### Contract and domain tests

Tests cover:

- stable opaque identities;
- lifecycle transitions and invalid transitions;
- session revision behavior;
- grant/session/route separation;
- authorization versus capacity outcomes;
- access-grant expiry and revocation;
- route-epoch and backend-generation fencing;
- profile negotiation;
- deterministic errors;
- no secret fields in public DTOs or logs.

### Gateway integration tests

Using fake providers and Agents, tests cover:

- successful live and Recording byte relay;
- denied connection without a valid grant;
- expired and replayed grants;
- relative manifest/segment authorization;
- disconnect and cleanup;
- range and seek behavior;
- growing Recording behavior;
- provider failure before and after bytes;
- route replacement and stale-route rejection;
- concurrent-session limits;
- bandwidth/admission limits;
- revocation during active delivery;
- no internal URL disclosure.

### Agent and multi-site tests

Tests cover:

- enrolled Agent only;
- wrong backend, site or generation rejection;
- outbound-capable media tunnel establishment;
- Agent loss and lease expiry;
- reconnect under a new generation;
- stale route rejection;
- remote read-only backend with explicit media permission;
- one compromised test Agent unable to serve another backend identity;
- internal Streamdev endpoint remaining unreachable from the public client network.

### Live VDR acceptance

Controlled acceptance uses disposable viewing sessions and proves:

- plugin/provider load and lifecycle;
- truthful capability reporting;
- one expected temporary receiver or Recording reader only;
- no Timer, Recording, channel-list, setup or filesystem mutation;
- bounded lock and callback behavior;
- client disconnect releases the provider resource;
- Gateway revocation stops delivery;
- VDR restart or plugin rollback leaves no stale listener, receiver or lease;
- internal provider credentials and URLs are absent from public responses and logs.

Live acceptance must not use destructive commands or alter real Timers and Recordings.

### Security tests

Tests cover:

- credential and token redaction;
- URL, referrer and browser-history leakage resistance;
- cookie origin and scope behavior;
- CSRF protection where browser cookies authorize session management;
- replay and stolen-grant limits;
- permission revocation;
- cross-user and cross-backend isolation;
- rate and connection limiting;
- malformed range and protocol input;
- denial-of-service bounds for abandoned provisioning and connections.

---

## Migration and Compatibility

Existing experimental or internal stream URLs may remain available only as explicitly local development tools until the Gateway path replaces them.

They must not be documented as the stable public API.

Migration rules:

- public clients move to MediaSession creation and Gateway playback;
- frontend code stops constructing backend or Streamdev URLs;
- provider credentials remain local to Agent configuration;
- old direct-stream capability is marked internal, deprecated or disabled;
- removal waits for supported clients to use the new session contract;
- rollback can disable the Gateway without changing VDR Recording or Timer data;
- session and audit records retain version information needed for compatibility analysis.

The final route and API version negotiation is aligned with ADR-0048.

---

## Alternatives Considered

### Return permanent Streamdev URLs to clients

Rejected because Streamdev is an internal transport, not the user authorization, multi-site trust or public compatibility boundary.

### Put user credentials into Streamdev or VDR plugins

Rejected because it duplicates identity, RBAC, revocation and audit inside each backend and expands the VDR process attack surface.

### Encode the user session or MediaSession ID in the playback URL

Rejected because URLs leak through logs, history, referrers, screenshots and link sharing, and identifiers are not bearer credentials.

### Let each frontend build backend-specific URLs

Rejected because it leaks topology, fragments client behavior and prevents backend-neutral routing.

### Make the Backend Agent the public Gateway

Rejected as the universal model because remote sites should not require public inbound exposure. A separately trusted site-edge Gateway remains an optional deployment topology.

### Force all media through the central Control Plane process

Rejected as a universal requirement because high-bandwidth media may require a separate Gateway or edge path. The Control Plane remains session authority even when bytes use another trusted topology.

### Treat one HTTP connection as the session

Rejected because HLS and reconnecting clients use multiple requests, and authorization, route and playback lifecycles outlive individual connections.

### Treat playback as an ordinary durable job

Rejected because real-time byte delivery has connection, lease and backpressure semantics different from asynchronous batch work. Durable provisioning or transcode preparation may use jobs, but the active MediaSession remains a separate domain.

### Implement timeshift automatically with live playback

Rejected because timeshift requires explicit buffer identity, quota, seek-window and cleanup contracts.

### Use a shared provider database or filesystem path as the protocol

Rejected because it violates the Agent boundary and cannot safely express authorization, generation, route or session lifecycle.

---

## Consequences

Positive:

- clients receive one stable VDR-Suite playback model;
- internal VDR and Streamdev endpoints remain private;
- multi-site streaming can work without arbitrary inbound plugin exposure;
- permissions, expiry and revocation are enforceable;
- provider replacement does not require frontend changes;
- live and Recording playback share security and route foundations;
- capacity, tuner and bandwidth use become observable;
- live failover and reconnect have explicit semantics;
- Recording paths and provider credentials do not leak;
- future remux, transcode and timeshift can be additive capabilities.

Trade-offs:

- Gateway and session services add operational complexity;
- remote media relay may consume central or site bandwidth;
- browser, native and television clients require careful credential handling;
- route and lease state must be reconciled across distributed failures;
- range, growing Recording and seek semantics require provider-specific proof;
- edge deployment requires certificate, trust and update lifecycle;
- high-volume media metrics need separation from durable audit records.

---

## Non-Goals

This ADR does not select:

- a mandatory streaming protocol;
- final public endpoint paths;
- one HLS, DASH or WebRTC implementation;
- one media server or proxy library;
- FFmpeg or a specific transcoder;
- codec ladders and device presets;
- final tuner-priority policy;
- final parental-control rules;
- final shared-storage semantics;
- timeshift implementation;
- content DRM;
- permanent offline downloads;
- CDN deployment;
- final public HTTP error mapping;
- the ADR-0049 audit schema.

It defines the mandatory ownership, identity, authorization, route, access and lifecycle boundary.

---

## Acceptance Criteria

ADR-0046 is implemented only when:

- MediaSession, MediaRoute, ProviderStreamLease, MediaAccessGrant and PlaybackConnection are distinct implemented concepts;
- the public client never needs an internal Streamdev URL, VDR plugin port or Recording path;
- session identifiers alone do not authorize access;
- login sessions and MediaSession IDs are not used as URL bearer credentials;
- every connection is bound to an unexpired, unrevoked grant and current route epoch;
- backend generation fences stale remote routes;
- authorization, capability and capacity outcomes are distinct;
- expiry, renewal, revocation, disconnect and cleanup are deterministic;
- live and Recording resources use stable Suite identities;
- range, seek and growing Recording behavior are capability-tested;
- remote streaming preserves Agent and site trust boundaries;
- internal provider credentials remain local;
- plugin/VDR lock and callback safety is proven;
- tests prove no unintended VDR mutation;
- audit and operational events are correlated without secret leakage;
- migration and rollback are documented and tested;
- full CI and controlled live acceptance pass.

Acceptance of this ADR is not runtime completion.

---

## Related Decisions

- [ADR-0012: Source Capability Model](ADR-0012-source-capability-model.md)
- [ADR-0013: Permission Model](ADR-0013-permission-model.md)
- [ADR-0014: Recording Identity Strategy](ADR-0014-recording-identity-strategy.md)
- [ADR-0017: Live Transport Boundary](ADR-0017-live-transport-boundary.md)
- [ADR-0020: Multi-Source Federation Architecture](ADR-0020-multi-source-federation-architecture.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0045: Canonical EPG Event Identity and Provenance](ADR-0045-canonical-epg-event-identity-provenance.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
