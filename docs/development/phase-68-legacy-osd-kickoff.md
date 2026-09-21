# Phase 68 - Legacy OSD Compatibility Bridge Kickoff

## Status

Phase 68 is active.

Active coherent vertical:

```text
68.A - Read-only OSD observation
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

The following ADR-0047 runtime concepts are not yet implemented as the Legacy OSD runtime:

- `OsdSurfaceRef`;
- `LegacyOsdSession`;
- `OsdViewerBinding`;
- `OsdFrame`;
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
SuiteBridge plugin version: 0.13.3
capability schema:          1
```

The current static capability catalogue does not yet advertise an OSD observation/input capability.

Any Phase-68 SuiteBridge capability must therefore be added deliberately and tested; it must not be inferred from plugin presence.

## 68.A scope

The first coherent vertical remains read-only:

1. define the distinct `OsdSurfaceRef` and immutable full `OsdFrame` value contracts;
2. keep `backendGeneration` and `osdEpoch` as separate fences;
3. define bounded rendering/scene payload limits and truthful inactive/degraded/suppressed states;
4. define a backend-local read-only OSD observation source boundary;
5. prove exact full-frame sequence/freshness semantics;
6. add focused domain/provider tests and architecture guards;
7. only then wire the accepted source toward Agent transport in 68.C.

68.A does not implement:

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
3. continue the active 68.A slice from the latest accepted branch evidence;
4. repeat the full ADR/repository inventory only if repository changes invalidate this checkpoint.

Do not restart Phase 68 merely because an execution/polling session ended.
