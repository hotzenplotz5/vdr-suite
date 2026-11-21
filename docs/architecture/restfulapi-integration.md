# RESTfulAPI Integration Architecture

## Status and purpose

This document describes the current role of `vdr-plugin-restfulapi` in VDR-Suite. RESTfulAPI is a **private backend adapter/provider**, not the public browser API and not the owner of Suite domain models.

The former endpoint inventory is retained as [historical evidence](history/restfulapi-integration-before-refresh.md).

## Core boundary

```text
VDR-Suite service/domain runtime
  -> backend-neutral adapter or executor/provider interface
  -> IHttpClient / BasicHttpClient
  -> vdr-plugin-restfulapi
  -> VDR Core / installed plugins
```

Higher layers receive normalized Suite/VDR domain values, never RESTfulAPI JSON or private route details. Browser code communicates only through Suite routes and `VdrSuiteClientApi`.

RESTfulAPI remains one possible local/backend implementation alongside SVDRP, SuiteBridge and future Agent-local providers. VDR-Suite may choose a different private source per capability.

## Implemented infrastructure

Current main contains:

- `IHttpClient`, `HttpRequest` and `HttpResponse` abstractions;
- `BasicHttpClient` production transport and mock/test transports;
- RESTfulAPI-backed status, channel, EPG, Recording and Timer mappings;
- SearchTimer discovery, preview/validation/workflow and conflict integrations;
- Recording action executors with Suite validation/policy/readback boundaries;
- backend-neutral RemoteAction and LiveOverlay executors/providers;
- Suite-owned cache/persistence layers that avoid repeated passthrough reads;
- server-side backend access-mode/capability enforcement.

Transport timeout/error handling is therefore not wholly future work. Broader retry, Agent lifecycle and durable operation semantics remain later cross-cutting work.

## Current use by domain

| Domain | Current RESTfulAPI role | Suite-owned boundary |
| --- | --- | --- |
| Status / overview | private status/current-channel/device/plugin source | normalized status/snapshot services |
| Channels | private channel list/movement source | backend-scoped channel domain and Client API |
| EPG | authoritative native event source where configured | persistent Suite EPG cache, search/details/Genres |
| Recordings | native Recording list/action source where configured | lazy cache, Recordings 2, metadata and guarded actions |
| Timers | native Timer read/mutation source where configured | Timer services, validation/readback and Client API |
| SearchTimer / epgsearch | private command/catalog/conflict source | backend-neutral SearchTimer domain/workflow |
| Remote control | private normalized-action executor | Suite allowlist, permission/capability gate and operation ID |
| Live overlay | private current-live-state input | Suite overlay read model using snapshots and persistent EPG |
| OSD | audited possible structured source only | no current LegacyOsdSession runtime; Phase 66 |
| Streaming | possible private provider input | no public provider URL; Phase 65 Gateway |
| Artwork/scraper data | optional private evidence/delivery source | Suite persistence, provider-neutral refs and authenticated routes |

## Read-path policy

RESTfulAPI passthrough is not the default architecture for all browser reads.

Current persistent paths include:

```text
RESTfulAPI/native refresh
  -> bounded adapter mapping
  -> Suite-owned backend-scoped cache/repository
  -> service/controller
  -> VdrSuiteClientApi
  -> frontend
```

Phase 61 Genre GETs and PR #111 Global Search GETs use query-only Suite SQLite connections and perform no RESTfulAPI, TVScraper, SuiteBridge or other provider lookup during the request.

## Mutation policy

RESTfulAPI may execute a private native mutation only after Suite-owned gates have run. Current bounded examples include Recording actions, Timer/SearchTimer paths and RemoteAction.

```text
Suite request
  -> validation and backend scope
  -> read-only and capability policy
  -> normalized operation/action
  -> private RESTfulAPI executor
  -> normalized result
  -> authoritative readback where supported
```

RESTfulAPI success alone is not the future universal durable-operation contract. Phase 62/63 must add actor authorization, accountability, revision/idempotency, dispatch evidence, generation fencing and reconciliation before new remote privileged operations.

## Remote and overlay mapping

The public Suite routes are:

```text
POST /api/vdr/remote/actions
GET  /api/vdr/live/overlay
GET  /api/vdr/live          (SSE change notifications)
```

The private executor maps fixed Suite action names to fixed RESTfulAPI remote operations. Raw key names, arbitrary sequences, unchecked path fragments, SVDRP commands and shell commands are not accepted from the browser.

PR #110 changed frontend interaction state only; the private adapter boundary remains unchanged.

## EPG and metadata interaction

RESTfulAPI supplies native EPG observations where configured. Suite-owned workers persist backend/channel/event-scoped cache rows and authoritative bounded retirement.

TVScraper/SuiteBridge may add asynchronous metadata/person/Genre evidence. RESTfulAPI is not used as a hidden provider resolver during normal Genre or Global Search GET requests.

Backend-native event identity remains backend scoped. Similar titles/times are evidence and do not silently create a canonical cross-provider event identity.

## Recordings 2 interaction

RESTfulAPI may supply native Recording lists and execute native actions. The delivered browser owner is Recordings 2, backed by Suite caches/services and metadata read models.

The browser does not render RESTfulAPI response shapes. Rename/move/trash requests use Suite validation, safety, policy and readback boundaries rather than direct plugin URLs.

## SearchTimer and epgsearch interaction

RESTfulAPI-backed paths provide native SearchTimer commands and helper catalogues such as channel groups, directories, blacklists, extended EPG information and conflicts where available.

These remain adapter concerns. The Suite domain owns request/response normalization, preview, validation and controlled execution. Exact remaining epgsearch edge semantics are tracked in the parity document. Future cross-backend TimerIntent orchestration belongs to Phase 64.

## OSD and streaming boundaries

RESTfulAPI may expose structured current OSD state and private media/provider endpoints, but those are not sufficient public Suite contracts.

- Streaming requires authenticated `MediaSession`, short-lived grants, route epoch and Gateway ownership in Phase 65.
- Legacy OSD requires sessions, ordered frames/deltas, resynchronization, viewer policy and one fenced controller lease in Phase 66.
- Current RemoteAction/LiveOverlay does not imply either runtime is complete.

## Capability and degradation rules

A configured RESTfulAPI endpoint does not automatically make a Suite capability available. Availability depends on:

- known/enabled backend;
- adapter/executor registration;
- compatible plugin endpoint/version behaviour;
- server-side backend access mode;
- current health and required capability evidence;
- domain preconditions.

Unknown, incompatible or unavailable private features degrade explicitly. Frontend visibility/disabled state is informational and never the authoritative permission gate.

## Security and privacy rules

- credentials and private backend URLs remain server-side;
- browser input does not become unchecked private paths;
- provider/plugin response fields are validated and normalized;
- private adapters are not user authorization boundaries;
- logs must not expose credentials or unbounded provider payloads;
- public API evolution does not reveal adapter versions as domain identity.

## Future work

RESTfulAPI-related open work is bounded to explicit gaps, not a general “integrate RESTfulAPI” phase:

- capability/version degradation hardening through Phase 63/67;
- remaining exact Timer/SearchTimer semantics where product value justifies them;
- specialist diagnostics such as femon/wirbelscan only after explicit product decisions;
- private media/OSD provider use behind Phase 65/66 Suite contracts;
- migration of transition routes to stable `/api/v1` in Phase 67.

## Related documents

- [Current Architecture State](../development/current-architecture-state.md)
- [Live Remote, Overlay and Legacy OSD Contract](live-remote-osd-contract.md)
- [Metadata-Backed Genre Browser](metadata-genre-browser.md)
- [Backend-Scoped Global Search](global-search.md)
- [VDR Ecosystem Parity](../planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture Gap Matrix](../planning/architecture-audit-gap-matrix.md)