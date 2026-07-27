# Client Capability, API Candidate and Gap Matrix

## Status and scope

This document classifies current client capabilities and future public API candidates against repository main at:

```text
cb77ff66e11dca7db2eafa36525762dcde35102d
```

It is planning evidence. It does not declare current unversioned routes stable, does not publish `/api/v1` and does not replace the strict Phase 62–67 sequence.

## Legend

Current state:

- **Yes** — implemented in current main for the stated boundary.
- **Partial** — useful foundations exist but the complete client function does not.
- **No** — not implemented.
- **Internal** — current route/data is an implementation or compatibility boundary.
- **Later** — depends on a planned phase.

Client fit:

- **Primary** — expected first-class client function.
- **Useful** — sensible but not necessarily first-release scope.
- **Limited** — constrained or administrative use only.
- **No** — intentionally not exposed.

Production classification:

- **Read-ready** — useful current read foundation, still unversioned and unauthenticated as a public contract.
- **Transition** — browser-internal/compatibility behaviour.
- **Blocked** — must not be used by an independent production client yet.
- **Phase N** — first phase that owns the missing runtime contract.

## Client capability matrix

| Function | Current implementation | Browser | Client wrapper / route | Android Mobile | Android TV | TV browser | Desktop | Third party | Production classification |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| API root/capability discovery | Backend capability route exists; no `/api/v1` root | Partial | `fetchClientCapabilities`; `/api/vdr/capabilities` | Primary | Primary | Primary | Primary | Primary | Transition; Phase 67 |
| Backend selection | Stable Backend Registry and browser selector | Yes | `fetchClientBackends`; `/api/backends` | Primary | Primary | Useful | Primary | Primary | Read-ready, unversioned |
| Backend availability/health | Status, health, snapshot and registry state | Yes | status/health/snapshot wrappers and routes | Primary | Primary | Useful | Primary | Primary | Read-ready; authorization later |
| Multi-backend aggregate reads | Foundations exist; many views still selected-backend | Partial | Per-route conventions vary | Primary | Useful | Limited | Primary | Primary | Partial; Phase 67 normalization |
| Server-enforced read-only backend | Implemented policy foundation | Yes | Reflected in backend representations; action services enforce | Primary | Primary | Primary | Primary | Primary | Implemented foundation; actor policy Phase 62 |
| Channel list | Implemented | Yes | `fetchClientChannels`; `/api/vdr/channels` | Primary | Primary | Primary | Primary | Primary | Read-ready, unversioned |
| Current/next programme | Implemented through EPG/read models | Yes | now-next and cached window wrappers/routes | Primary | Primary | Primary | Primary | Primary | Read-ready, identity normalization needed |
| EPG timeline/window | Implemented browser timeline and cached windows | Yes | `fetchClientEpgWindow`, cache/time-window helpers | Primary | Useful | Primary | Primary | Primary | Read-ready; public ProgramEvent model Phase 67 |
| EPG detail | Implemented with metadata owner | Yes | EPG routes plus detail modules | Primary | Primary | Useful | Primary | Primary | Read-ready; stable event ID/provenance required |
| EPG search | Implemented | Yes | `fetchClientEpgSearch`; `/api/epg/search` | Primary | Useful | Useful | Primary | Primary | Read-ready, unversioned |
| Live video | No Suite media player or gateway | No | No wrapper/route for media bytes | Primary | Primary | Primary | Primary | Limited | Blocked; Phase 65 |
| Current LiveOverlay | Structured channel/programme state | Yes, in remote dialog | `fetchClientLiveOverlay`; `/api/vdr/live/overlay` | Primary | Primary | Useful | Useful | Useful | Read-ready transition; not video |
| Sender change | Remote `switchChannel` action | Yes | `fetchClientRemoteAction`; `/api/vdr/remote/actions` | Useful | Primary | Useful | Useful | Limited | Blocked for independent production client; Phase 62/67 |
| Volume/mute | Remote actions exist if backend maps them; no backend-neutral audio endpoint | Yes as remote keys | RemoteAction | Useful | Primary | Useful | Useful | Limited | Treat as remote control, not device volume; Phase 62/67 |
| Full VDR remote | 35-hotspot browser remote with allowlisted actions | Yes | live-remote wrapper | Primary | Limited; TV remote should map physical keys | Limited | Useful | Limited | Mutation safety/auth gap |
| Remote key repeat/long press | Pointer pressed state exists; no stable repeated-command contract | Partial | UI-owned; repeated API semantics undefined | Useful | Primary | Useful | Useful | No | Blocked; rate/lease/idempotency design required |
| Haptic feedback | Browser/device dependent, not implemented as contract | No | UI-only | Useful | No | No | No | No | Native UI concern |
| Recordings list/folders | Recordings 2 implemented | Yes | `fetchClientRecordings`, folder/cache wrappers | Primary | Primary | Primary | Primary | Primary | Read-ready; stable public IDs needed |
| Recording metadata/people/artwork | Implemented | Yes | metadata/person/folder detail routes | Primary | Primary | Useful | Primary | Primary | Read-ready; remove `backendNativeId` from public identity |
| Recording playback | No Suite MediaSession/Gateway | No | No public playback route | Primary | Primary | Primary | Primary | Limited | Blocked; Phase 65 |
| Resume position | No complete client contract | No | None | Primary | Primary | Useful | Primary | Useful | Later; Phase 65 plus identity/accountability |
| Audio/subtitle track selection | No Suite playback contract | No | None | Primary | Primary | Primary | Primary | Limited | Later; Phase 65 |
| Marks and cut functions | Domain/backend foundations not exposed as stable client contract | No/Partial | No stable wrapper | Useful | Limited | Limited | Primary | Limited | Later after safe mutation contract |
| Recording rename/move/trash | Guarded browser workflows | Yes | validation/execution wrappers | Useful | Limited | Limited | Primary | Limited | Blocked for independent client; Phase 62/67 |
| Timer list | Implemented | Yes | `fetchClientTimers` with fallback | Primary | Primary | Useful | Primary | Primary | Transition read |
| Timer conflicts | Implemented | Yes | `fetchClientTimerConflicts` | Primary | Primary | Useful | Primary | Primary | Read-ready, unversioned |
| Timer create/update/delete | Implemented native action routes | Yes | timer action wrappers | Primary | Useful | Limited | Primary | Limited | Blocked; Phase 62 then Phase 64/67 |
| TimerIntent/assignment | Accepted target only | No | None | Primary | Useful | No | Primary | Primary | Phase 64 |
| SearchTimer list/discovery/preview | Implemented foundations | Yes | wrappers with aliases/fallback | Useful | Limited | Limited | Primary | Useful | Transition read |
| SearchTimer mutation/execution | Implemented domain workflows with route aliases | Yes | mutation fallbacks exist | Useful | No | No | Primary | Limited | Blocked; Phase 62/64/67 |
| Global search | Backend-scoped persistent search | Yes | `fetchClientGlobalSearch`; `/api/search` | Primary | Primary | Useful | Primary | Primary | Read-ready, unversioned |
| Genre navigation | Implemented metadata-backed browser | Yes | genre wrapper/routes | Primary | Primary | Useful | Primary | Primary | Read-ready, unversioned |
| Metadata and people catalogue | Implemented bounded reads | Yes | metadata/person wrappers/routes | Primary | Primary | Useful | Primary | Primary | Read-ready; public identity/schema hardening |
| Notifications | No server notification resource or mobile delivery lifecycle | No | None | Primary | Useful | Limited | Useful | Useful | Later; Phase 62 accountability/events then product slice |
| Operations | Domain-specific operation foundations; no universal public lifecycle | Partial | No canonical wrapper | Primary | Useful | Limited | Primary | Primary | Phase 62/67 |
| Jobs | Basic `/api/jobs`; target durable semantics later | Partial | No dedicated current wrapper in main Client API | Useful | Limited | Limited | Primary | Primary | Internal/partial; Phase 62/63/67 |
| Change feed | Snapshot feed and SSE foundations | Partial | EventSource only for current live changed-domain events | Primary | Primary | Useful | Primary | Primary | Transition; authenticated public feed Phase 67 |
| Offline state | Browser loading/error states; no intentional offline cache | Partial | No service worker/cache contract | Primary | Primary | Limited | Primary | Limited | Client implementation plus public cache semantics |
| Administration/diagnostics | Runtime diagnostics and operator refresh routes exist | Yes, selected settings/diagnostics | `/api/runtime...`, native-fuzzy admin routes | Limited | No | No | Primary | Limited | Separate protected Admin API |
| Legacy OSD | Not implemented | No | None | Useful | Useful | Useful | Useful | No | Phase 66 |
| Android App Links/deep links | Not applicable to browser runtime | No | Future public resource URLs required | Primary | Useful | No | Useful | Useful | App work after canonical public URLs |
| Push token management | Not implemented | No | None | Primary | Useful | No | No | No | Later authenticated client-device resource |
| Multiple Suite installations | Browser origin-specific only | Partial | No client profile contract | Primary | Primary | Limited | Primary | Useful | Client design; Phase 62/67 discovery/auth |

## API candidate matrix

The target class values are:

- **Public v1** — stable client-independent resource/operation.
- **Compatibility route** — temporary unversioned alias mapped server-side.
- **Internal Web API** — bundled frontend transition support only.
- **Admin API** — separately authorized operator/diagnostic surface.
- **Media Plane** — byte delivery, not ordinary JSON domain API.
- **Agent protocol** — private Control Plane/Agent machine contract.
- **Provider adapter** — private VDR/plugin/provider boundary.
- **UI-only** — never a server contract.
- **No API candidate** — do not preserve.

| Domain/function | Current route | Current wrapper | Runtime owner | Suite identity | Backend scope | Client need | Read/mutation | Authentication | Revision | Idempotency | Media relation | Current stability | Target class | Target phase |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| API root | None | None | Future public API layer | Installation/API version | Installation | All clients | Read | Anonymous-minimal or actor-aware | No | No | Links only | Missing | Public v1 `/api/v1` | 67 |
| Capability discovery | `/api/vdr/capabilities` | `fetchClientCapabilities` | CapabilityController | Capability names | Backend/installation mixed | All clients | Read | Actor-aware filtering | Capability revision useful | No | Advertises media capability only | Internal transition | Public v1 `/capabilities` | 62/67 |
| Actor/session self | None | None | Future identity/session service | `actorId`, `sessionId` | Installation and granted scopes | Web/native | Read/mutation login/logout | Required | Session revision | Login-specific replay protection | None | Missing | Public v1 | 62/67 |
| Permission summary | None | None | Future authorization service | Actor/grant/policy IDs | Backend/resource scopes | Web/native | Read | Required | Policy revision | No | Media permissions included | Missing | Public v1, filtered self view | 62/67 |
| Backends | `/api/backends` | `fetchClientBackends` | BackendRegistryController/Service | Stable `backendId` | Collection/explicit | All clients | Read | Actor filters visibility | Backend representation ETag | No | Capability links | Useful but unversioned | Public v1 | 62/67 |
| Default backend | `/api/backends/default` | `fetchClientDefaultBackend` | BackendRegistryController | `backendId` | Implicit compatibility default | Bundled Web mainly | Read | Actor-aware | ETag useful | No | None | Compatibility concept | Compatibility route; prefer explicit client preference | 67 |
| Backend status/health | `/api/backends/{id}/status`, `/health`, `/snapshot` | snapshot wrapper; status helpers | VdrController/read services | `backendId`, generation | Explicit | All clients | Read | Required for protected installs | Snapshot/generation | No | Availability only | Useful transition | Public v1 backend subresources | 62/63/67 |
| Channels | `/api/vdr/channels` | `fetchClientChannels` | VdrController/snapshot read | Channel ID currently backend-native-derived | Often implicit default | All clients | Read | Required | Channel/read-model revision | No | Link to media session creation | Useful transition | Public v1 `/channels` | 45 identity foundation + 67 exposure |
| Channel movement | `/api/vdr/channels/move` | `fetchClientChannelMoveAction` | VdrChannelMoveController | Stable Channel + backend | Explicit required in target | Admin/Web | Mutation | Scoped admin/operator | Required | Required | None | Provisional | Public v1 operation or Admin API | 62/67 |
| Program events | `/api/vdr/events`, EPG window routes | `fetchClientEpgWindow` and EPG helpers | VdrController/EpgController/EpgCacheController | Canonical `programEventId` target | Explicit/aggregate reads | All clients | Read | Required where private | Event/read-model revision | No | Describes live resource | Multiple provisional shapes | Public v1 `/program-events` | 45 foundation + 67 |
| Now/next | `/api/epg/now-next` | `fetchClientEpgNowNext` | EpgController | Channel + ProgramEvent | Currently channel/default-oriented | All clients | Read | Required | ETag/cursor useful | No | Context only | Useful transition | Public v1 query/view | 67 |
| EPG cache status | `/api/epg/cache/status` | `fetchClientEpgCacheStatus` | EpgCacheController | Backend cache state | Explicit | Web diagnostics | Read | Operator | Cache generation | No | None | Internal | Admin API or internal health | 62/67 |
| EPG cache refresh | `/api/epg/cache/refresh` | `fetchClientEpgCacheRefresh`; legacy fetch interceptor | EpgCacheController | Backend operation | Explicit | Operator only | Mutation | Admin/operator | Generation/precondition | Required durable operation | None | Provisional debug/operator route | Admin API | 62/67 |
| Recordings collection/query | `/api/vdr/recordings/query` | `fetchClientRecordings` | VdrRecordingQueryController | Stable Recording target exists but transition response may expose native data | Explicit/implicit | All clients | Read | Required | Recording/read-model revision | No | Links to MediaSession | Useful transition | Public v1 `/recordings` | 67 |
| Recording folders | `/api/vdr/recordings/folder` | `fetchClientRecordingFolder` | VdrRecordingFolderController | Folder path today; target logical hierarchy | Explicit | Web/native | Read | Required | Folder/read-model revision | No | None | Internal transition | Public v1 hierarchy or query; no path identity | 67 |
| Recording metadata | `/api/vdr/recordings/metadata` | Detail modules through current wrappers | VdrRecordingFolderController/metadata services | Current `backendNativeId`; target Recording ID | Explicit | All clients | Read | Required | Metadata/Recording revision | No | Artwork links | Provisional identity | Public v1 Recording subresource | 67 |
| Recording metadata image | `/api/vdr/recordings/metadata/image` | Browser detail helpers | VdrRecordingFolderController | Current native ID + kind/index | Explicit | Web/native/TV | Read | Required | Cache validators | No | Small artwork HTTP, not video plane | Internal transition | Public v1 artwork resource/CDN-like response | 67 |
| Recording action validation | `/api/vdr/recordings/actions/validate` | `fetchClientRecordingActionValidation` | RecordingActionValidationController | Recording + backend | Target explicit | Web/desktop | Read-like preview | Required | Must return current revision | No execution key for pure validation | None | Strong domain foundation, incomplete universal envelope | Public v1 preview/operation support | 62/67 |
| Recording action execution | `/api/vdr/recordings/actions/execute` | `fetchClientRecordingActionExecution` | RecordingActionExecutionController/services/adapters | Stable Recording target required | Explicit | Web/desktop/mobile selective | Mutation | Required scoped actor | `If-Match` | Required durable | May affect playable resource | Provisional | Public v1 operation | 62/67 |
| Persons | `/api/persons`, `/api/vdr/persons` | `fetchClientPersons` with fallback | PersonController | Person identity/provider references | Aggregate or filtered | All clients | Read | Required where catalogue private | ETag | No | Artwork links only | Alias/provisional | Public v1 `/persons` | 67 |
| Recording-person search | `/api/.../recordings/persons/search` | `fetchClientRecordingPersons` with fallback | RecordingPersonSearchController | Recording + Person | Explicit backend | Web/native | Read | Required | Read-model revision | No | None | Alias/provisional | Public v1 query | 67 |
| Genres | `/api/metadata/genres` | `fetchClientGenres` | GenreBrowserApiRuntime/service/repository | Canonical Genre ID | Explicit/aggregate query | All clients | Read | Required | Assignment/read-model revision | No | None | Useful unversioned | Public v1 `/genres` | 67 |
| Genre recordings | `/api/metadata/genres/recordings` | `fetchClientGenreRecordings` | Genre browser service/repository | Genre + Recording IDs | Explicit | All clients | Read | Required | Collection cursor/ETag | No | Recording links | Useful unversioned | Public v1 relation/query | 67 |
| Genre EPG | `/api/metadata/genres/epg` | `fetchClientGenreEpg` | Genre browser service/repository | Genre + ProgramEvent IDs | Explicit | All clients | Read | Required | Collection cursor/ETag | No | Event links | Useful unversioned | Public v1 relation/query | 67 |
| Global search | `/api/search`, `/api/vdr/search` | `fetchClientGlobalSearch` | GlobalSearchApiRuntime/controller/service/repository | Typed result refs | Explicit selected backend today | All clients | Read | Required | Index/read-model generation + cursor | No | Result links only | Strong read slice, unversioned | Public v1 `/search` | 67 |
| Native timers | `/api/vdr/timers`, `/live` | `fetchClientTimers` with fallback | VdrController/snapshot | Native timer identity currently exposed | Often implicit default | All clients | Read | Required | Timer revision | No | None | Transition | Public v1 native binding read or hidden behind TimerIntent | 64/67 |
| Timer conflicts | `/api/vdr/timer-conflicts/live` aliases | `fetchClientTimerConflicts` | VdrController | Conflict/Timer/Event IDs | Backend-scoped | All clients | Read | Required | Snapshot sequence | No | None | Transition | Public v1 conflict resource | 64/67 |
| Timer create/update/delete | `/api/vdr/timers/actions/*` | timer action wrappers | VdrTimerActionController/executor registry | Native timer today; TimerIntent target | Explicit required | Web/native/desktop | Mutation | Required | Required | Required | None | Provisional | Public v1 TimerIntent operation | 62/64/67 |
| SearchTimer collection | `/api/searchtimers`, `/api/vdr/searchtimers` | fallback wrapper | SearchTimerController | SearchTimer ID | Explicit/implicit | Web/desktop/integrations | Read | Required | SearchTimer revision | No | None | Aliased | Public v1 `/search-timers` | 62/64/67 |
| SearchTimer preview/plan | Multiple aliases | fallback wrappers | SearchTimerController/discovery/cache services | SearchTimer/proposal | Explicit target | Web/desktop | Read/preview | Required | Preview binding/revision | No execution for preview | None | Provisional | Public v1 preview/planning resource | 62/64/67 |
| SearchTimer execute/update/delete | Multiple aliases | mutation fallbacks | SearchTimerController/executor | SearchTimer/TimerIntent target | Explicit | Web/desktop | Mutation | Required | Required | Required | None | Unsafe as independent-client contract | Public v1 operation; no client fallback | 62/64/67 |
| RemoteAction | `/api/vdr/remote/actions` | `fetchClientRemoteAction` | LiveRemoteApiRuntime/controller/service/private executor | Operation + backend; key action | Explicit in body | Web/mobile/TV | Mutation | Required control scope | Backend generation/lease where relevant | Required/rate controlled | Control Plane, not media | Implemented transition | Public v1 command/operation | 62/63/67 |
| LiveOverlay | `/api/vdr/live/overlay` | `fetchClientLiveOverlay` | LiveRemoteApiRuntime/overlay services | Backend + snapshot sequence | Explicit query | Web/mobile/TV | Read | Required | Snapshot/overlay revision | No | Descriptive only | Implemented transition | Public v1 live state resource | 62/67 |
| Snapshot change feed | `/api/vdr/changes` | No main wrapper | SnapshotChangeFeedController | Event sequence | Backend-aware model | Integrations/clients | Read | Required | Cursor/sequence | No | Not media | Internal foundation | Public v1 change feed | 62/67 |
| Live SSE notifications | `/api/vdr/live` | `createClientLiveUpdateSource` | LiveTransportController | Sequence/change domains | Event payload backend-scoped | Web/native | Read subscription | Required | Cursor/sequence | No | Not media | Transition | Public v1 events/SSE | 62/67 |
| Jobs | `/api/jobs` | None in main wrapper | JobsController | Job ID | Needs explicit scope | Desktop/admin/integrations | Read | Required | Job revision | Submission idempotency when created | May reference media work | Basic/partial | Public v1 jobs, protected | 62/63/67 |
| Operations | No universal route | None | Future operation service/repository | `operationId` | Explicit | All mutating clients | Read/create/cancel | Required | Operation revision | Core requirement | May reference MediaSession | Missing universal runtime | Public v1 `/operations` | 62/67 |
| MediaSession | None | None | Future Media Session Service | `mediaSessionId` | Explicit route policy | Playback clients | Create/read/revoke | Required play scope | Session/resource revision | Session creation key useful | Domain control for media | Missing | Public v1 `/media-sessions` | 65/67 |
| Video bytes | None | None | Future Streaming Gateway | Session/grant/connection IDs | Route-owned | Playback clients | Media connection | Short-lived grant | Protocol/session epoch | Connection semantics, not mutation key | Actual Media Plane | Missing | Media Plane | 65 |
| Resume/progress | None | None | Future playback state service | Actor + Recording + position | Explicit | Playback clients | Read/mutation | Required | Position revision/conflict policy | Required for safe retry if persisted | Media-related domain API | Missing | Public v1 playback state | 65/67 |
| Legacy OSD session | None | None | Future OSD bridge | OSD session/frame/controller lease IDs | Explicit | Selected clients | Session read/control | Required distinct scopes | Frame sequence/lease epoch | Fenced commands | Separate OSD data plane | Missing | Public v1 session control + OSD plane | 66/67 |
| Runtime diagnostics | `/api/runtime...` | Selected settings code, not core wrapper | RuntimeDiagnosticsController | Runtime/build/backend diagnostics | Installation/backend | Admin | Read | Strong admin scope | Snapshot timestamp | No | Must redact media/provider secrets | Internal | Admin API | 62/67 |
| Native fuzzy refresh/stale delete | `/api/.../epgsearch/native-fuzzy/...` | No public wrapper | Operator administration controllers | Probe/admin operation | Backend/runtime | Admin only | Mutation | Strong admin scope | Required | Required | None | Operator-internal | Admin API | 62/67 |
| Audit/accountability query | None | None | Future audit service/repository | Audit event/actor/request/operation IDs | Actor/backend/resource filters | Admin/security | Read | Restricted | Append-only cursor | No | May reference session, never reveal grant secret | Missing | Admin API | 62/67 |
| Backend Agent enrollment/commands | None as production Agent protocol | None | Future Control Plane/Agent runtime | Agent/backend/site/generation | Explicit | No public client | Machine protocol | Mutual machine trust | Generation/lease | Required | Private provider route | Missing | Agent protocol | 63 |
| RESTfulAPI | Private adapter calls | None directly | RestfulApiVdrAdapter and specialised private executors/providers | Native IDs behind binding | Local backend | No public client | Adapter read/mutation | Agent/local trust | Adapter-specific | Internal dedupe only below Suite envelope | May reach provider | Private | Provider adapter | Existing/private |
| SVDRP | SuiteBridge/local adapter path | None directly | SuiteBridge/private transport | Native IDs/commands | Local backend | No public client | Adapter | Agent/local trust | Adapter-specific | Internal | Not public media | Private | Provider adapter/Agent local transport | Existing/private |
| Streamdev | No public route | None | Future StreamProvider behind Agent | Provider lease only | Local backend/route | No public domain client | Media provider | Agent/provider credentials | Lease/route epoch | Connection lifecycle | Private source bytes | Not implemented as Suite gateway | Provider adapter | 65 |
| TVScraper raw data | No public raw route | None | Provider workers/repositories | Provider evidence mapped to Suite targets | Backend/target | No raw client need | Provider ingest | Private provider trust | Evidence version | Job idempotency | Artwork source possible | Private | Provider adapter | Existing/post-61 |
| DOM/CSS/dialog/pressed state | None | Browser only | Frontend modules | None | None | No independent client need | UI state | None | None | None | None | UI implementation | UI-only | Never |
| Remote hotspot geometry | Static browser array/image coordinates | Browser module | `remote.js` | None | None | No API need | UI state | None | None | None | None | UI implementation | UI-only | Never |
| Direct provider URL | None should be public | None | Provider/Agent only | Not a Suite resource ID | Private route | No | No public operation | Never sufficient | N/A | N/A | Security-sensitive | Forbidden target | No API candidate | Never |

## Explicit non-public data and behaviour

The following must remain outside the public API:

- DOM structures and element IDs;
- CSS classes and browser breakpoints;
- scroll position and opened dialogs;
- pressed-state and local busy flags;
- hotspot geometry and image coordinates;
- JavaScript module names and global objects;
- speculative route fallbacks;
- SQLite table/column names;
- local recording paths;
- VDR pointers, list positions and file descriptors;
- raw RESTfulAPI fields and reply shapes;
- raw SVDRP responses;
- TVScraper provider payloads without Suite normalization;
- SuiteBridge messages and Agent transport frames;
- plugin-local schemas;
- permanent Streamdev URLs;
- hostnames/IP addresses as resource identity;
- arbitrary command tunnels;
- unprotected refresh/debug endpoints;
- translated UI text as machine error codes.

## Gap register

| Gap | Current evidence | Client impact | Required outcome | Owner phase | PoC rule |
| --- | --- | --- | --- | --- | --- |
| Actor identity/session | Accepted ADR, no production runtime | Cannot safely distinguish household users, admins or integrations | Stable actor, bounded revocable sessions, logout/revocation | 62 | Read-only local adapter only |
| Central RBAC | Backend read-only exists, full actor policy missing | Client cannot infer permissions safely | Server-side backend/resource/action authorization | 62 | Display server capability/read-only only; never decide authority |
| Accountability | Request/operation/audit chain incomplete | Mobile timeout and support diagnosis are ambiguous | Request/correlation IDs, append-only events, outbox | 62 | Local diagnostics only |
| Universal mutation envelope | Domain-specific foundations only | Retry can duplicate or obscure outcome | Revision, `If-Match`, idempotency, durable operations | 62/67 | No mutations |
| Secure remote site | Private VDR ports must not be exposed | App cannot safely connect across sites | Enrolled Agent, mTLS-equivalent trust, generation/lease | 63 | One local Suite origin |
| TimerIntent orchestration | Native timer actions exist | Multi-backend timer semantics unstable | Intent, assignment, binding, scheduler, reconciliation | 64 | Timer read only |
| Streaming Gateway | No media session/player route | No product live/recording playback | MediaSession, grants, Gateway, provider leases | 65 | Overlay only |
| Legacy OSD plane | LiveOverlay is not OSD | Cannot mirror/control arbitrary OSD safely | Sequenced frames/deltas and fenced controller lease | 66 | Excluded |
| Public `/api/v1` | Current aliases/unversioned errors | Native client could freeze accidents | Root, schemas, errors, compatibility/deprecation | 67 | Replaceable compatibility adapter |
| Stable Channel/Event identity exposure | Native/snapshot IDs vary | Deep links and cache keys may drift | Canonical Suite IDs and provenance | 45 foundation/67 exposure | Treat IDs as adapter-local |
| Stable Recording public identity | Some routes use path/native ID | Move/rename breaks client identity | Opaque Recording ID and binding | Existing foundation/67 exposure | No persisted deep link across mutation |
| Pagination/cursors | Mixed limit/offset | Large catalogues and event feeds inconsistent | Standard collection/cursor rules | 67 | Bounded pages only |
| Structured errors | Ad-hoc `{"error":...}` and JS strings | No reliable retry/auth/conflict UX | Stable problem codes/status/request ID | 67 | Adapter maps best-effort categories |
| Notification resource | No server/client-device model | Push cannot be permission-scoped or revoked | Notification preferences, device token lifecycle, event source | 62 plus later slice | Excluded |
| PWA shell | No manifest/service worker | Browser install/offline weak | Manifest, secure cache model, offline shell | Separate web enhancement | Optional independent work |
| Android/TV UI | No native project | No native lifecycle/focus/player | Shared core plus separate mobile/TV UI | After study; runtime gates remain 62–67 | Fake-server UI prototype allowed |
| Admin API separation | Diagnostics mixed in `/api` | General clients may see operator concepts | Separate authorization, routes and redaction | 62/67 | Do not consume admin routes |

## Recommended delivery slices

These are work packages, not new numbered runtime phases.

### Documentation and contract fixtures

- keep matrices current against main;
- capture representative current responses as non-stable fixtures;
- define target v1 resource examples without claiming implementation;
- add compatibility tests during Phase 67.

### Native fake-server prototype

- Kotlin/Compose project with shared core modules;
- phone/tablet navigation;
- TV focus prototype in a separate app surface;
- deterministic fake responses and errors;
- Media3 test source unrelated to production VDR routes.

### Read-only compatibility PoC

- selected local installation;
- backends, status, channels, EPG, recordings, Genres, search and LiveOverlay;
- no mutation;
- no direct provider URLs;
- all route strings isolated in one adapter.

### Product client

Only after relevant gates:

- Phase 62 for login, sessions, permissions and accountability;
- Phase 65 for playback;
- Phase 67 for stable public API and SDK publication.

## Decision gates

A production Android beta must not begin until:

- actor/session and authorization contracts are implemented for its functions;
- every included mutation has universal operation/revision/idempotency semantics;
- media uses a Suite MediaSession and Gateway;
- no current alias or backend-native field is treated as stable;
- API compatibility and error fixtures exist;
- phone/tablet lifecycle tests pass;
- Android TV has separate D-pad/focus tests;
- real VDR-system acceptance confirms channel changes, playback, reconnect and read-only denials without exposing private provider details.
