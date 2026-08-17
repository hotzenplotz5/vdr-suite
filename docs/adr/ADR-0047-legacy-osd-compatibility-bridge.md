# ADR-0047: Legacy OSD Compatibility Bridge

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Current State](../CURRENT.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Strict Roadmap](../planning/roadmap.md)
- [ADR-0013: Permission Model](ADR-0013-permission-model.md)
- [ADR-0030: Domain-First UI Over OSD Proxy](ADR-0030-domain-first-ui-over-osd-proxy.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0046: Streaming Gateway and Media Session Boundary](ADR-0046-streaming-gateway-media-session-boundary.md)

---

## Status

Accepted

Date: 2026-07-16

---

## Context

VDR-Suite is a domain-first platform. Its Web, Windows, Android, iOS and television clients are expected to use stable Suite resources and workflows for EPG, channels, timers, SearchTimers, recordings, metadata, streaming, administration and diagnostics.

ADR-0030 therefore rejects the classic VDR OSD as the primary application model.

That decision is still correct.

However, VDR and its plugin ecosystem contain useful functions that may not yet have a native VDR-Suite domain representation. Some plugin setup pages, diagnostic menus, maintenance actions and niche workflows may remain reachable only through the VDR OSD for a transition period or permanently as a compatibility surface.

The completed source audit reached the following conclusion for `vdr-plugin-osd2web` and similar approaches:

- OSD access may be retained as an isolated legacy compatibility bridge;
- the primary Web, TV and native-client interfaces remain domain-first;
- viewing and controlling require different permissions;
- control requires a single bounded controller lease;
- frame or event delivery requires explicit sequencing and resynchronization;
- arbitrary command execution must not be exposed.

The current repository does not yet implement such a hardened bridge.

Existing related foundations include:

- backend identity and backend-scoped access modes;
- server-enforced read-only behavior;
- snapshot, event-sequence and resynchronization concepts;
- Control Plane and Backend Agent ownership boundaries;
- authenticated client and Agent trust paths;
- VDR-native lock, callback and pointer-isolation rules;
- a read-only Suite Bridge plugin foundation;
- RESTfulAPI and SVDRP adapter concepts;
- the explicit rule that clients do not call VDR plugin ports directly.

These foundations do not answer all OSD-specific questions:

```text
Which native OSD surface is being observed?
Can several users watch it at the same time?
Who may send keys?
Can two browser tabs control one VDR simultaneously?
What happens when a controller disconnects?
How are stale keys rejected after reconnect?
How does a client recover after losing one OSD update?
What happens when VDR restarts or replaces the OSD?
How are local physical remote-control actions represented?
Can a read-only backend expose OSD control?
How are sensitive OSD contents protected?
Can an OSD bridge become a shell or raw SVDRP tunnel?
```

A naive OSD proxy would create serious problems:

- it could bypass domain-level authorization by navigating native menus;
- it could make a read-only backend writable through remote key input;
- concurrent controllers could interleave keys unpredictably;
- dropped frames or deltas could leave clients rendering invented state;
- a stale browser could continue controlling a restarted backend;
- direct plugin ports would become public security boundaries;
- raw remote, SVDRP, shell or plugin-service commands could escape into the public API;
- VDR locks or callback threads could be held while serializing or sending large frames;
- OSD contents could expose recording names, plugin configuration, PIN prompts or other private information;
- the compatibility bridge could become a permanent shortcut instead of a bounded migration aid.

This ADR completes the architecture decision for gap G-20. Runtime implementation remains Phase 67 work.

---

## Decision

VDR-Suite introduces an isolated Legacy OSD Compatibility Bridge.

The normal flow is:

```text
Client
  -> authenticated VDR-Suite Client API
  -> Control Plane authorization and session policy
  -> LegacyOsdSession
  -> authenticated Agent Protocol
  -> Backend Agent OSD broker
  -> local OSD adapter or vdr-plugin-suite-bridge
  -> VDR native OSD and remote-input boundary
```

The client never receives a permanent direct URL, credential or unrestricted command channel for:

- `osd2web`;
- RESTfulAPI `/osd` or `/remote` endpoints;
- SVDRP;
- a VDR plugin service;
- a local input device;
- a shell command;
- a remote-site address.

The bridge separates these concepts:

```text
OsdSurfaceRef
  the backend-local native OSD surface being observed

LegacyOsdSession
  one authorized, time-bounded compatibility session

OsdViewerBinding
  one viewer attached to a session or shared surface

OsdFrame
  one immutable complete render state

OsdDelta
  one ordered change based on an exact prior frame

OsdControllerLease
  one exclusive Suite controller authority for the surface

OsdInputCommand
  one normalized, allowlisted input action

OsdInputResult
  acceptance, rejection and bounded execution evidence
```

These identities, states and responsibilities are not interchangeable.

The Control Plane owns:

- client identity and authorization;
- LegacyOsdSession lifecycle;
- `osd.view` and `osd.control` policy;
- controller-lease ownership and revocation policy;
- cross-client concurrency decisions;
- public API and compatibility behavior;
- audit correlation and administrative visibility.

The Backend Agent owns:

- the authenticated backend relationship;
- backend-generation fencing;
- local OSD capability discovery;
- local adapter selection;
- bounded frame and event transport;
- local sequence buffering and resynchronization support;
- enforcement that a command targets the active lease and backend generation;
- local cleanup after session or lease expiry.

The VDR plugin or local adapter owns:

- short-lived safe access to native OSD state;
- copying native state into immutable bounded values;
- native OSD epoch and change observation where available;
- bounded translation of normalized input actions to VDR-native keys;
- rejection of unsupported or unsafe commands;
- deterministic local results;
- VDR lock, thread and lifecycle correctness.

The plugin does not own:

- user authentication;
- user or role storage;
- public sessions;
- global controller arbitration;
- multi-site policy;
- audit retention;
- a public Internet listener;
- arbitrary command execution;
- a second Control Plane.

---

## Domain-First Boundary Remains Authoritative

The Legacy OSD Bridge is not the normal VDR-Suite application architecture.

Preferred implementation order remains:

1. identify the underlying domain function;
2. define a backend-neutral Suite resource or workflow;
3. expose it through the versioned Client API;
4. build the normal frontend from that domain API;
5. retain OSD access only where no adequate domain surface exists.

The following functions must not remain OSD-only merely because they can be reached through the bridge:

- normal EPG browsing;
- channel navigation;
- timer management;
- SearchTimer management;
- recording browsing and actions;
- backend administration;
- permission management;
- streaming session management;
- diagnostics that can be represented as structured data.

An OSD bridge is acceptable for:

- legacy plugin menus not yet modeled by VDR-Suite;
- controlled diagnostics or maintenance;
- transition and compatibility workflows;
- administrator-requested access to a native menu;
- source validation during implementation.

A bridge session must be visibly labeled as a legacy compatibility surface. Clients must not present it as the preferred modern workflow.

---

## Identity Model

The bridge uses these identities:

| Identity | Meaning |
| --- | --- |
| `legacyOsdSessionId` | Stable opaque Suite identity for one authorized compatibility session. |
| `sessionRevision` | Opaque revision of session lifecycle and policy state. |
| `osdSurfaceId` | Stable bounded identity for one observed native OSD surface within a backend generation. |
| `osdEpoch` | Monotonic or opaque epoch that changes when the native surface is recreated, reset or loses continuity. |
| `frameSequence` | Ordered sequence of complete frames within one OSD epoch. |
| `eventSequence` | Ordered sequence of OSD changes, deltas or control-related events within one OSD epoch. |
| `viewerBindingId` | Identity of one client viewer attachment. |
| `controllerLeaseId` | Identity of the active controller lease. |
| `controllerLeaseEpoch` | Fencing value changed whenever controller authority changes. |
| `inputCommandId` | Stable identity of one normalized input command. |
| `actorId` | Authenticated Suite actor. |
| `clientInstanceId` | Optional registered or observed client instance. |
| `backendId` | Stable Suite backend identity. |
| `backendGeneration` | Current backend or Agent generation used for fencing. |

These values must not be replaced by:

- a hostname or IP address;
- a browser connection ID alone;
- a raw VDR pointer;
- an OSD object address;
- a file descriptor;
- a process-local plugin handle;
- a user login cookie;
- a raw SVDRP command;
- a remote-control device path.

`legacyOsdSessionId` and `osdSurfaceId` are identifiers, not credentials.

Possession of either value alone never authorizes viewing or control.

---

## Native OSD Surface Semantics

A VDR backend normally exposes one current interactive OSD surface at a time.

VDR-Suite therefore models controller exclusivity at the native surface scope:

```text
backendId
+ backendGeneration
+ osdSurfaceId
+ osdEpoch
```

A LegacyOsdSession is a client-facing authorization and lifecycle object. Several sessions or viewers may observe the same native surface when policy allows.

Only one active VDR-Suite controller lease may exist for the same native surface scope.

The controller lease does not claim exclusive ownership over all input reaching VDR. Physical remotes, local keyboards, HDMI-CEC devices or other trusted local VDR mechanisms may still change the OSD.

Such external changes are authoritative native changes and must appear in subsequent frames or events.

A client must never assume:

- every observed change was caused by its own command;
- no local user can alter the menu;
- one key acknowledgement proves a particular menu action occurred;
- the OSD surface remains the same across a backend restart.

---

## LegacyOsdSession Contract

A LegacyOsdSession carries at least:

| Field | Meaning |
| --- | --- |
| `legacyOsdSessionId` | Stable Suite identity. |
| `sessionRevision` | Opaque lifecycle revision. |
| `actorId` | Actor that created the session. |
| `clientInstanceId` | Optional client instance. |
| `backendId` | Target backend. |
| `backendGeneration` | Generation admitted when the session route is established. |
| `mode` | View-only or control-capable request. |
| `state` | Current session lifecycle state. |
| `createdAt` | Creation timestamp. |
| `expiresAt` | Hard expiry. |
| `idleExpiresAt` | Optional inactivity expiry. |
| `lastActivityAt` | Latest bounded activity. |
| `capabilitySnapshot` | Relevant OSD capabilities observed at admission. |
| `policySnapshot` | Authorization and backend-policy evidence. |
| `osdSurfaceId` | Current bound native surface, when available. |
| `osdEpoch` | Current surface epoch, when available. |
| `closeReason` | Final reason when closed. |
| `correlationId` | Request or audit correlation. |

Canonical session states are:

| State | Meaning |
| --- | --- |
| `requested` | The request is authenticated but not yet admitted. |
| `starting` | Agent and local OSD capability are being resolved. |
| `active` | Viewing is available. Control may still require a separate lease. |
| `degraded` | Viewing continues with reduced fidelity or freshness. |
| `resync_required` | Continuity was lost and a full frame is required. |
| `suspended` | Backend or Agent state temporarily prevents safe progress. |
| `closing` | Cleanup has begun. |
| `closed` | Session ended normally or administratively. |
| `expired` | Hard or idle expiry ended the session. |
| `failed` | Admission or startup failed. |

A session state does not imply controller authority.

An active view-only session has no right to send input.

A session that requested control still requires a current OsdControllerLease before each command is accepted.

---

## Authorization Model

The permission model distinguishes at least:

```text
osd.view
osd.control
osd.session.manage_own
osd.session.manage_all
```

Final public permission names may be refined during ADR-0048 and Phase 62, but the separation is mandatory.

### View permission

`osd.view` permits observation of the current native OSD subject to backend and privacy policy.

It does not permit:

- key input;
- opening a menu;
- closing a menu;
- invoking a plugin action;
- changing a timer, recording, channel or setup value;
- acquiring controller authority.

### Control permission

`osd.control` is a privileged legacy compatibility permission.

It is not equivalent to a harmless UI-navigation permission. Native OSD navigation may indirectly:

- create, update or delete timers;
- delete, move or rename recordings;
- change VDR or plugin configuration;
- trigger plugin-specific operations;
- start or stop replay;
- tune a channel;
- expose PIN or other sensitive prompts.

Because the bridge cannot reliably infer the domain meaning of every menu action, `osd.control` cannot provide the same fine-grained least-privilege guarantees as normal VDR-Suite domain APIs.

Therefore:

- `osd.control` is denied by default;
- it requires an explicit grant, not only generic backend visibility;
- it is denied on a server-enforced read-only backend;
- it is denied when backend policy disables legacy interactive control;
- it must not be used to bypass missing domain mutation permissions;
- it is intended for trusted users or administrators where compatibility outweighs fine-grained authorization;
- clients should direct users to domain APIs whenever those APIs exist.

Successful authentication never implies OSD control permission.

### Reauthorization

Authorization is checked:

- when a session is created;
- when a viewer attaches;
- when a controller lease is requested or renewed;
- before every input command;
- after relevant policy or backend-state changes;
- during administrative revocation.

A previously granted controller lease does not survive a permission revocation.

---

## OsdViewerBinding

An OsdViewerBinding represents one client connection observing a LegacyOsdSession or shared native surface.

It carries at least:

- viewer-binding identity;
- actor and client instance;
- session identity and revision;
- backend identity and generation;
- current OSD epoch;
- last acknowledged frame and event sequence;
- attachment and last-seen timestamps;
- requested rendering capabilities;
- degraded or resync state.

Multiple viewers may be attached concurrently when limits and policy allow.

A viewer has no control authority merely because another viewer in the same session holds the controller lease.

Every concrete client connection is authenticated and authorized independently.

Viewer limits may be enforced per:

- actor;
- client instance;
- session;
- backend;
- site;
- deployment.

A disconnected viewer does not terminate the native OSD or other viewers.

---

## OsdControllerLease

Controller authority is represented by an explicit, time-bounded lease.

A lease carries at least:

| Field | Meaning |
| --- | --- |
| `controllerLeaseId` | Stable lease identity. |
| `controllerLeaseEpoch` | Fencing value for authority changes. |
| `legacyOsdSessionId` | Owning client session. |
| `viewerBindingId` | Controlling client binding. |
| `actorId` | Authorized controller actor. |
| `clientInstanceId` | Controlling client instance. |
| `backendId` | Target backend. |
| `backendGeneration` | Fenced backend generation. |
| `osdSurfaceId` | Target surface. |
| `osdEpoch` | Target surface epoch. |
| `grantedAt` | Lease grant time. |
| `expiresAt` | Hard lease expiry. |
| `renewAfter` | Renewal boundary. |
| `lastHeartbeatAt` | Latest accepted controller heartbeat. |
| `state` | Requested, active, expiring, revoked, expired or released. |
| `revocationReason` | Reason when authority ends. |

Only one active controller lease may exist for one native surface scope.

The Control Plane is authoritative for lease ownership.

The Backend Agent enforces the current lease and epoch at command delivery.

The plugin or local adapter may apply an additional local fencing check, but it does not independently choose the global controller.

### Lease rules

- leases are short-lived and renewable;
- expiry is based on server-controlled time;
- renewal requires current authorization and active backend generation;
- a new controller increments or replaces the controller-lease epoch;
- commands from an old epoch are rejected;
- releasing a lease is idempotent;
- administrative revocation is immediate at the Control Plane and propagated to the Agent;
- backend restart invalidates all leases for the old generation;
- OSD epoch change invalidates commands targeting the previous surface;
- client disconnect does not leave a permanent controller lock;
- a bounded grace interval may exist for transient reconnect, but no input is accepted without a current lease;
- controller ownership is visible to authorized viewers without exposing private credentials.

### Controller handover

A controller may release the lease voluntarily.

A waiting actor may request authority according to policy.

The default handover policy is explicit, not silent stealing.

Administrative takeover may revoke the previous lease and create a new epoch.

Queued input from the previous controller is discarded after revocation.

---

## OSD Frame and Delta Model

The bridge publishes immutable render observations.

### OsdFrame

A full OsdFrame carries at least:

- backend identity and generation;
- OSD surface identity and epoch;
- frame sequence;
- capture timestamp;
- logical dimensions and pixel-aspect information where known;
- rendering schema and encoding version;
- complete normalized render content;
- visibility and active/inactive state;
- content fingerprint;
- bounded quality or truncation diagnostics;
- correlation to the event sequence that produced it.

The exact rendering representation may be:

- a bounded bitmap or image frame;
- normalized OSD areas and palette data;
- a versioned scene description;
- another deterministic adapter representation.

ADR-0047 does not mandate one encoding. It mandates complete-state and continuity semantics.

A full frame is sufficient to reconstruct the current client render state without older frames.

### OsdDelta

A delta carries at least:

- backend identity and generation;
- OSD surface identity and epoch;
- base frame sequence;
- resulting frame or event sequence;
- versioned delta encoding;
- bounded changed regions or commands;
- content fingerprint where applicable.

A client applies a delta only when:

```text
backendGeneration matches
AND osdEpoch matches
AND local frameSequence equals baseFrameSequence
```

Otherwise the client enters `resync_required` and requests or waits for a full frame.

Clients must not guess missing pixels, menu entries, cursor positions or state transitions.

### Full resynchronization

A full frame is required after:

- initial attachment;
- sequence gap;
- buffer overflow;
- unsupported delta version;
- backend generation change;
- OSD epoch change;
- Agent reconnect without proven continuity;
- client rendering reset;
- explicit administrative request.

The Agent or plugin may emit a bounded `resync_required` marker instead of retaining an unbounded history.

---

## Sequencing and Backpressure

OSD updates require ordered delivery semantics.

The shared contract distinguishes:

- `backendGeneration` — backend/Agent process authority;
- `osdEpoch` — continuity of one native OSD surface;
- `frameSequence` — complete render observations;
- `eventSequence` — ordered changes and state events;
- `controllerLeaseEpoch` — current input authority;
- `inputCommandSequence` — optional ordering of accepted commands for one controller lease.

These values are never treated as interchangeable counters.

The local adapter or plugin uses bounded queues or dirty-state markers. It must not block VDR callbacks waiting for a client or network connection.

When producers outrun consumers:

- intermediate deltas may be coalesced or dropped according to the versioned contract;
- the latest complete frame may replace obsolete queued frames;
- sequence loss is reported;
- the client resynchronizes;
- memory use remains bounded;
- VDR threads remain non-blocking.

No guarantee of replaying every visual intermediate state is required.

The required guarantee is that a client can detect lost continuity and recover to one authoritative current frame.

---

## OsdInputCommand

Public clients do not send raw shell, SVDRP, plugin-service or device commands.

They send normalized allowlisted OSD input actions.

The minimum portable vocabulary may include:

```text
up
down
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
fast_forward
rewind
next
previous
channel_up
channel_down
volume_up
volume_down
mute
number_0 through number_9
```

Support is capability-driven. A backend or adapter may expose only a safe subset.

An OsdInputCommand carries at least:

| Field | Meaning |
| --- | --- |
| `inputCommandId` | Stable command identity. |
| `legacyOsdSessionId` | Authorizing session. |
| `sessionRevision` | Expected current session revision. |
| `controllerLeaseId` | Required active lease. |
| `controllerLeaseEpoch` | Fencing value. |
| `actorId` | Authenticated actor. |
| `clientInstanceId` | Client instance. |
| `backendId` | Target backend. |
| `backendGeneration` | Expected generation. |
| `osdSurfaceId` | Target surface. |
| `osdEpoch` | Expected surface epoch. |
| `action` | Allowlisted normalized key action. |
| `inputMode` | Press, bounded repeat or another versioned supported mode. |
| `repeatCount` | Optional bounded repeat count. |
| `deadline` | Latest safe dispatch time. |
| `correlationId` | Audit and request correlation. |

### Input safety rules

- action names are allowlisted;
- unknown actions are rejected;
- arbitrary strings are not passed to VDR;
- arbitrary numeric key codes are not accepted from public clients;
- shell commands are prohibited;
- raw SVDRP commands are prohibited;
- unrestricted plugin-service calls are prohibited;
- device-path input injection is prohibited;
- command and repeat rates are bounded;
- queue length is bounded;
- stale lease, generation or OSD epoch is rejected;
- expired commands are rejected before dispatch;
- only one command stream is active for one controller lease;
- stuck-key state is cleared during disconnect, expiry and shutdown;
- adapter translation is explicit and testable.

The bridge must not offer an escape action such as:

```text
execute(command)
runSvdrp(text)
callPlugin(service, payload)
pressRawKey(number)
```

### Input acknowledgement is not domain success

Acceptance of a key command proves only that the bounded input was admitted or delivered according to the local contract.

It does not prove that a timer was created, a recording was deleted, a setting was saved or any other domain effect occurred.

The next authoritative OSD frame may show the visible result, but OSD observation is still not a replacement for domain-level mutation verification.

For normal Suite operations, clients use the domain APIs defined by ADR-0042 and related domain contracts.

---

## OsdInputResult

An input result distinguishes at least:

```text
accepted_for_dispatch
rejected_unauthorized
rejected_no_controller_lease
rejected_stale_lease
rejected_backend_generation
rejected_osd_epoch
rejected_session_state
rejected_rate_limit
rejected_unsupported_action
rejected_read_only_backend
rejected_policy
expired_before_dispatch
native_rejected
dispatched_unverified
local_processing_failure
outcome_unknown
```

A result carries bounded evidence:

- input command identity;
- controller lease and epoch;
- backend generation and OSD epoch;
- local dispatch timestamp;
- normalized action;
- result category;
- optional native response category;
- sequence observed before and after dispatch when known;
- correlation ID;
- safe diagnostics without secrets or raw internal objects.

The bridge does not silently retry a command after an unknown dispatch outcome.

Interactive key commands are time-sensitive. A stale command is discarded rather than replayed later.

---

## Local Physical Input and External Changes

VDR may receive input outside VDR-Suite.

Examples include:

- infrared remote control;
- local keyboard;
- HDMI-CEC;
- another trusted local plugin;
- an administrator using a local console.

The VDR-Suite controller lease coordinates VDR-Suite clients. It does not block or impersonate native local input.

When local input changes the OSD:

- the native OSD remains authoritative;
- the resulting frame or event is published normally;
- clients must not attribute the change to their own command without evidence;
- controller lease ownership remains unchanged unless explicit policy revokes it;
- optional local-activity policy may suspend or revoke remote control for privacy or safety;
- any such policy is explicit and audit-visible.

A deployment may configure local-presence priority. For example, recent physical input may temporarily deny new remote controller leases.

The default contract does not invent a universal local-user identity for raw physical input.

---

## Backend Generation and OSD Epoch

Backend generation and OSD epoch fence different failure modes.

`backendGeneration` changes when the authoritative backend or Agent process generation changes under ADR-0040.

`osdEpoch` changes when OSD continuity is lost even within the same backend generation, for example:

- the native OSD is destroyed and recreated;
- an adapter restarts its capture state;
- rendering mode changes incompatibly;
- a full reset invalidates prior frame sequence;
- the plugin reports an unrecoverable sequence discontinuity.

Rules:

- a command targets one backend generation and one OSD epoch;
- a frame belongs to one generation and epoch;
- old-generation or old-epoch commands are rejected;
- controller leases are invalidated when their fenced generation or epoch is no longer current;
- clients request a full frame after an epoch change;
- sequence numbers may restart only with an explicit new epoch;
- reconnect does not preserve continuity unless the Agent proves the same generation, epoch and sequence history.

---

## Agent Disconnect, Reconnect and Offline Behavior

A remote-site Agent normally connects outbound to the Control Plane.

No public inbound OSD plugin port is required.

When the Agent connection is lost:

- new viewer attachments are rejected or reported unavailable;
- existing clients display stale/degraded state explicitly;
- controller input is suspended immediately;
- controller leases may be revoked or allowed only a short no-input reconnect grace period;
- no commands are queued for delayed replay;
- the last frame may remain visible only with a stale marker;
- session expiry continues according to Control Plane time.

After reconnect:

- Agent identity and backend generation are revalidated;
- current OSD capability is republished;
- a full frame is required unless exact continuity is proven;
- old controller-lease epochs are rejected;
- clients reattach or renew according to policy;
- queued stale keys are never replayed.

If the backend generation changed, the previous native surface and all associated leases are obsolete.

---

## Rendering Fidelity and Capability Model

OSD rendering capabilities are explicit and versioned.

Possible capabilities include:

```text
osd.snapshot.read
osd.delta.read
osd.input
osd.controller.lease
osd.bitmap
osd.palette
osd.alpha
osd.multiarea
osd.logical_geometry
osd.cursor_metadata
osd.visibility_state
osd.local_activity_hint
osd.privacy_suppression
```

Capability reports include:

- state: available, degraded, disabled or unsupported;
- contract/schema version;
- backend generation;
- adapter origin;
- maximum dimensions and payload size;
- supported frame and delta encodings;
- supported input actions and repeat behavior;
- freshness expectations;
- whether control is disabled by policy;
- privacy or redaction limitations.

A capability is not inferred solely from the installed plugin name.

The bridge must degrade truthfully when:

- only full snapshots are available;
- deltas are unavailable;
- alpha or palette fidelity is reduced;
- an OSD is inactive;
- input translation is unsupported;
- privacy policy disables capture;
- the local adapter becomes unhealthy.

Clients render degraded state explicitly rather than pretending full fidelity.

---

## Privacy and Sensitive OSD Content

OSD frames may contain sensitive information.

Examples include:

- recording names;
- search history;
- plugin configuration;
- host or network details;
- account names;
- PIN prompts;
- conditional-access messages;
- filesystem paths;
- diagnostic tokens or secrets displayed by unsafe plugins.

Therefore:

- OSD viewing always requires authentication and authorization;
- frames are transported only over protected client and Agent paths;
- frames are not stored in normal logs;
- frame payloads are not included in durable audit records;
- caches are bounded and access-controlled;
- browser and proxy caching is disabled or tightly controlled;
- screenshots are not retained by default;
- secret-like input values are not logged;
- diagnostics contain fingerprints, sizes and categories rather than screen contents;
- an adapter may report privacy suppression instead of capturing a protected surface;
- administrative exports require a separate explicit feature and policy.

The bridge cannot guarantee semantic redaction of arbitrary plugin UIs. Deployments requiring strict confidentiality disable legacy OSD viewing or restrict it to trusted administrators.

---

## Separation From Media Streaming

OSD transport is not the media byte path defined by ADR-0046.

```text
MediaSession
  carries live television or Recording bytes

LegacyOsdSession
  carries compatibility UI observations and bounded input
```

They have separate:

- identities;
- permissions;
- expiry;
- transport and backpressure requirements;
- capability models;
- audit events;
- failure handling.

A client may visually overlay an OSD frame over video, but that presentation does not merge the two server-side session contracts.

Starting video playback does not automatically create an OSD controller lease.

Holding an OSD controller lease does not automatically grant media playback.

---

## Relation to osd2web, RESTfulAPI and SVDRP

`vdr-plugin-osd2web` remains a source of VDR-specific knowledge and may inform a local adapter implementation.

It is not the public VDR-Suite client contract.

RESTfulAPI OSD and remote endpoints may be used behind an adapter for compatibility or validation when capabilities and safety are proven.

They are not public authorization boundaries.

SVDRP may be used as a local transport for specific allowlisted operations where the adapter contract proves safe translation.

Raw SVDRP is not exposed to clients.

No implementation is approved merely because an upstream endpoint exists.

Every adapter must satisfy:

- capability truthfulness;
- backend-generation fencing;
- bounded payloads;
- sequence and resync semantics;
- controller-lease enforcement;
- input allowlisting;
- privacy rules;
- live acceptance on supported VDR versions.

---

## VDR Plugin and Native Safety Rules

A future `vdr-plugin-suite-bridge` OSD slice follows the shared Gold-Standard rules.

### Native state copying

- acquire only the VDR-recommended lock for the relevant OSD state;
- copy bounded values into plugin-owned immutable structures;
- release the lock before encoding, compression, logging, disk I/O or network transport;
- never expose raw pointers, iterators, lock guards or object addresses;
- never retain native OSD objects across callbacks or Agent calls.

### Callback behavior

VDR callbacks may:

- mark OSD state dirty;
- increment a bounded sequence;
- enqueue a bounded change marker;
- copy a small immutable event when proven safe.

They must not:

- open network connections;
- wait for clients;
- perform compression or large serialization;
- access the Control Plane database;
- block on controller arbitration;
- write files;
- execute unrelated commands;
- hold VDR locks while logging large payloads.

### Input behavior

- only normalized allowlisted actions are translated;
- translation is explicit and versioned;
- unsupported keys are rejected;
- stale lease/generation/epoch is rejected before native dispatch where possible;
- no unbounded repeat or key queue exists;
- shutdown clears pending or pressed-key state;
- input execution never creates a plugin-owned durable job scheduler.

### Lifecycle

- start does not publish a capability until initialization is complete;
- stop disables callbacks and input before dependent state is destroyed;
- restart creates new continuity where required;
- no stale listener, worker, file, lease or registration remains after rollback;
- live acceptance proves load, observe, control rejection/acceptance and cleanup behavior.

---

## Rate Limiting and Abuse Protection

OSD input and frame delivery are independently rate-limited.

Input limits may include:

- commands per second;
- bounded repeat count;
- maximum queued commands;
- minimum interval for expensive actions;
- per-actor and per-client limits;
- backend-wide limits;
- automatic lease revocation after abuse.

Frame limits may include:

- maximum full-frame rate;
- maximum delta rate;
- maximum payload bytes;
- maximum retained frames;
- maximum viewers;
- compression work budget;
- per-site bandwidth budget.

Exceeding a rendering budget degrades or resynchronizes the viewer. It must not block VDR.

Exceeding an input budget rejects commands and may revoke the controller lease.

Rate-limit diagnostics do not expose screen content or credentials.

---

## Session, Lease and Adapter Failure Categories

Minimum semantic categories include:

```text
osd_unavailable
osd_inactive
osd_capture_degraded
osd_capture_suppressed
osd_sequence_gap
osd_resync_required
osd_schema_unsupported
viewer_limit_reached
controller_busy
controller_lease_expired
controller_lease_revoked
controller_epoch_conflict
backend_generation_conflict
osd_epoch_conflict
input_unsupported
input_rate_limited
read_only_backend
policy_denied
agent_offline
native_rejected
local_processing_failure
outcome_unknown
```

These categories remain distinct from final HTTP status mapping, which belongs to ADR-0048.

Internal plugin class names, VDR pointers, hostnames, paths and credentials are not returned to public clients.

---

## Audit and Observability Boundary

The bridge emits bounded lifecycle, security and operational facts.

Audit-relevant events include:

- session requested, granted, denied, expired and closed;
- viewer attached and detached;
- controller lease requested, granted, renewed, released, revoked and expired;
- input accepted or rejected;
- administrative takeover;
- read-only or policy denial;
- backend-generation or OSD-epoch conflict;
- privacy suppression;
- abnormal rate-limit or abuse behavior.

Operational metrics include:

- active viewers and controllers;
- frame and delta rates;
- payload sizes;
- dropped or coalesced updates;
- resync count;
- input latency;
- Agent and adapter health;
- rate-limit counts.

Normal audit and logs do not store:

- complete OSD frames;
- secret input;
- access credentials;
- raw provider payloads;
- arbitrary plugin messages containing private data.

ADR-0049 defines the final audit and security-event schema.

---

## Persistence Boundary

The Control Plane may persist:

- LegacyOsdSession lifecycle records;
- viewer and controller-lease metadata;
- authorization decisions;
- bounded command result categories;
- audit correlation;
- administrative policy.

The Control Plane does not need to persist every OSD frame.

Frames and deltas are normally transient bounded transport data.

The Agent may maintain bounded in-memory buffers for current frames and resynchronization.

The plugin does not create a durable OSD database.

No client or Agent accesses the Control Plane database schema directly.

---

## Multi-Site Deployment

For a remote backend, the normal topology is:

```text
Client
  -> VDR-Suite Client API and live event path
  -> Control Plane
  -> authenticated outbound Agent connection
  -> Backend Agent OSD broker
  -> local OSD adapter
  -> VDR
```

The remote site does not expose:

- an osd2web port;
- RESTfulAPI `/osd` or `/remote` publicly;
- SVDRP publicly;
- the Suite Bridge plugin directly;
- a local remote-control socket;
- VDR-internal credentials.

A compromised Agent can affect only its enrolled backend scope. It cannot claim controller authority for another backend or submit frames for another generation.

Site and backend policy may disable OSD entirely even when other read access is allowed.

---

## Compatibility and Versioning

The following contracts evolve independently:

- LegacyOsdSession API schema;
- OSD capability schema;
- frame encoding schema;
- delta encoding schema;
- input-action schema;
- controller-lease schema;
- Agent OSD transport schema;
- plugin-local OSD contract schema.

Rules:

- existing field meaning is never changed silently;
- additive optional fields define defaults;
- incompatible frame or delta changes require a new version;
- unsupported versions are rejected or degraded explicitly;
- a client may fall back from deltas to full frames when supported;
- input is disabled when action semantics cannot be negotiated safely;
- control is never enabled through optimistic version guessing;
- older Agents or plugins report truthful degraded or unsupported capability state.

ADR-0048 defines the final public API compatibility and deprecation policy.

---

## Migration Strategy

Any existing or experimental direct OSD access is migrated in stages.

### Stage 1: inventory and disable public direct exposure

- identify osd2web, RESTfulAPI OSD/remote, SVDRP and other remote-control paths;
- remove public or cross-site exposure by default;
- document local compatibility dependencies;
- preserve rollback.

### Stage 2: read-only local observation

- introduce bounded native OSD snapshots;
- keep input disabled;
- define capability, epoch, frame sequence and full-resync behavior;
- prove VDR lock and callback safety;
- complete controlled live acceptance.

### Stage 3: Agent transport and view-only sessions

- transport frames through the authenticated Agent boundary;
- implement `osd.view` authorization;
- support multiple viewers and bounded resynchronization;
- keep control disabled.

### Stage 4: controller lease without native input

- implement durable or authoritative Control Plane lease arbitration;
- enforce controller-lease epochs in the Agent;
- test expiry, revocation, reconnect and takeover;
- keep native input disabled until the complete boundary passes.

### Stage 5: allowlisted native input

- add the smallest supported key vocabulary;
- require explicit `osd.control` permission;
- deny read-only backends;
- enforce generation, OSD epoch, lease epoch, rate and deadline;
- perform controlled live acceptance and rollback.

### Stage 6: optional fidelity improvements

- add deltas, palettes, alpha, richer geometry or local-activity hints only through versioned capabilities;
- preserve full-frame resynchronization;
- do not weaken the domain-first architecture.

---

## Implementation Sequence

Phase 67 should proceed in bounded slices:

1. shared OSD identities, states and capability vocabulary;
2. read-only OsdFrame contract;
3. backend-local adapter abstraction;
4. sequence, epoch and full-resync behavior;
5. Agent transport and buffering;
6. Control Plane LegacyOsdSession service;
7. `osd.view` authorization and viewer bindings;
8. controller-lease service and fencing;
9. allowlisted input request/result contract;
10. rate limits, privacy and audit integration;
11. frontend legacy-surface module;
12. controlled local and multi-site acceptance;
13. direct-endpoint migration and rollback documentation.

Each slice begins read-only unless it explicitly proves all prerequisites for input.

No slice may expose a public raw command tunnel.

---

## Alternatives Rejected

### Use the VDR OSD as the primary frontend

Rejected because it conflicts with ADR-0030, backend-neutral domain APIs, modern multi-client UX and fine-grained authorization.

### Publish osd2web directly

Rejected because an upstream plugin port must not become the public VDR-Suite security and compatibility boundary.

### Publish RESTfulAPI `/osd` and `/remote` directly

Rejected because RESTfulAPI remains an internal compatibility adapter and does not implement the Suite user, lease, multi-site, sequencing or policy model.

### Expose raw SVDRP

Rejected because raw protocol commands are not a safe public domain API and could bypass authorization and command allowlisting.

### Allow every viewer to send keys

Rejected because concurrent input would be nondeterministic and unsafe.

### Use a permanent controller owner

Rejected because disconnects, browser crashes and stale clients would strand or retain authority.

### Queue remote keys while the Agent is offline

Rejected because interactive commands are stale by nature and could execute in an unrelated future menu state.

### Infer state after missing deltas

Rejected because a client must detect discontinuity and request a full authoritative frame.

### Treat read-only backend as view plus OSD navigation

Rejected because OSD navigation can indirectly mutate native VDR state.

### Store all OSD frames for audit

Rejected because it creates excessive storage, privacy and secret-retention risk without improving authorization evidence.

---

## Consequences

Positive:

- preserves access to legacy VDR and plugin workflows;
- keeps the normal product domain-first;
- separates viewers from controllers;
- prevents multiple Suite clients from interleaving keys;
- fences stale controllers after restart, reconnect or takeover;
- makes sequence loss detectable and recoverable;
- keeps plugin ports and VDR credentials private;
- supports local and multi-site deployments through one logical boundary;
- keeps VDR locks and callbacks isolated from network latency;
- prevents arbitrary command execution;
- gives read-only backend policy a deterministic denial boundary;
- enables gradual migration from OSD-only functions to normal Suite domains.

Trade-offs:

- OSD rendering is visually and operationally more complex than structured domain data;
- graphical fidelity may vary by adapter and VDR version;
- one native OSD surface limits independent interactive sessions;
- physical local input can change state outside the Suite controller lease;
- fine-grained authorization inside opaque plugin menus is impossible;
- controller arbitration and resynchronization require distributed state;
- high-frequency frames require careful bandwidth and backpressure handling;
- privacy policy may require disabling the feature in some deployments;
- live VDR acceptance is required for every supported native adapter path.

---

## Non-Goals

This ADR does not define:

- final public endpoint paths;
- one mandatory frame encoding;
- one mandatory image codec;
- one mandatory WebSocket or SSE transport;
- a full browser implementation;
- pixel-perfect compatibility with every VDR skin;
- semantic parsing of arbitrary plugin menus;
- automated conversion of OSD menus into domain APIs;
- fine-grained authorization for every possible native menu action;
- a shell, raw SVDRP or generic plugin-service API;
- video streaming or media-session transport;
- permanent OSD frame recording;
- final HTTP error mapping;
- final audit-event schema;
- implementation of Phase 67.

It defines the mandatory ownership, identity, authorization, lease, sequencing, resynchronization, input and native-safety boundary.

---

## Acceptance Criteria

ADR-0047 is implemented only when:

- LegacyOsdSession, OsdSurfaceRef, OsdViewerBinding, OsdFrame, OsdDelta, OsdControllerLease, OsdInputCommand and OsdInputResult are distinct implemented concepts;
- the primary product remains domain-first;
- clients never require a permanent direct OSD plugin, RESTfulAPI remote or SVDRP endpoint;
- `osd.view` and `osd.control` are separately enforced;
- server-enforced read-only backends reject OSD control;
- several viewers can observe one surface within bounded limits;
- only one VDR-Suite controller lease is active per native surface scope;
- controller-lease epochs reject stale commands;
- backend generation and OSD epoch are separately fenced;
- full frames provide authoritative resynchronization;
- sequence gaps are detected rather than guessed;
- queues, buffers and frame retention remain bounded;
- Agent disconnect never causes delayed key replay;
- public input is allowlisted and rate-limited;
- no arbitrary shell, SVDRP, plugin-service or raw key channel exists;
- VDR locks and callbacks remain non-blocking and bounded;
- raw VDR pointers and lock ownership never cross the adapter boundary;
- privacy-sensitive content is not written to normal logs or audit records;
- session, lease and input events are audit-correlated without retaining frames;
- local physical input and remote controller behavior are documented and tested;
- migration and rollback from direct endpoints are documented;
- full CI and controlled live VDR acceptance pass.

Acceptance of this ADR is not runtime completion.

---

## Related Decisions

- [ADR-0007: RESTfulAPI Adapter Boundary](ADR-0007-restfulapi-adapter-boundary.md)
- [ADR-0012: Source Capability Model](ADR-0012-source-capability-model.md)
- [ADR-0013: Permission Model](ADR-0013-permission-model.md)
- [ADR-0016: Snapshot Change Feed Architecture](ADR-0016-snapshot-change-feed-architecture.md)
- [ADR-0018: Incremental Snapshot Synchronization](ADR-0018-incremental-snapshot-synchronization.md)
- [ADR-0030: Domain-First UI Over OSD Proxy](ADR-0030-domain-first-ui-over-osd-proxy.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0046: Streaming Gateway and Media Session Boundary](ADR-0046-streaming-gateway-media-session-boundary.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
