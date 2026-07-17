# ADR-0001: Suite Bridge Plugin Role and Native Integration Strategy

## Navigation

- [Plugin README](../README.md)
- [Plugin Roadmap](ROADMAP.md)
- [Shared VDR-Suite Handoff](VDR-SUITE-HANDOFF.md)
- [SB.9 Capability Discovery](SB-9-capability-discovery.md)

---

## Status

Accepted

Date: 2026-07-17

---

## Context

VDR-Suite already uses `vdr-plugin-restfulapi` as a broad structured integration
surface for channels, EPG, Timers, Recordings, SearchTimers, status information
and selected actions.

That integration remains valuable and is not replaced by
`vdr-plugin-suite-bridge`.

However, a general HTTP API and periodic domain refresh cannot represent every
VDR-native fact with the required latency, lifecycle accuracy, continuity and
security semantics.

Relevant gaps include:

- immediate observation of VDR callbacks;
- exact start and stop transitions for Recording and replay;
- short-lived OSD messages that may disappear between HTTP polls;
- explicit instance, event and OSD continuity after restart or reconnect;
- authoritative confirmation that a requested native action actually occurred;
- safe access to legacy VDR and plugin OSD workflows that do not yet have a
  structured VDR-Suite domain API;
- deterministic local capability reporting without guessing from installed
  software versions;
- a bounded local integration surface for the Backend Agent without exposing
  VDR-internal transports to public clients.

The current Suite Bridge foundation already proves:

- deterministic plugin lifecycle;
- bounded, non-blocking `cStatus` observation;
- saturating diagnostic counters;
- immutable counter epochs;
- complete read-only `SNAP` resynchronization;
- explicit `CAPS` compatibility negotiation;
- no plugin-owned listener, database, worker or mutation surface;
- `mutations=disabled`.

The next steps require a durable decision about what the plugin is for, what it
must never become and how it relates to RESTfulAPI, the Backend Agent and the
VDR-Suite Control Plane.

---

## Decision

`vdr-plugin-suite-bridge` is the small VDR-process-local native integration
bridge for VDR-Suite.

It complements RESTfulAPI. It does not replace RESTfulAPI and does not become a
second general-purpose HTTP API.

The preferred responsibility split is:

```text
RESTfulAPI
  broad structured VDR reads and existing domain-oriented operations

vdr-plugin-suite-bridge
  immediate VDR-native observations, continuity, bounded native readback,
  safe Legacy OSD adaptation and narrowly typed native operations

Backend Agent
  local transport, compatibility negotiation, buffering, freshness,
  backend generation, authenticated machine relationship and local brokering

Control Plane
  users, authorization, public API, multi-site policy, durable jobs,
  idempotency, retries, reconciliation and audit retention
```

The only supported client path is:

```text
Client
  -> authenticated VDR-Suite public API
  -> VDR-Suite Control Plane
  -> authenticated Backend Agent
  -> local adapter
  -> vdr-plugin-suite-bridge and/or RESTfulAPI
  -> VDR Core
```

Clients never connect directly to the Suite Bridge plugin, SVDRP, RESTfulAPI,
plugin service interfaces or local remote-control devices.

---

## Source Selection Strategy

For every VDR-Suite function, the implementation selects the narrowest source
that can truthfully satisfy the required contract.

### RESTfulAPI remains preferred for

- complete channel lists;
- complete EPG windows and searches;
- complete Timer lists;
- complete Recording lists;
- SearchTimer integration;
- metadata already exposed as structured values;
- existing broad read APIs where callback-level latency is unnecessary;
- existing actions that can be safely wrapped and authoritatively reconciled.

### Suite Bridge is preferred for

- exact VDR lifecycle facts;
- immediate callback-derived state changes;
- short-lived native notifications;
- event epoch, sequence, overflow and resynchronization semantics;
- native OSD visibility and frame continuity;
- allowlisted local input translation after all authorization prerequisites
  exist;
- bounded authoritative readback after a native action;
- typed VDR or plugin operations that RESTfulAPI cannot provide correctly,
  promptly or safely.

### Hybrid operation is expected

The normal efficient pattern is:

```text
Suite Bridge emits a bounded native change fact
  -> Backend Agent validates continuity
  -> only the affected RESTfulAPI or native read path is refreshed
  -> Control Plane publishes the resulting Suite-domain change
```

A native event is not automatically the complete domain object. It may be a
precise dirty hint that triggers an authoritative targeted read.

---

## Native Observation Strategy

The plugin may observe VDR callbacks only through bounded, non-blocking code.

A callback may:

- check one atomic active-state flag;
- update a bounded atomic counter;
- mark a bounded dirty flag;
- reserve or write one fixed-capacity event slot when proven safe;
- copy a small immutable value when the VDR contract permits it;
- return immediately.

A callback must not:

- open a network connection;
- wait for the Agent or a client;
- perform HTTP, SVDRP or filesystem I/O;
- access a database;
- allocate an unbounded object graph;
- serialize a large payload;
- compress a frame;
- write detailed logs;
- execute an external command;
- retain raw VDR pointers or lock ownership.

Large or potentially blocking work occurs outside the callback after native
values have been copied into bounded plugin-owned state.

---

## Native Event Feed

A future read-only native event feed is an approved plugin responsibility.

Its purpose is to represent exact transitions that a broad polling API may only
expose as a later changed snapshot.

Candidate event families include:

- channel switch;
- Recording started;
- Recording stopped;
- replay started;
- replay stopped;
- Timer changed;
- OSD opened;
- OSD closed;
- OSD status message changed or cleared;
- volume, mute, audio-track or subtitle-track change where VDR exposes safe
  authoritative facts.

The feed must use:

- an explicit event schema;
- an immutable event epoch;
- monotonic sequence numbers within one epoch;
- a fixed-capacity ring or another bounded buffer;
- explicit overflow and sequence-gap reporting;
- complete resynchronization after lost continuity;
- bounded payload size and string length;
- no durable history in the plugin.

Event delivery is not audit persistence. The Control Plane owns durable audit
and security history.

A native callback can occur more than once for one user-visible action. Event
semantics therefore describe observed VDR facts, not inferred user intent.

---

## OSD Notification Strategy

Short-lived OSD status messages are a high-value native observation surface.

RESTfulAPI can expose the current OSD snapshot, but a message may appear and
vanish between client polls. A Suite Bridge notification contract may retain a
small bounded recent window so the Agent can observe the transition.

A future OSD notification value may include:

- OSD epoch;
- event sequence;
- capture timestamp;
- active, changed or cleared state;
- bounded UTF-8 message text;
- optional bounded title or category when known without unsafe inference;
- truncation state;
- privacy or suppression state.

OSD notification payloads are transient. They are not written to normal logs or
stored indefinitely.

Because arbitrary plugin OSDs can expose paths, configuration, account names,
PIN prompts or other private values, OSD observation always remains capability-
and policy-controlled.

---

## Legacy OSD Strategy

The normal VDR-Suite product remains domain-first.

A Legacy OSD bridge is allowed only for:

- VDR or plugin menus not yet represented by a Suite domain;
- controlled diagnostics;
- controlled maintenance;
- source validation during migration;
- trusted administrator compatibility workflows.

Normal EPG browsing, channel browsing, Timer management, SearchTimer management,
Recording management, backend administration and permission management must not
remain OSD-only merely because they can be reached through a native menu.

A future read-only OSD surface must provide:

- immutable complete frames;
- explicit OSD surface identity;
- explicit OSD epoch;
- frame sequence;
- bounded frame size;
- sequence-gap detection;
- full-frame resynchronization;
- truthful degraded capability reporting;
- no direct public plugin endpoint.

View and control are separate capabilities.

OSD viewing does not grant input authority.

---

## OSD Input Strategy

Native OSD input remains disabled until the Control Plane and Backend Agent
implement all required authorization and fencing concepts.

Required prerequisites include:

- explicit `osd.control` authorization;
- server-enforced read-only denial;
- one bounded controller lease per native OSD surface;
- controller-lease epoch;
- backend generation;
- OSD epoch;
- command identity;
- deadline and rate limit;
- bounded queue;
- stale-command rejection;
- audit correlation;
- controlled live acceptance and rollback.

Public clients may eventually send only normalized allowlisted actions such as:

```text
up
Down
left
right
ok
back
menu
red
green
yellow
blue
play
pause
stop
channel_up
channel_down
volume_up
volume_down
mute
number_0 through number_9
```

Concrete spelling and schema are versioned when implemented. The illustrative
list above does not enable input.

The following generic escape surfaces are prohibited:

```text
execute(command)
runSvdrp(text)
callPlugin(service, payload)
pressRawKey(number)
sendKeyboard(text)
```

Acceptance of a key for dispatch proves only bounded delivery. It does not prove
that a Timer was created, a Recording was deleted or a setup value was saved.
Domain mutations require domain-level verification.

---

## Typed Native Action Strategy

A future native action is justified only when a concrete gap is demonstrated.

Examples of potentially useful typed actions include:

- channel switch with authoritative resulting-channel readback;
- absolute volume selection;
- explicit mute state;
- explicit audio-track selection;
- explicit subtitle-track selection;
- one named plugin maintenance operation with a fixed schema;
- one bounded native health or resynchronization operation;
- later Timer or Recording operations with revision checks and authoritative
  readback.

Each action must have:

- a stable typed request schema;
- a capability gate;
- bounded input values;
- explicit generation and revision fences where applicable;
- deterministic local rejection categories;
- no silent retry after an unknown outcome;
- authoritative readback or reconciliation;
- controlled live tests using disposable resources;
- complete rollback evidence.

The plugin never exposes a universal plugin-service or SVDRP tunnel.

---

## Mutation Boundary

Read-only remains the default.

`mutations=disabled` remains a hard prohibition until a later accepted and
implemented capability explicitly enables one narrowly scoped mutation family.

The plugin does not own:

- user authentication;
- RBAC;
- durable operation records;
- idempotency storage;
- retry scheduling;
- saga coordination;
- cross-site reconciliation;
- public mutation status;
- durable audit retention.

The plugin may execute one bounded native step after the Agent has supplied a
valid fenced assignment. It returns bounded execution and readback evidence.

Unknown outcome is a valid result and must not trigger blind redispatch.

---

## Capability and Versioning Strategy

The following compatibility axes are independent:

- plugin software version;
- capability-discovery schema;
- capability schema;
- snapshot schema;
- local-contract schema;
- future event schema;
- future OSD frame schema;
- future input schema;
- authenticated Agent protocol version;
- public VDR-Suite API version.

Compatibility is never inferred from the plugin software version alone.

A capability becomes `available` only after:

- source and boundary checks;
- deterministic unit tests;
- malformed and unsupported request tests;
- final VDR shared-object build;
- staged API-versioned installation;
- required controlled live VDR acceptance;
- shutdown and rollback proof;
- synchronized documentation and handoff.

Unknown or absent optional capabilities are unavailable.

Unknown or absent mutation capabilities are disabled.

---

## Transport Boundary

The plugin does not open a network listener and does not initiate an outbound
connection.

The Backend Agent owns local transport selection.

Approved local transports may include:

- VDR SVDRP for fixed allowlisted plugin commands;
- a bounded VDR plugin service adapter where a later contract proves lifecycle
  and thread safety;
- RESTfulAPI behind an Agent adapter;
- another explicitly accepted local mechanism.

Raw transports are never exposed to public clients.

SVDRP use must be command-typed. The Agent does not pass arbitrary command text
through the Suite Bridge transport.

---

## Ownership Summary

| Concern | Control Plane | Backend Agent | Suite Bridge plugin |
| --- | --- | --- | --- |
| Public API | owner | no | no |
| User authentication and RBAC | owner | machine trust participant | no |
| Multi-site policy | owner | backend-scoped participant | no |
| Durable jobs and idempotency | owner | executes assignments | no |
| Local transport | no | owner | command target only |
| Capability freshness | consumes | owner | reports local truth |
| VDR lifecycle | observes | observes | owner |
| Native callbacks | no | consumes copied facts | owner |
| Native event epoch and sequence | consumes | validates and buffers | local producer |
| OSD public session | owner | local broker | native adapter only |
| OSD controller lease | owner | enforces locally | optional local fence |
| Native mutation | coordinates | transports and fences | bounded executor |
| Durable audit | owner | bounded producer | bounded local facts only |
| Plugin database | no | no | prohibited |
| Public plugin listener | no | no | prohibited |

---

## Alternatives Rejected

### Replace RESTfulAPI with Suite Bridge

Rejected because the Suite Bridge should remain small and VDR-native. Rebuilding
all broad EPG, channel, Timer, Recording and SearchTimer APIs would duplicate
working integration and enlarge the plugin's lifecycle and security surface.

### Use only RESTfulAPI polling

Rejected because polling alone cannot guarantee observation of short-lived
messages, exact transitions, plugin-instance continuity or native action
readback with the required semantics.

### Publish RESTfulAPI, SVDRP or the plugin directly to clients

Rejected because local VDR transports do not implement VDR-Suite user identity,
backend-scoped authorization, multi-site routing, controller leases, public API
compatibility or durable audit policy.

### Implement a generic plugin-service gateway

Rejected because it would become an unbounded hidden control surface and bypass
typed authorization, capability and compatibility contracts.

### Put durable event history in the plugin

Rejected because the plugin must not become a database, audit store or workflow
engine. Native buffers remain bounded and transient.

### Enable OSD input before view-only continuity

Rejected because input without OSD epoch, frame continuity, controller leases
and stale-command fencing is nondeterministic and unsafe.

---

## Consequences

Positive consequences:

- RESTfulAPI remains useful and avoids unnecessary duplication;
- VDR-Suite gains an explicit path for immediate native facts;
- short-lived OSD messages can be observed without making the plugin public;
- exact start and stop transitions can trigger targeted refreshes;
- restart and reconnect uncertainty becomes detectable;
- future OSD access can be sequenced and resynchronized;
- future native actions can be narrowly typed and authoritatively read back;
- multi-site clients use one consistent authenticated Suite path;
- the plugin remains small, bounded and VDR-lifecycle-correct.

Trade-offs:

- Agent integration becomes more sophisticated because it composes more than
  one local source;
- event sequencing and resynchronization require explicit contracts;
- OSD fidelity and privacy vary between VDR versions, skins and plugins;
- some features require controlled live acceptance on each supported VDR path;
- domain APIs and Legacy OSD compatibility must coexist during migration;
- exact low-latency guarantees must be measured rather than assumed.

---

## Non-Goals

This ADR does not:

- implement a new plugin command;
- change plugin version `0.10.0`;
- change discovery, capability, snapshot or local-contract schemas;
- enable mutations;
- define final native event fields;
- define final OSD frame encoding;
- define final public endpoint paths;
- define final Agent protocol messages;
- promise every candidate roadmap slice;
- make the plugin a replacement for RESTfulAPI;
- make Legacy OSD the primary VDR-Suite UI.

---

## Acceptance Criteria

This decision is respected only while:

- RESTfulAPI and Suite Bridge responsibilities remain distinct;
- every new plugin feature identifies a concrete native gap;
- callbacks remain bounded and non-blocking;
- raw VDR pointers and locks never cross the plugin boundary;
- event, snapshot and OSD continuity use explicit epochs and sequences;
- buffers and payloads remain bounded;
- unknown or lost continuity triggers resynchronization;
- OSD view and control remain separately authorized;
- no raw SVDRP, shell, keycode or plugin-service tunnel is exposed;
- read-only remains the default;
- native actions require capability gates and authoritative readback;
- public clients use only the authenticated VDR-Suite path;
- required automated and live VDR acceptance passes before a capability becomes
  available;
- the shared handoff remains synchronized after accepted implementation slices.

---

## Related Decisions and Contracts

- [Plugin Roadmap](ROADMAP.md)
- [Shared VDR-Suite Handoff](VDR-SUITE-HANDOFF.md)
- [SB.3 Native VDR Status Events](SB-3-status-events.md)
- [SB.6 Read-Only SVDRP](SB-6-read-only-svdrp.md)
- [SB.8 Counter Continuity](SB-8-counter-continuity.md)
- [SB.9 Capability Discovery](SB-9-capability-discovery.md)
- [VDR-Suite ADR-0030: Domain-First UI Over OSD Proxy](../../docs/adr/ADR-0030-domain-first-ui-over-osd-proxy.md)
- [VDR-Suite ADR-0039: Backend Agent and Control Plane Boundary](../../docs/adr/ADR-0039-backend-agent-control-plane-boundary.md)
- [VDR-Suite ADR-0042: Safe Mutation, Revision and Idempotency](../../docs/adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [VDR-Suite ADR-0047: Legacy OSD Compatibility Bridge](../../docs/adr/ADR-0047-legacy-osd-compatibility-bridge.md)
- [VDR-Suite ADR-0049: Audit and Security Event Model](../../docs/adr/ADR-0049-audit-security-event-model.md)

---

## Back

- [Back to Plugin README](../README.md)
- [Back to Plugin Roadmap](ROADMAP.md)
- [Back to Shared Handoff](VDR-SUITE-HANDOFF.md)
