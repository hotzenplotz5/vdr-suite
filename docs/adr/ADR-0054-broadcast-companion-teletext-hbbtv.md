# ADR-0054: Broadcast Companion Services — Teletext and HbbTV

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [ADR Index](index.md)
- [Current State](../CURRENT.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)
- [ADR-0030: Domain-First UI Over OSD Proxy](ADR-0030-domain-first-ui-over-osd-proxy.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0046: Streaming Gateway and Media Session Boundary](ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0053: Client Playback Engine and Media Adaptation Strategy](ADR-0053-client-playback-engine-media-adaptation-strategy.md)
- [ADR-0047: Legacy OSD Compatibility Bridge](ADR-0047-legacy-osd-compatibility-bridge.md)

---

## Status

**Accepted**

Date: 2026-08-17

Accepted during the post-Phase-64 roadmap reconciliation. The Broadcast Companion architecture remains accepted. Its original future phase numbering is superseded by accepted ADR-0058: Broadcast Companion runtime is now Phase 67 and is not started by this ADR.

---

## Context

VDR-Suite intends to provide a modern television experience beyond EPG, Recording and Live playback. Two traditional broadcast companion capabilities are currently absent from the canonical Suite architecture and roadmap:

- Teletext / Videotext;
- HbbTV broadcast applications.

Both exist in the VDR ecosystem, but their existing implementations do not define the desired VDR-Suite public architecture.

`vdr-plugin-osdteletext` demonstrates Teletext reception, caching, page/subpage navigation, color-key behavior and OSD rendering. Its useful underlying capability is the Teletext page/service data. The VDR OSD is only one rendering implementation.

Existing VDR HbbTV plugins demonstrate application discovery from broadcast information and coupling to an external browser/application runtime. Some implementations expose local control surfaces such as URL loading, JavaScript execution, key injection or player attach/detach. Those are useful implementation references but are not safe or stable public VDR-Suite contracts.

ADR-0030 already establishes the primary rule:

> Model the underlying domain capability directly; do not make the classic VDR OSD the primary product model.

ADR-0047 separately defines a later Legacy OSD Compatibility Bridge for genuinely opaque native/plugin workflows.

Therefore Teletext and HbbTV need a first-class domain decision before Legacy OSD runtime is treated as the solution.

---

## Decision

VDR-Suite introduces a **Broadcast Companion** domain containing two distinct capability families:

```text
Broadcast Companion
  +--> Teletext Service / Page Domain
  +--> Broadcast Application / HbbTV Domain
```

They may share channel/programme context, backend generation, authorization, Agent transport and client presentation surfaces, but they do not share one identity or lifecycle model.

The high-level direction is:

```text
LiveChannel / ProgramEvent
  |
  +--> TeletextServiceRef
  |      -> TeletextPageRef
  |      -> TeletextPage / TeletextSubpage
  |
  +--> BroadcastApplicationRef
         -> BroadcastApplicationDescriptor
         -> BroadcastApplicationSession
         -> sandboxed HbbTV-capable application runtime
```

Phase-65 MediaSession/Gateway contracts remain authoritative for Suite-managed media delivery where HbbTV/application playback uses Suite media resources.

Legacy OSD remains separate:

```text
Teletext != LegacyOsdSession
HbbTV != LegacyOsdSession
BroadcastApplicationSession != LegacyOsdSession
```

A client may visually combine Live TV, Teletext, HbbTV and OSD, but the server-side resource contracts remain distinct.

---

# Teletext Domain

## TeletextServiceRef

A Teletext service reference identifies the Teletext capability associated with one current backend/channel/broadcast-service context.

It carries or resolves at least:

```text
backendId
backendGeneration
channelIdentity / broadcastServiceIdentity
providerIdentity
providerGeneration or capability revision
service availability
observation/freshness evidence
```

A service reference is not a provider cache directory or VDR plugin pointer.

## TeletextPageRef

One page reference contains at least:

```text
TeletextServiceRef
pageNumber
subpage identity when required
```

Page identity remains stable only within its explicit broadcast service and backend/generation context.

The client does not infer global identity from page number `100` alone.

## TeletextPage / TeletextSubpage

The normalized page representation may include:

- page/subpage numbers;
- page revision or observation sequence;
- received/observed time;
- normalized character cells or another bounded versioned rendering representation;
- foreground/background color information;
- control attributes needed for a faithful supported presentation;
- navigation/link metadata where safely derived;
- concealed/reveal information where represented;
- completeness/degradation markers.

The exact wire representation is versioned and must remain independent of one plugin's internal cache format.

## Teletext provider boundary

A backend-local provider may use:

- a dedicated SuiteBridge/VDR-native Teletext adapter;
- an explicitly reviewed existing Teletext plugin/data source;
- another bounded VDR-local extraction path.

The provider contract must not expose to clients:

- local cache filenames;
- VDR/plugin object addresses;
- plugin-private binary structures;
- local filesystem paths;
- raw direct plugin control ports.

Provider reachability does not create authority. Current provider selection is explicit and generation/capability fenced.

## Teletext cache and freshness

Teletext is observation data.

Rules:

- local/provider caches remain bounded;
- current-page freshness is explicit;
- stale cached pages may be exposed only with truthful stale/freshness state;
- backend/channel change does not silently reuse pages as if they belonged to the new service;
- an incomplete page is not represented as authoritative complete state;
- client navigation may request a page/subpage but does not directly manipulate provider cache files.

## Teletext UI direction

The normal first-party flow is domain-first:

```text
Live TV
  -> Teletext available indicator
  -> Open Teletext
  -> numeric page selection
  -> page/subpage navigation
  -> supported color/link navigation
  -> close to Live TV
```

Rendering may appear as a full screen, overlay or side panel according to client design. That presentation choice does not change the domain contract.

## Teletext subtitles

Teletext subtitle pages are potentially valuable, but they are not automatically equivalent to a normalized timed media subtitle track.

A later implementation may promote supported Teletext subtitle pages into Phase-65 playback track semantics only after timing, language, page selection, discontinuity and synchronization behavior are explicitly defined and tested.

No client should be told that a Teletext subtitle track is available merely because a page exists.

---

# HbbTV / Broadcast Application Domain

## Application discovery

VDR-Suite models broadcast-associated application discovery as provider evidence.

A backend-local provider may inspect AIT/DSM-CC or another proven broadcast signaling source and normalize it into `BroadcastApplicationDescriptor` values.

A descriptor contains bounded facts such as:

```text
BroadcastApplicationRef
backendId
backendGeneration
broadcast service / channel identity
organization/application identity where available
application type/profile/version facts
autostart / red-button semantics
entry-point reference
control-code / lifecycle facts
provider/provenance revision
observedAt
availability / degradation state
```

The exact source protocol field names are not automatically public VDR-Suite fields.

A discovered HTTP/HTTPS URL is application evidence, not sufficient global Suite identity by itself.

## BroadcastApplicationRef

The stable reference must distinguish at least:

- backend/site context;
- backend generation where native discovery continuity matters;
- broadcast service/channel;
- broadcaster/application identity;
- application revision/observation where necessary.

Changing channel or backend generation invalidates stale application context unless an explicit application contract proves otherwise.

## BroadcastApplicationSession

Launching an HbbTV application creates an explicit Suite-owned session.

A session carries at least:

```text
broadcastApplicationSessionId
actor / client context
BroadcastApplicationRef
backendId / generation
application descriptor revision
state
createdAt / expiresAt
application runtime capability profile
close reason
correlation/accountability context
```

Possible lifecycle states include:

```text
requested
starting
active
degraded
suspended
closing
closed
expired
failed
```

A session identity is not a browser bearer token and not an unrestricted URL launcher.

## Application runtime boundary

The HbbTV application executes in a **sandboxed HbbTV-capable browser/application runtime** appropriate to the client/deployment.

VDR-Suite does not define one universal browser engine as part of the public architecture.

Implementations may use:

- a browser with the required HbbTV compatibility layer;
- a television platform's HbbTV runtime where safely integrable;
- a dedicated isolated application runner;
- another proven engine behind the Suite application-session abstraction.

The engine is replaceable. Engine-specific command protocols are private adapters.

## No public raw browser/plugin command API

Existing HbbTV implementations may expose commands conceptually similar to:

```text
load URL
execute JavaScript
send raw key
attach/detach player
start/stop browser
```

These do **not** become a general public VDR-Suite API.

Public clients must not receive arbitrary capabilities such as:

```text
executeJavascript(text)
openArbitraryUrl(url)
runPluginCommand(text)
rawKey(code)
```

The Suite contract exposes normalized application lifecycle and supported input semantics only.

## Application network and origin policy

Broadcaster applications can load remote resources. This creates a security boundary different from normal VDR-Suite frontend code.

The application runtime must define:

- allowed schemes and navigation behavior;
- isolation from Suite credentials and browser-session secrets;
- cookie/storage isolation policy;
- access to local/private network targets;
- mixed-content/TLS handling according to the supported HbbTV profile;
- download/file access restrictions;
- external navigation policy;
- teardown and storage cleanup rules.

An HbbTV page never receives VDR-Suite administrative credentials merely because it runs inside a first-party client.

## Remote and color-key input

HbbTV input is application-scoped and normalized.

Supported actions may include:

```text
up/down/left/right/ok/back
red/green/yellow/blue
number keys
play/pause/stop/seek-related actions where supported
```

The public contract uses named/allowlisted actions, not arbitrary browser key codes.

Input is fenced to the current application session and client context. Stale input after channel/app-session replacement is discarded.

This input model is not the ADR-0047 OSD controller lease. HbbTV application interaction belongs to the active application session; Legacy OSD control remains a separate privileged compatibility capability.

## Media relationship

HbbTV applications may reference video/audio resources.

Three classes must remain distinguishable:

1. **Suite-owned Live/Recording resources** — use Phase-65 MediaSession/Gateway authorization.
2. **Broadcaster application network media** — governed by the isolated HbbTV runtime/network policy.
3. **Backend-local private provider media** — must never be exposed directly merely because an HbbTV application requests it.

No HbbTV integration may bypass MediaSession/provider authority for Suite-owned media.

---

# Authorization and Privacy

Teletext viewing is normally a read capability associated with an authorized backend/channel.

HbbTV application launch is more security-sensitive because it creates an active external application/browser context.

Final permission names may be refined, but policy must distinguish at least:

```text
broadcast.teletext.view
broadcast.hbbtv.launch
broadcast.hbbtv.input
broadcast.session.manage_own
broadcast.session.manage_all
```

Rules:

- successful authentication is not authorization;
- backend scope is explicit;
- read-only backend policy does not automatically prohibit Teletext viewing;
- HbbTV application launch/input may be disabled by deployment policy even when Live TV is allowed;
- no application receives broader backend permissions than the actor/session policy grants;
- provider/browser internals and secrets are not returned to clients;
- app URLs, storage and network activity are logged only through bounded safe diagnostics, not secret-bearing dumps.

---

# Agent and Multi-Site Boundary

For a remote backend:

```text
Client
  -> Control Plane
  -> authenticated Agent path
  -> backend-local Teletext/HbbTV discovery provider
  -> VDR / broadcast data
```

No remote site must expose a public Teletext/HbbTV plugin port merely to support VDR-Suite.

The Agent enforces:

- enrolled backend identity;
- backend generation;
- provider identity/capability revision;
- bounded observations and application-discovery payloads;
- local cleanup when a route/session is closed;
- no submission for another backend/site.

A provider process or plugin does not become a second Control Plane.

---

# Relationship to Legacy OSD

The architectural precedence is explicit:

```text
underlying structured capability available?
  -> model it as a Suite domain

only opaque native/plugin UI available?
  -> Legacy OSD compatibility may be used later
```

Therefore:

- Teletext normal browsing belongs to this ADR, not ADR-0047;
- HbbTV application lifecycle belongs to this ADR, not ADR-0047;
- an old plugin setup page for Teletext/HbbTV configuration may still be reachable through ADR-0047 as legacy compatibility until a structured admin domain exists;
- using OSD frames to prototype a feature does not make OSD the final public contract.

---

# Sequencing Decision

Accepted ADR-0058 now owns the not-yet-started future sequence after completed Phase 65:

```text
Phase 65 - Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 - Media Home and Browse Experience
  -> Phase 67 - Broadcast Companion Services: Teletext and HbbTV
  -> Phase 68 - Legacy OSD Compatibility Bridge
  -> Phase 69 - Public API and Client Compatibility Hardening
  -> Phase 70 - Recommendation and Content Knowledge Graph
```

This supersedes only ADR-0054's former phase-number sequencing statement. It does **not** supersede the Broadcast Companion architecture in this ADR, nor ADR-0047 or ADR-0048.

Completed history through Phase 65 is unchanged. Phase 66 Media Home is next and remains not started until a separate explicit kickoff. Broadcast Companion runtime follows as Phase 67.

# Implementation Direction

After Phase 66 Media Home closes and Phase 67 Broadcast Companion is explicitly authorized, Broadcast Companion implementation should use coherent verticals rather than micro-slices.

## Vertical 1 — Teletext read path

```text
real broadcast Teletext
  -> backend-local provider
  -> normalized TeletextService/Page
  -> Agent/Control Plane read path
  -> browser/TV Teletext view
  -> page navigation
```

No OSD proxy is required.

## Vertical 2 — HbbTV discovery

```text
real broadcast application signaling
  -> backend-local discovery provider
  -> normalized BroadcastApplicationDescriptor
  -> Suite read model
  -> user-visible HbbTV availability
```

No app execution yet until the session/runtime boundary is complete.

## Vertical 3 — HbbTV application session/runtime

```text
user launches discovered application
  -> authorization
  -> BroadcastApplicationSession
  -> isolated runtime
  -> normalized application input
  -> deterministic close/channel-change cleanup
```

## Vertical 4 — Media integration and hardening

- preserve Phase-65 media authorization for Suite-owned media;
- classify broadcaster-network media separately;
- test backend restart/channel change/session expiry;
- test security/origin/storage isolation;
- add product acceptance on representative broadcasts.

---

# Acceptance Criteria

ADR-0054 is implemented only when:

- Teletext and HbbTV are explicit domain capabilities separate from Legacy OSD;
- Teletext service/page/subpage identity and freshness are modeled;
- normal Teletext viewing works without requiring an OSD snapshot contract;
- backend-local provider/cache details remain private;
- HbbTV application discovery retains backend/service/application identity and provenance;
- HbbTV application launch creates an explicit authorized Suite session;
- the application runtime is isolated from Suite administrative/session secrets;
- no public raw URL/JavaScript/plugin-command/key-code control API exists;
- normalized HbbTV input is session-scoped and bounded;
- channel/backend-generation changes invalidate stale application context;
- Suite-owned media continues to use Phase-65 MediaSession semantics;
- multi-site operation uses the authenticated Agent boundary rather than public remote plugin ports;
- real broadcast Teletext and HbbTV acceptance passes for the supported deployment profile;
- rollback leaves no stale browser/application process, provider lease, callback or public listener.

Acceptance of this ADR is not runtime completion.

---

# Alternatives Rejected

## Put Teletext entirely behind Legacy OSD

Rejected. It would preserve the rendering implementation instead of modeling the underlying broadcast data and would violate the domain-first direction.

## Put HbbTV entirely behind Legacy OSD

Rejected. HbbTV is an application/browser lifecycle with network/media behavior, not merely native OSD pixels and keys.

## Expose existing plugin SVDRP/browser commands publicly

Rejected. Arbitrary URL, JavaScript or raw-key control would create an unsafe implementation-specific public API.

## Build one universal Suite browser engine into the server

Rejected. The public architecture needs an application-session/runtime abstraction, not permanent coupling to one browser implementation.

## Let HbbTV pages use private Streamdev/SuiteBridge URLs directly

Rejected. That bypasses Phase-65 media authorization, provider ownership, route fencing and topology privacy.

## Add Teletext/HbbTV only after Legacy OSD and Public API hardening

Rejected as the default roadmap. These are ordinary television product capabilities and should be modeled before the legacy fallback is treated as the answer. Public API hardening can then stabilize already-mature broadcast companion resources.

---

# Consequences

Positive:

- restores Teletext and HbbTV to the explicit VDR-Suite product roadmap;
- keeps ADR-0030 domain-first architecture intact;
- prevents Legacy OSD from becoming a shortcut for normal TV functions;
- keeps provider/plugin/browser internals private and replaceable;
- reuses Phase-65 media boundaries instead of creating a second streaming architecture;
- supports browser, TV and future native clients with shared Suite semantics;
- makes multi-site/backend generation handling explicit;
- enables later public API stabilization over real implemented resources.

Trade-offs:

- Teletext normalization requires more work than mirroring an OSD frame;
- HbbTV browser/application compatibility is platform- and broadcaster-sensitive;
- representative broadcast acceptance is required;
- HbbTV security/network isolation increases client/runtime complexity;
- not every historical plugin feature will be part of the first supported scope;
- Phase 66 adds a new product domain before Legacy OSD compatibility.

---

## Related Decisions

- [ADR-0012: Source Capability Model](ADR-0012-source-capability-model.md)
- [ADR-0016: Snapshot Change Feed Architecture](ADR-0016-snapshot-change-feed-architecture.md)
- [ADR-0017: Live Transport Boundary](ADR-0017-live-transport-boundary.md)
- [ADR-0030: Domain-First UI Over OSD Proxy](ADR-0030-domain-first-ui-over-osd-proxy.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0046: Streaming Gateway and Media Session Boundary](ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0053: Client Playback Engine and Media Adaptation Strategy](ADR-0053-client-playback-engine-media-adaptation-strategy.md)
- [ADR-0047: Legacy OSD Compatibility Bridge](ADR-0047-legacy-osd-compatibility-bridge.md)
- [ADR-0048: Public API Versioning, Error and Compatibility Contract](ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0049: Audit and Security Event Model](ADR-0049-audit-security-event-model.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Strict Roadmap](../planning/roadmap.md)
- [Back to Current State](../CURRENT.md)
- [Back to Documentation Index](../index.md)
