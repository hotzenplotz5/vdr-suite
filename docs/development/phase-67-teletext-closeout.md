# Phase 67 — Teletext Vertical Closeout

Status: **COMPLETED / MERGED THROUGH PR #293.**

Merge checkpoint:

```text
branch=work/phase67-teletext-read-path
accepted_head=c72f88a6a7cb3d86fbd59c16195e99cb6e5791fe
pull_request=293
main_merge=d92e7907637122368908ce4c7564a0332db9c487
```

This document is the durable implementation and acceptance record for the
Teletext vertical inside Phase 67 — Broadcast Companion Services.

Phase 67 itself remains active because HbbTV discovery/session/runtime work is
still open. This closeout does not mark the whole numbered phase complete.

## Delivered architecture

The accepted Teletext path is domain-first and does not use Legacy OSD as the
primary contract:

```text
real broadcast Teletext
  -> osdteletext provider
  -> SuiteBridge private Teletext service/page commands
  -> typed existing Phase-63 Agent transport
  -> Suite Teletext resolver/domain
  -> fenced Control Plane read service
  -> authorized HTTP API
  -> first-party browser/TV Teletext view
```

The public/browser contract exposes normalized service/page data only. Provider
cache filenames, raw VDR pointers, osdteletext internals, private SVDRP
commands and local paths remain private.

## Provider and transport

Real yaVDR acceptance proved the provider path with real German Teletext
content.

The SuiteBridge integration provides private Teletext capability/page access
without adding those commands to public SVDRP help.

The typed transport uses the existing Phase-63 Agent path. It does not create
a second Control Plane or a parallel backend transport architecture.

## Normalized Teletext domain

The implemented domain models:

- backend identity and backend generation;
- channel/service identity;
- provider identity, capability revision and provider generation;
- availability, receiver activity, source and freshness;
- page number and subpage;
- normalized 25 x 40 page geometry;
- normalized cells with codepoint/raw character, charset, foreground,
  background, kind and flags;
- completeness/degradation evidence.

## Backend lifecycle and fencing

Real acceptance exposed an important lifecycle gap: the local embedded
SuiteBridge runtime was not represented by the standalone Agent lease.

The accepted fix introduced a shared monotonic backend runtime generation
allocator and a dedicated embedded backend lifecycle authority.

Properties retained:

- embedded and standalone runtimes share one monotonically increasing backend
  generation space;
- stale generations fail closed;
- embedded lifecycle becomes authoritative only with current SuiteBridge
  health/freshness;
- a valid standalone Agent lease blocks embedded takeover;
- a newer runtime generation invalidates stale embedded authority;
- the standalone backend-agent service may remain disabled for the local
  SuiteBridge deployment.

Real acceptance established an embedded generation greater than the historical
standalone generation and a valid online lease before Teletext reads were
accepted.

## Authorized HTTP read surface

The first-party Suite API exposes:

```text
GET /api/vdr/broadcast/teletext/service
GET /api/vdr/broadcast/teletext/page
```

The permission is:

```text
broadcast.teletext.view
```

The routes are backend-scoped, audited through the existing authorization
boundary, GET/read-only, and return `Cache-Control: no-store`.

Provider implementation details are not returned to clients.

## First-party frontend

The first-party frontend includes:

- a dedicated Teletext owner;
- page 100 as the initial page;
- direct page entry for 100..899;
- previous/next page navigation;
- automatic and explicit subpage navigation;
- normalized 40 x 25 rendering from the Suite cell model;
- desktop Teletext + existing Live-TV companion composition;
- no second playback owner or second MediaSession;
- direct entry from active Live-TV;
- direct entry from Media Home next to the EPG action.

While Teletext is open, the underlying channel grid is interaction-isolated;
the existing Live-TV companion remains the only interactive playback owner.

## Real yaVDR / browser acceptance

The accepted real-system evidence includes:

```text
RESULT=PHASE67_EMBEDDED_LIFECYCLE_AUTHORITY_PASS
BACKEND_LIFECYCLE=EMBEDDED_ONLINE
TELETEXT_SERVICE_API=PASS
TELETEXT_PAGE_100=PASS
NORMALIZED_25X40=PASS
CELLS_1000=PASS
RESULT_DETAIL=PHASE67_REAL_TELETEXT_HTTP_E2E_PASS
```

Observed page evidence included live ARD/ZDF Teletext with real changing
broadcast content.

The accepted browser journey additionally proved:

- open Teletext from Live-TV;
- navigate pages;
- close Teletext while Live-TV remains usable;
- desktop companion presentation with the existing Live player;
- direct Media Home Teletext launch for the focused channel.

## Golden Journey 8

Journey 8 — Teletext while watching Live TV — is accepted for the supported
desktop/browser deployment profile.

The accepted behavior is:

```text
Live TV
  -> Teletext available
  -> open Teletext
  -> page/subpage navigation
  -> close
  -> Live remains usable
```

The Media Home direct-entry path is an additional accepted first-party entry
point and does not replace the Live-TV journey.

## Retained boundaries

This Teletext closeout does not authorize or implement:

- HbbTV application discovery/session/runtime;
- Legacy OSD compatibility;
- arbitrary raw plugin/SVDRP/browser commands;
- public raw provider topology;
- a second Live playback owner;
- Phase-69 public API compatibility hardening.

## Next Phase-67 work

Phase 67 remains active.

The next coherent vertical is HbbTV discovery:

```text
real broadcast application signaling
  -> backend-local discovery provider
  -> normalized BroadcastApplicationDescriptor
  -> Suite read model
  -> user-visible HbbTV availability
```

HbbTV application execution/session work remains a later Phase-67 vertical
after discovery is established.

## Related documents

- [Current State](../CURRENT.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)
- [ADR-0054 Broadcast Companion Services](../adr/ADR-0054-broadcast-companion-teletext-hbbtv.md)
