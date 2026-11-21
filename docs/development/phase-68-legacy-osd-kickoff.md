# Phase 68 - Legacy OSD Compatibility Bridge Kickoff

## Status

Phase 68 is completed for the accepted 68.A-G scope.

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

Accepted authenticated transport vertical:

```text
68.C - Authenticated read-only Agent OSD transport
PR #309 -> ed2f451d824e43c4264dee6caf8ae49dbeb71a88
```

Accepted view-session authorization vertical:

```text
68.D - Authorized bounded Legacy OSD view sessions
PR #310 accepted candidate -> 8807d587536daa6c27ae72a3f88d40b0c7de3480
Hosted CI 35594914471: PASS
real yaVDR acceptance: PASS
```

Accepted viewer-binding vertical:

```text
68.E - Bounded viewer bindings and multi-viewer delivery
PR #311 accepted runtime candidate -> 2b74b26ed33ce1a555a9cc037d44ad6768480eb2
Hosted CI 35601863603: PASS (6/6)
real yaVDR acceptance: RESULT=PHASE68E_REAL_ACCEPTANCE_PASS
```

Accepted controller-lease vertical:

```text
68.F - Exclusive controller lease and osd.control fencing
PR #312 -> e3f9215f5f80dd230e1e855e3ce09f2ac70231ef
Hosted CI 35610473828: SUCCESS
```

Accepted allowlisted-input vertical:

```text
68.G - Allowlisted native OSD input
PR #313 runtime candidate -> 7ae51d090cbe06570b8a70e787137232df83f124
real yaVDR acceptance: RESULT=PHASE68G_REAL_NATIVE_OSD_INPUT_PASS
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

68.A through 68.G establish the bounded semantic observation, local continuity,
authenticated Agent transport, explicit view-session authorization, bounded
multi-viewer delivery, exclusive controller authority and allowlisted native
input required by ADR-0047.

`osd.view` and `osd.control` remain independent backend-scoped permissions and
are not implied by `role.admin`. The view session intentionally remains
`view_only`; controller authority is acquired separately through the fenced
`OsdControllerLease`. SuiteBridge 0.14.0 advertises
`osd.view=available` and `osd.control=available`, while generic
`mutations` remains disabled.

Optional fidelity work such as deltas is not required to close Phase 68.
Renderer/output-client work, raw command tunnels and Phase-69 public API
stabilization remain separate scopes.

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
3. continue with 68.F exclusive controller leasing from the verified main and the accepted 68.E viewer-binding boundary;
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


## 68.D authorized view-session closeout

Accepted candidate:

```text
PR #310
8807d587536daa6c27ae72a3f88d40b0c7de3480
Hosted CI run 35594914471: SUCCESS
real yaVDR acceptance: RESULT=PHASE68D_REAL_ACCEPTANCE_PASS
```

68.D is SB.14 stage 7: authenticated Control Plane admission and refresh of a
bounded, view-only `LegacyOsdSession`. It consumes the single existing
`BackendAgentLifecycleService` and the transient 68.C OSD receiver rather than
creating a second OSD cache or lifecycle owner.

The accepted boundary provides:

- explicit backend-scoped `osd.view` enforcement at HTTP admission and again
  when a session is refreshed;
- no implicit `osd.view` grant from `role.admin`;
- browser CSRF protection for session creation while keeping view-session
  creation non-mutating;
- bounded transient session count and lifetime;
- actor/client/backend binding;
- backend-generation fencing and current Agent authority/freshness rechecks;
- view-only session metadata with `Cache-Control: no-store`;
- no OSD frame/menu text returned through the session admission/status API;
- fail-closed revocation, expiry and generation-change handling.

Hosted CI passed architecture, documentation, Make inventory, frontend,
packaging and fast regression jobs. The fast suite includes the 68.D
view-session/security tests, the accepted 68.C authenticated OSD transport
regression and a production daemon build.

Real yaVDR acceptance additionally proved:

```text
DAEMON_BUILD=PASS
RESULT=PHASE68C_REAL_SOURCE_AUTHENTICATED_RECEIVER_PASS
REAL_OSD_SOURCE_AUTHENTICATED_RECEIVER=PASS
NO_CONTROL_INPUT_BOUNDARY=PASS
WORKTREE_CLEAN_AFTER=PASS
RESULT=PHASE68D_REAL_ACCEPTANCE_PASS
```

No production binary was installed or replaced and neither VDR nor the
production daemon was restarted for this acceptance.

68.D does **not** implement `OsdViewerBinding`, multi-viewer fan-out,
`OsdControllerLease`, `osd.control`, native input, raw VDR key codes,
`cRemote`, generic SVDRP/plugin-service tunneling, renderer/browser overlay,
output-plugin rendering, SkinDesigner/native pixel capture, Teletext/HbbTV
refactoring, MediaSession changes or Phase 69.

68.E is accepted as the bounded viewer-binding and multi-viewer delivery
slice. Controller leasing is now the next coherent Phase-68 slice, 68.F, and
must remain separate from native input. Allowlisted input follows as 68.G only
after the controller lease boundary is accepted.


## 68.E bounded viewer-binding closeout

Accepted runtime candidate:

```text
PR #311
2b74b26ed33ce1a555a9cc037d44ad6768480eb2
Hosted CI run 35601863603: SUCCESS (6/6)
real yaVDR acceptance: RESULT=PHASE68E_REAL_ACCEPTANCE_PASS
```

68.E adds transient bounded `OsdViewerBinding` ownership above the accepted
68.D `LegacyOsdSession` boundary. It supports independent read-only viewers
with exact actor/client/session/backend/generation/surface/epoch association,
per-viewer sequence acknowledgement, bounded backpressure and explicit full
resynchronization. The binding service reuses the existing transient 68.C OSD
observation; it does not create a second frame cache or durable payload store.

The public/API layer owns binding attach/detach lifecycle metadata only.
Sequenced OSD frame delivery remains on the separately defined Legacy OSD
compatibility plane; 68.E does not invent a public REST frame route,
WebSocket/SSE transport or renderer.

The accepted implementation remains view-only. It does not add
`OsdControllerLease`, `osd.control`, native key input, raw key codes,
`cRemote`, generic SVDRP/plugin-service tunneling, shell execution, direct
RESTfulAPI/osd2web exposure, Teletext/HbbTV changes, MediaSession changes or
Phase 69 work.

Real yaVDR acceptance additionally proved the production daemon build, the real
SuiteBridge/VDR source path through the existing VDR SVDRP endpoint, retention
of the no-control/no-input boundary, a clean isolated worktree and the full
Hosted-CI evidence. No production binary replacement or VDR/daemon restart was
required.

The next coherent slice is 68.F controller leasing. 68.G allowlisted native
input remains later and is not authorized by this closeout.


## 68.G final closeout checkpoint

Accepted real-runtime candidate:

```text
7ae51d090cbe06570b8a70e787137232df83f124
```

The real yaVDR/VDR 2.7.9 acceptance used the production daemon and the manually
started, deliberately disabled Backend Agent service state. No additional build,
installation or restart was performed for the final input acceptance.

The acceptance proved:

```text
OSD_OBSERVATION_SYNCHRONIZED=PASS
NATIVE_OSD_READY=PASS
VIEW_SESSION_BOUNDARY=PASS
LEGACY_OSD_SESSION_EPOCH_CURRENT=PASS
VIEWER_ATTACH=PASS
OSD_CONTROL_AUTHORITY=PASS
CONTROLLER_LEASE=PASS
DOWN_INPUT_ACCEPTED=PASS
DOWN_IDEMPOTENT_REPLAY=PASS
DOWN_NATIVE_DISPATCH=PASS
DOWN_OSD_SELECTION_CHANGED=PASS
UP_INPUT_ACCEPTED=PASS
UP_NATIVE_DISPATCH=PASS
UP_OSD_SELECTION_RESTORED=PASS
DISPATCH_FENCE_AUDIT=PASS
NATIVE_RESULT_AUDIT=PASS
AUDIT_NO_RAW_KEY_PAYLOAD=PASS
CONTROLLER_RELEASE=PASS
STALE_AUTHORITY_REJECTED=PASS
STALE_AUTHORITY_NO_ASSIGNMENT=PASS
STALE_AUTHORITY_NO_NATIVE_DISPATCH=PASS
VIEWER_DETACH=PASS
OSD_PERMISSION_STATE_RESTORED=PASS
BROWSER_SESSION_LOGOUT=PASS
SQLITE_QUICK_CHECK=PASS
VDR_NOT_RESTARTED=PASS
DAEMON_NOT_RESTARTED=PASS
AGENT_NOT_RESTARTED=PASS
AGENT_ENABLEMENT_UNCHANGED=PASS
RESULT=PHASE68G_REAL_NATIVE_OSD_INPUT_PASS
```

The native semantic effect was observed directly through SuiteBridge OSDSNAP:
DOWN changed the selected menu index from 0 to 1 and UP restored it from 1 to
0. An identical replay reused the same Agent command without a second
assignment. After controller release, stale authority returned conflict without
creating an assignment or causing native dispatch.

The final acceptance also confirmed an important ownership detail: the
`LegacyOsdSession` remains `view_only`; `osd.control` authority belongs to
the separate controller lease. Because authenticated OSD observations are
published on accepted Agent heartbeats, the acceptance synchronized one fresh
heartbeat after opening the native menu before creating the session.

PR #313 is the Phase-68.G/Phase-68-closeout integration PR. Its final hosted CI
is a merge gate; the accepted real-runtime evidence above belongs to the exact
runtime candidate and is not invalidated by documentation-only closeout changes.
