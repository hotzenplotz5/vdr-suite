# Phase 68 - Legacy OSD Compatibility Bridge Kickoff

## Status

Phase 68 is active.

Accepted coherent vertical:

```text
68.A - Read-only OSD observation
PR #304 -> a37bb0c9cd0c262cde40369bb7c66686c3ba995e
real yaVDR / VDR 2.7.9 acceptance: PASS
```

Accepted local continuity vertical:

```text
68.B - Agent-local OSD buffering and resynchronization
PR #308 -> 81b9a28759debf5c87fe72afa793bc3db0a486fa
```

Active coherent vertical:

```text
68.C - Authenticated read-only Agent OSD transport
```

This document is the durable execution checkpoint for the Phase-68 start. It exists so a later work session does not repeat the complete architecture inventory merely because a tooling or polling session ended.

## Verified starting point

At kickoff, live GitHub `main` was verified at:

```text
3fb91f9aa1d02799dd5ff85560d96a2ddbc64389
```

That commit is PR #303, `Docs: define federation, output client and packaging roadmap`.

Working branch:

```text
work/phase68-legacy-osd-observation
```

The branch was verified identical to the above `main` checkpoint before Phase-68 changes.

Phase 67 Broadcast Companion Services is completed. Teletext and HbbTV remain their own domain-first capabilities and are not folded into Legacy OSD.

## Binding architecture

Phase 68 reuses, rather than replaces:

- ADR-0030 domain-first UI over OSD proxy;
- ADR-0039 Control Plane / Backend Agent boundary;
- ADR-0040 backend generation, lease and health;
- ADR-0041 authentication and Agent trust;
- ADR-0042 safe mutation/revision/idempotency;
- ADR-0047 Legacy OSD Compatibility Bridge;
- ADR-0049 accountability/security events.

ADR-0047 historical references to Phase 67 are numbering drift only. Runtime ownership and safety rules remain unchanged.

## Repository inventory at kickoff

The following Phase-68 prerequisites already exist and remain their current owners:

- central actor/backend authorization and backend access policy;
- Backend Agent identity and `backendGeneration` fencing;
- Agent observation snapshot/producer-sequence/resync primitives;
- `SuiteBridgeObservation`, `SuiteBridgeObservationWorker` and `SuiteBridgeEmbeddedAgentRuntime`;
- typed local SuiteBridge transport and capability discovery;
- SuiteBridge VDR lifecycle and bounded `cStatus` observation;
- existing normalized RemoteAction and LiveOverlay domains;
- private RESTfulAPI transport below Suite-owned adapters.

68.A now implements the bounded semantic `OsdSurfaceRef` / full
`OsdFrame` foundation. The following ADR-0047 concepts remain later work:

- `LegacyOsdSession`;
- `OsdViewerBinding`;
- `OsdDelta`;
- `OsdControllerLease`;
- `OsdInputCommand`;
- `OsdInputResult`;
- `osd.view` / `osd.control` authorization enforcement for the Legacy OSD plane.

Existing capability catalogue/UI references to the names `osd.view` and `osd.control` are not evidence that the runtime exists.

## Native/provider source audit - first result

### osd2web is not the Phase-68 capture owner

Current upstream `vdr-plugin-osd2web` was inspected as a source reference.

It creates its own VDR `cSkin` and `cSkinDisplay*` implementations. For interactive VDR menus it attaches itself to the VDR skin interface; upstream documentation notes that this can replace/remove the normal TV OSD while attached.

Therefore Phase 68 must not use direct osd2web exposure or skin takeover as its normal observation architecture.

osd2web remains useful source knowledge only.

### VDR core boundary

The normal VDR plugin OSD API allows a plugin to create an OSD through `cOsdProvider::NewOsd()`. That does not by itself provide a portable public plugin API for transparently copying the already-rendered arbitrary current skin surface.

Phase 68 must therefore not invent a raw-current-OSD pointer/capture API.

### Accepted 68.A native observation owner: SuiteBridge cStatus

The production reference is VDR 2.7.9. Its current cStatus contract exposes observation callbacks for OSD clear/title/status/help/menu items/current item/text/channel/programme state.

Phase 68.A therefore extends the existing SuiteBridge status monitor as the local native observation owner. Callback values are copied immediately into fixed-size value storage; VDR pointers are never retained. Writers use a non-blocking try-lock and mark continuity incomplete if an update cannot be captured. Oversized strings/items are bounded and likewise mark the frame incomplete.

The local OSD epoch is generated independently from backend generation. Full-frame sequence and observation time are local observation metadata. No input path is added.

RESTfulAPI /osd.json remains useful comparison/fallback evidence, but is not the primary 68.A owner: the existing generic IHttpClient cannot enforce the OSD body bound before reading the response, and RESTfulAPI's event stream does not currently provide OSD sequence continuity.


## Current SuiteBridge truth

Current code, not the older plugin roadmap baseline, is authoritative:

```text
SuiteBridge kickoff baseline:   0.13.3
SuiteBridge Phase-68.A accepted: 0.13.4
SuiteBridge Phase-68.B accepted:  0.13.5
capability schema:                1
```

68.B deliberately advertises `osd.view=available` only after the bounded
semantic local full-frame source exists. `osd.control=disabled` remains
explicit. Capability must never be inferred from plugin presence.

## Accepted 68.A scope

The first coherent vertical remains read-only:

1. define the distinct `OsdSurfaceRef` and immutable full `OsdFrame` value contracts;
2. keep `backendGeneration` and `osdEpoch` as separate fences;
3. define bounded rendering/scene payload limits and truthful inactive/degraded/suppressed states;
4. define a backend-local read-only OSD observation source boundary;
5. prove exact full-frame sequence/freshness semantics;
6. add focused domain/provider tests and architecture guards;
7. stop before authenticated Agent/client delivery and before every input path.

The accepted 68.A slice does not implement:

- controller leases;
- native key input;
- `osd.control`;
- raw key codes;
- raw SVDRP;
- shell/process execution;
- generic plugin-service tunneling;
- public RESTfulAPI/osd2web endpoints;
- Teletext or HbbTV replacement;
- MediaSession or playback ownership.

## 68.B scope

68.B is the local continuity slice from SB.14 stage 5:

1. expose one private typed `OSDSNAP` read command from SuiteBridge;
2. serialize only the bounded semantic full frame, never skin/native OSD pixels;
3. advertise `osd.view=available` and keep `osd.control=disabled`;
4. parse the local payload into the Suite-owned `OsdFrame` domain;
5. maintain one Agent-local latest-full-frame buffer;
6. treat source inconsistency, dropped native updates, same-sequence divergence
   and sequence regression as explicit resynchronization requirements;
7. accept forward sequence gaps because every accepted observation is a full frame;
8. clear a sticky resync only at a trustworthy new OSD epoch/backend generation
   or an inactive-to-new-surface transition;
9. keep the source capability-gated and private.

68.B still does **not** add authenticated remote Agent delivery, Control Plane
authorization, HTTP/REST/WebSocket exposure, browser rendering, controller
leases, key input, native `cRemote` calls or output-plugin rendering. Those
remain later Phase-68 slices. In particular, 68.C is the next transport slice,
not an input slice.

## Safety rules retained

- OSD payloads are bounded and transient.
- Complete frames are never written to normal logs or accountability events.
- Native VDR pointers/locks never cross the adapter boundary.
- VDR callbacks perform only bounded non-blocking work.
- local physical remote activity remains authoritative native activity.
- later sequence loss causes explicit resynchronization; state is never guessed.
- later input work cannot begin until view-only continuity, authorization and controller fencing are proven.

## Resume rule

A later work session should:

1. verify the live branch head and compare it with live `main`;
2. read this checkpoint and any later Phase-68 development notes;
3. continue the active 68.C transport slice from the verified main and this checkpoint;
4. repeat the full ADR/repository inventory only if repository changes invalidate this checkpoint.

Do not restart Phase 68 merely because an execution/polling session ended.


## 68.C transport checkpoint

Verified base: `81b9a28759debf5c87fe72afa793bc3db0a486fa` (PR #308).
Working branch: `work/phase68c-authenticated-osd-transport`.
Isolated checkout: `/home/yavdr/vdr-suite-phase68c`.

This is SB.14 stage 6: authenticated Agent transport of the semantic latest
full-frame state. It is not completion of ADR-0047, public view sessions,
viewer bindings, browser rendering or multi-site production acceptance.

### Existing owners and exact integration

- `SuiteBridgeOsdFrameSource` / `SuiteBridgeOsdFrameBuffer` retain local
  capability checks, parsing, fingerprint, epoch and sticky resync ownership.
- `apps/agent/main.cpp` composes the existing typed local transport, discovery
  and source. A new accepted backend generation creates a new local source.
- `BackendAgentClientRuntime` publishes once after an accepted heartbeat through
  `IBackendAgentControlPlaneTransport::postAuthenticated`. The existing protected
  Curl transport and credential lifecycle remain authoritative.
- `BackendAgentHttpServer` admits the new typed
  `/api/agent/v1/observations/osd` POST only after normal Agent authentication.
- `BackendAgentLifecycleService` reuses enrolled identity, current credential,
  device, backend, instance, generation, heartbeat, capability and lease owners.
  Its OSD reception cache is transient and bounded; it does not call the durable
  observation repository with frame contents.
- The internal `readOsdObservation` boundary requires an authenticated non-Agent
  context, explicit backend-scoped `osd.view`, current backend generation and
  live authority. `AuthorizationService` remains the permission evaluator.
  No new public client route is exposed.

`OsdFrame` and `OsdSurfaceRef` remain the semantic value contracts. Agent wire
schema `osdObservationSchema=1` is distinct from plugin `osd_schema=1`.
Wire schema 1 has a canonical strict field order, rejects extra/duplicate fields,
and transmits normalized values rather than forwarding a provider JSON body.
Source-state codes are fixed at NotRead=0, Current=1, ResyncRequired=2,
CapabilityUnavailable=3, TransportDegraded=4, ReplyRejected=5, InvalidPayload=6.
Surface-state codes are Inactive=0, Active=1, Degraded=2, Suppressed=3;
kind codes are None=0, Menu=1, ChannelInfo=2. Incompatible meanings require a
new wire schema. The fingerprint is diagnostic, not authentication evidence.

### Bounds, continuity and privacy

- Optional `OBSERVATION_DOMAINS=osd` uses the existing configured loopback
  `SUITEBRIDGE_HOST` / `SUITEBRIDGE_PORT`; configuration requires a heartbeat
  interval of at most 30 seconds. Default behavior remains unchanged without
  the explicit OSD domain.
- OSD-only configuration with empty `COMMAND_TYPES` does not auto-activate
  native Recording commands merely because the local endpoint is configured.
- One complete observation per accepted heartbeat; no extra worker or listener.
- `producerSequence` is the accepted heartbeat sequence, independent of native
  `frameSequence`, `osdEpoch` and `backendGeneration`.
- Forward producer gaps are safe because every message is complete. Old
  producer sequences and same-sequence divergence are rejected. A duplicate
  does not refresh cache age. Source resync state is carried explicitly.
- At most 64 backend entries; at most 512 KiB encoded body, with the existing
  tighter semantic string/item limits. Entries expire after 60 seconds; reads
  also recheck live lease, capability, credential and generation.
- Source failure publishes no cached frame content as current. Transport
  failure waits for the next newly observed full state; no durable queue or
  old-frame retry loop exists. OSD failure does not tear down health/commands.
- No OSD payloads in database, audit, logs or local identity files. Responses
  use the existing `Cache-Control: no-store` behavior.

### Validation and live boundary

`test-phase68-osd-authenticated-transport` proves the actual 68.B source/buffer
through Agent runtime, authenticated HTTP admission and the internal authorized
reader. Cases cover rejected identities/scopes/generations, malformed/oversized
payloads, UTF-8, replay/conflict, forward full-frame gaps, sticky resync, epoch
replacement, new Agent generation, expiry, revocation, lost acknowledgement,
unavailable source and absence of durable frame contents.

The executable's explicit `--live` mode reads real loopback SuiteBridge on port
6419 and delivers that frame through a temporary enrolled Agent and the same
authenticated receiver. Agent HTTP dispatch is in-process in this harness;
this is not a claim of live TLS or two-site network acceptance. Its temporary
database and identity are separate from production. No native input, daemon
replacement, installation or VDR restart is performed. Rollback is removal of
the optional OSD configuration or use of the previous Agent binary; transient
receiver data becomes unavailable at expiry. Production was not redeployed.

Required focused checks: the new transport target, existing
`test-phase68-osd-agent-local-resync`, `test-backend-agent-foundation`,
`test-backend-agent-client`, `backend-agent`, `test-architecture`, documentation
and Make-inventory validation. New Phase-68 group membership lives only in
`mk/test-groups.mk`.

The unchanged verified base already fails the strict Make inventory with 38
ungrouped targets, four orphan test sources, three required-reachability errors,
and noncanonical group definitions in older Make fragments. These were
reproduced from an archive of the exact base commit. The Phase-68 grouping is
corrected in this slice; unrelated Home/Recording/packaging inventory repairs
are not silently folded into the OSD change. Full-repository green status must
not be claimed from the focused acceptance results.

SuiteBridge stays `0.13.5`, local schemas remain unchanged, and `osd.control`
stays disabled. Controller leases, native inputs, raw tunnels, public provider
exposure, renderer/output clients, Teletext/HbbTV changes and MediaSession work
remain outside 68.C.
