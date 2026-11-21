# Phase 68.E - Bounded Legacy OSD Viewer Bindings Closeout

## Status

Phase 68.E is accepted.

```text
PR: #311
Accepted runtime candidate: 2b74b26ed33ce1a555a9cc037d44ad6768480eb2
Hosted CI: 35601863603 SUCCESS (6/6)
Real yaVDR acceptance: RESULT=PHASE68E_REAL_ACCEPTANCE_PASS
```

Phase 68 remains active. The next coherent slice is **68.F - exclusive
controller lease and `osd.control` fencing**. Native input remains a separate
later slice, 68.G.

## Scope completed

68.E implements the bounded viewer plane on top of the accepted 68.D
`LegacyOsdSession` boundary:

```text
SuiteBridge semantic OsdFrame
  -> authenticated Agent OSD transport
  -> transient Control Plane OSD receiver
  -> authorized LegacyOsdSession
  -> bounded OsdViewerBinding fan-out
```

The implementation reuses the existing `BackendAgentLifecycleService`,
`LegacyOsdSessionService`, transient OSD observation and API/security owners.
It does not introduce a second Agent lifecycle, a second frame cache or durable
OSD payload storage.

The accepted viewer binding carries the ADR-0047 identity/fencing state needed
for independent readers:

- opaque `viewerBindingId` and binding revision;
- actor/client/session identity and current session revision;
- backend identity and `backendGeneration`;
- OSD surface identity and `osdEpoch`;
- acknowledged/delivered frame cursor and acknowledged event cursor;
- attach/last-seen/expiry timestamps;
- rendering profile, state and close/resync reason;
- explicit `controlAuthorized=false`.

Bindings are transient and bounded to 64 process-wide and 8 per
`LegacyOsdSession`. A binding never stores an OSD frame payload. Its hard
expiry is bounded by the parent view-session expiry.

## Delivery, backpressure and resynchronization

68.E consumes the authoritative latest full-frame observation already returned
through the 68.D session refresh. It does not duplicate that observation in a
viewer-owned cache.

The accepted delivery semantics are:

- initial attachment requires a trustworthy full frame;
- each viewer has an independent acknowledgement/delivery cursor;
- one slow viewer does not block the producer or other viewers;
- an outstanding frame that is overtaken by a newer authoritative frame causes
  only that viewer to enter `resync_required`;
- acknowledgement conflicts and non-contiguous sequence advancement are
  explicit resynchronization boundaries;
- source-side resync propagates as viewer resync without inventing client state;
- OSD surface/epoch replacement rebinds the viewer to the new surface and
  resets cursors before a new full frame is delivered;
- backend-generation change fails closed and invalidates the stale binding;
- suspension/resume requires resynchronization before current rendering
  continues.

No unbounded replay queue is required. The guarantee is detection of lost
continuity and recovery to one authoritative complete current frame.

## Security and ownership boundary

Viewer attachment and subsequent delivery remain bound to the exact
actor/client/session/backend identity.

`LegacyOsdSessionService::status()` remains the authorization/freshness owner
and rechecks explicit backend-scoped `osd.view`, current backend generation,
Agent authority, expiry and transient OSD availability. `role.admin` still
does not imply `osd.view`.

The HTTP/API runtime exposes only viewer-binding lifecycle metadata:

- attach a viewer;
- detach a viewer;
- `Cache-Control: no-store`;
- view capability only, with control false.

Sequenced OSD frame delivery remains on the separately defined Legacy OSD
compatibility plane. 68.E deliberately does **not** add a public
`/api/vdr/legacy-osd/viewers/frame` route, WebSocket, SSE, long-poll transport
or first-party renderer.

## Accepted safety boundary

68.E remains strictly read-only.

It does not add:

- `OsdControllerLease`;
- `osd.control`;
- native VDR key input;
- raw key codes or `cRemote`;
- generic SVDRP or plugin-service tunnels;
- shell/process execution;
- direct public RESTfulAPI/osd2web exposure;
- SkinDesigner/native pixel capture;
- Web/TV OSD renderer ownership;
- output-plugin rendering;
- Teletext/HbbTV changes;
- Phase-65 MediaSession changes;
- Phase 69 work.

The accepted 68.C privacy rule also remains intact: frame/menu payloads stay
transient and do not enter durable observation storage, ordinary logs or
accountability events.

## Hosted CI evidence

Run `35601863603` completed successfully on the accepted runtime candidate.

```text
architecture-check=SUCCESS
docs-check=SUCCESS
make-test-audit=SUCCESS
frontend-regression-test=SUCCESS
packaging-regression-test=SUCCESS
fast-regression-test=SUCCESS
daemon build=PASS
```

The fast suite covers the 68.E viewer-binding tests plus the retained 68.D
session/security and 68.C authenticated OSD transport regressions.

CI caught one integration defect before acceptance: removing the prohibited
public frame-delivery serializers also removed the harmless viewer-binding
metadata serializer. Restoring only the metadata serializer fixed both Fast CI
and install-staging while preserving the no-public-frame-route boundary.

## Real yaVDR evidence

The isolated real-system acceptance completed:

```text
DAEMON_BUILD=PASS
RESULT=PHASE68C_REAL_SOURCE_AUTHENTICATED_RECEIVER_PASS
REAL_OSD_SOURCE_AUTHENTICATED_RECEIVER=PASS
NO_PHASE68F_68G_RUNTIME_SYMBOLS=PASS
WORKTREE_CLEAN_AFTER=PASS
HOSTED_CI_RUN=35601863603
HOSTED_CI_HEAD=2b74b26ed33ce1a555a9cc037d44ad6768480eb2
HOSTED_CI_6_OF_6=PASS
HOSTED_CI=PASS
RESULT=PHASE68E_REAL_ACCEPTANCE_PASS
```

The live source path uses the existing VDR SVDRP loopback endpoint and invokes
SuiteBridge through that VDR boundary. The acceptance installed or replaced no
production binary and required no VDR or production-daemon restart.

## Continuation boundary

68.F may add only the exclusive controller-lease plane on top of the accepted
view plane:

- separate explicit `osd.control` permission;
- exactly one active Suite controller for one native surface scope;
- controller lease identity/epoch, expiry, renewal and revocation;
- read-only backend denial;
- stale session/generation/surface/epoch fencing;
- no native key dispatch yet.

Allowlisted normalized native input is **68.G** and must not be folded into the
68.F lease slice.
