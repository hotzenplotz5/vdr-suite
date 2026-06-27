# Phase 55.0 - VDR-Suite Feature-Parity and Adapter Audit

## Status

Initial audit record.

This phase is documentation-only. It does not change production code, test code or runtime behaviour.

## Scope

This audit compares the current VDR-Suite architecture with the relevant upstream/reference components:

- VDR-Suite `main`
- `yavdr/vdr-plugin-restfulapi`
- `vdr-projects/vdr-plugin-live`
- `vdr-projects/vdr-plugin-epgsearch`
- VDR core concepts as exposed through RESTfulAPI and plugin APIs

The focus is practical parity and adapter correctness:

- Are RESTfulAPI endpoints called with the right methods, paths and payload shapes?
- Which Live/EPGSearch features are present, partial or missing in VDR-Suite?
- Which gaps should become concrete Phase 55.x implementation work?

## Source files inspected in this pass

### VDR-Suite

- `core/vdr/src/RestfulApiVdrAdapter.cpp`
- `core/recordings/src/RestfulApiRecordingActionExecutor.cpp`
- `core/recordings/include/RestfulApiRecordingActionRequestBuilder.h`
- `core/vdr/src/RestfulApiVdrTimerActionExecutor.cpp`
- `core/vdr/include/RestfulApiVdrTimerActionRequestBuilder.h`
- `core/vdr/src/RestfulApiSearchTimerCommandExecutor.cpp`
- `core/vdr/src/RestfulApiSearchTimerMapper.cpp`
- `core/vdr/include/SearchTimer.h`
- `core/vdr/include/SearchTimerCreateRequest.h`
- `core/vdr/include/SearchTimerUpdateRequest.h`
- `core/vdr/src/EpgSearchService.cpp`
- `core/vdr/src/EpgSearchMatcher.cpp`
- `api/rest/src/VdrController.cpp`
- `api/rest/include/VdrController.h`
- `api/rest/src/ApiRouter.cpp`
- `core/vdr/include/SearchTimerDiscovery.h`
- `core/vdr/src/SearchTimerDiscoveryService.cpp`
- `api/rest/src/SearchTimerDiscoveryController.cpp`

### RESTfulAPI

- `recordings.cpp`
- `timers.cpp`
- `events.cpp`
- `searchtimers.cpp`
- `epgsearch.cpp`
- `tools.h`
- `tools.cpp`

### Live / EPGSearch reference material

- `vdr-plugin-live` README
- `vdr-plugin-epgsearch` HISTORY

## Executive summary

VDR-Suite has the right architectural foundation: backend abstraction, snapshot reads, backend-aware routing, selective EPG queries, SearchTimer workflow safety gates, readback verification, recording actions and timer actions.

The current gaps are mostly not structural. They are adapter-parity and semantic-parity gaps:

1. SearchTimer update against RESTfulAPI appears to use the wrong HTTP method/path for the current upstream RESTfulAPI implementation.
2. SearchTimer create/update payloads flatten fields that upstream expects as arrays for selected blacklists and extended EPG info.
3. Timer create/update uses `application/x-www-form-urlencoded` but does not URL-encode values, which can break titles, directories and aux data containing `&`, `=`, `+` or spaces.
4. EPGSearch preview currently implements a useful internal matcher but is not semantically equivalent to epgsearch for the full SearchTimer feature set.
5. RESTfulAPI discovery endpoints for SearchTimer support exist upstream and VDR-Suite has a discovery domain/controller foundation, but provider/wiring parity still needs verification and likely completion.
6. Live parity is partially present for core read domains, but still missing important interactive domains such as conflict information, recording marks/cut/play/rewind and richer discovery/status surfaces.

## RESTfulAPI adapter audit

### Basic read adapter

VDR-Suite currently calls:

- `GET /info.json`
- `GET /channels.json`
- `GET /events...json`
- `GET /timers.json`
- `GET /recordings.json`
- `GET /change-state.json`

Assessment:

- The endpoint shape matches RESTfulAPI's classic JSON resource model.
- `events` uses `from`, `timespan`, `start`, `limit`, `chevents` and `only_count`, which matches RESTfulAPI's event query option names.
- The main risk is parser robustness, because several VDR-Suite mappers still use lightweight string parsing instead of a full JSON parser.

Priority:

- Medium.
- Keep current path, but add regression tests for escaped strings, missing fields, negative values and large numeric fields.

### Recording actions

VDR-Suite builds:

- `POST /recordings/move.json`
- JSON body with `source`, `target`, `copy_only`
- `POST /recordings/delete.json`
- JSON body with `file`

RESTfulAPI accepts:

- `/recordings/move` with `POST`
- `/recordings/delete` with `POST` or `DELETE`
- JSON body parsing through `QueryHandler` when the body starts with `{`

Assessment:

- Method and endpoint are aligned.
- JSON body is acceptable because RESTfulAPI parses JSON request bodies.
- The `.json` suffix is acceptable because RESTfulAPI's `QueryHandler` uses it as output format detection.
- The move/rename target semantics are delicate: RESTfulAPI's move implementation builds a new path as `videoDir + target + old recording leaf`. VDR-Suite therefore correctly tries to preserve the `.rec` leaf for folder moves.
- Rename currently maps to `/recordings/move.json` with a target derived from `newName`. This is compatible with RESTfulAPI's implementation pattern, but it needs real backend regression tests for nested paths, spaces, umlauts and already-encoded `~` folders.

Risks:

- Target encoding is easy to get wrong for VDR folder separators.
- Spaces are converted to underscores for rename targets, but not for folder move targets.
- Move/copy semantics must remain explicit; current VDR-Suite sends `copy_only:false`.

Priority:

- Medium-high.

Recommended Phase 55.x work:

- Add a real RESTfulAPI recording action contract test matrix for:
  - nested folder move
  - rename with spaces
  - rename with umlauts
  - source path with `~`
  - target path with `/`
  - delete by backend-native path

### Timer actions

VDR-Suite builds:

- `POST /timers`
- `PUT /timers`
- `DELETE /timers/<timerId>`
- `Content-Type: application/x-www-form-urlencoded`
- fields: `flags`, `channel`, `weekdays`, `day`, `start`, `stop`, `priority`, `lifetime`, `file`, `aux`, `timer_id`

RESTfulAPI accepts:

- `GET`, `DELETE`, `POST`, `PUT` on `/timers`
- `POST` for create
- `PUT` for update
- `DELETE /timers/<id>` for delete
- body fields matching the current VDR-Suite builder

Assessment:

- Method and field names are broadly aligned.
- The major issue is URL/form encoding: VDR-Suite concatenates raw `name=value` pairs without percent-encoding values.
- This is safe only for simple ASCII values without reserved form characters.
- It can break timer file names, aux metadata or directory/title values containing `&`, `=`, `+`, `%` or non-trivial whitespace.

Priority:

- High.

Recommended Phase 55.x work:

- Add a shared form-url-encoding utility for RESTfulAPI form bodies.
- Apply it to timer action request bodies.
- Add tests for `file=Film & Serie`, `aux=<xml>...</xml>`, plus signs, equals signs and umlauts.

### SearchTimer read mapping

VDR-Suite maps a large part of RESTfulAPI SearchTimer JSON:

- id/search/state
- recording options
- schedule options
- channel options
- duration/day/time options
- repeat options
- blacklist mode and IDs
- matching options
- extended EPG options
- validity/action options
- delete/keep options

Assessment:

- The read model is already broad and is a good basis for parity.
- It still loses semantic detail in places where upstream uses complex arrays/config structures but VDR-Suite stores strings.
- `name` is effectively `search`, because RESTfulAPI exposes no separate display name in the inspected serializer.

Priority:

- Medium.

Recommended Phase 55.x work:

- Add fixtures from real `/searchtimers.json` output and assert full parsing coverage.
- Preserve raw upstream JSON or raw backend fields for unknown/new SearchTimer fields.

### SearchTimer create/update/delete

VDR-Suite currently builds:

- create: `POST /searchtimers`
- update: `PUT /searchtimers/<id>`
- delete: `DELETE /searchtimers/<id>`
- JSON body with many SearchTimer fields

RESTfulAPI SearchTimersResponder currently advertises and handles:

- `GET`
- `POST`
- `DELETE`
- no `PUT`

Important finding:

- `DELETE /searchtimers/<id>` is aligned.
- `POST /searchtimers` is aligned for create.
- `PUT /searchtimers/<id>` is probably not aligned with the inspected upstream RESTfulAPI implementation.
- RESTfulAPI appears to use `POST /searchtimers` plus body field `id` to load/save an existing SearchTimer through `SearchTimer::LoadFromQuery()` and `SearchTimers::Save()`.

This is the highest-priority adapter finding in this audit.

Priority:

- Critical/high.

Recommended Phase 55.x work:

- Verify against real VDR: run one update through current VDR-Suite and confirm whether RESTfulAPI returns 404/405/501 or fails silently.
- If confirmed, change VDR-Suite update to `POST /searchtimers` with `id` in the JSON body.
- Add regression tests proving create/update/delete HTTP method/path/body against RESTfulAPI contract.
- Keep readback verification mandatory after update.

### SearchTimer complex payload fields

RESTfulAPI `SearchTimer::LoadCommonFromQuery()` expects arrays for:

- `ext_epg_info`
- `blacklist_ids`

VDR-Suite currently serializes:

- `ext_epg_info` as a string
- `blacklist_ids` as a string

Assessment:

- For simple SearchTimers this is harmless.
- For selected blacklists and extended EPG info, RESTfulAPI may ignore the string because it asks `QueryHandler::getBodyAsArray()`.
- This can produce false confidence: VDR-Suite sends fields, but upstream does not apply them.

Priority:

- High.

Recommended Phase 55.x work:

- Represent selected blacklist IDs as a vector in the command request model.
- Represent extended EPG info selections as a vector in the command request model.
- Serialize both as JSON arrays for RESTfulAPI.
- Keep string compatibility only for read/display or legacy compatibility.

## EPG / EPGSearch parity audit

### Current VDR-Suite EPG search

VDR-Suite currently provides an internal matcher supporting:

- phrase search
- exact search
- all words
- any word
- regex
- fuzzy via Levenshtein distance
- selected fields: title/subtitle/description
- channel interval in a simplified form
- duration window
- content descriptor matching

Assessment:

- This is useful for preview and local matching.
- It is not yet full epgsearch semantic parity.

Known gaps against epgsearch/restfulapi semantics:

- channel groups
- only free-to-air channel scope
- day-of-week and time windows in preview matching
- SearchTimer action semantics
- extended EPG category matching
- ignore missing EPG categories behaviour
- blacklist filtering
- avoid repeats semantics
- compare date/time/category subtleties
- parental rating as a first-class search parameter
- aux-field search
- conflict check / check-advised status
- native epgsearch result comparison

Priority:

- High for preview correctness.
- Medium for general EPG search API.

Recommended Phase 55.x work:

- Add a native epgsearch preview mode backed by RESTfulAPI `/events/search.json` or `/searchtimers/search/<id>.json` where available.
- Keep VDR-Suite matcher as fallback/offline preview mode.
- Expose preview result authority clearly: `native`, `local`, `cache-warming`, `stale`, `unavailable`.

## SearchTimer discovery parity

RESTfulAPI exposes SearchTimer support endpoints:

- `/searchtimers/channelgroups.json`
- `/searchtimers/recordingdirs.json`
- `/searchtimers/blacklists.json`
- `/searchtimers/conflicts.json`
- `/searchtimers/extepginfo.json`
- `/searchtimers/update`

VDR-Suite already has:

- `SearchTimerDiscovery` domain model
- `SearchTimerDiscoveryService`
- `SearchTimerDiscoveryController`
- router route `/api/searchtimers/discovery` and `/api/vdr/searchtimers/discovery`

Assessment:

- The domain and controller foundation exists.
- The audit did not yet verify a completed RESTfulAPI-backed provider implementation and daemon wiring for all upstream discovery endpoints.
- This should be verified before implementing new UI features around advanced SearchTimer setup.

Priority:

- High.

Recommended Phase 55.x work:

- Verify provider/wiring status.
- If missing or partial, implement `RestfulApiSearchTimerDiscoveryProvider`.
- Fetch and map channel groups, recording dirs, blacklists and extended EPG info.
- Add conflicts separately because conflict status is operational, not static discovery.

## Live parity audit

Live is a reference for interactive VDR web usage because it runs inside VDR and has direct access to VDR data structures. VDR-Suite intentionally uses a different architecture: service boundary, REST adapters, snapshots, backend IDs and future multi-backend support.

Current VDR-Suite parity strengths:

- status read
- channels read
- events read
- timers read
- recordings read
- SearchTimer read/search workflow foundation
- EPG targeted APIs
- backend-aware snapshot foundation
- change feed foundation
- live transport foundation
- recording actions
- timer actions
- SearchTimer guarded mutation workflow

Current parity gaps:

- recording marks read/write/delete through VDR-Suite API
- recording cut status / cut command
- recording play / rewind commands
- recording sync/update endpoints
- event images and channel logos
- conflict check surface
- timer conflict check-advised status
- channel group and recording directory discovery in the public API, if not already wired
- full SearchTimer edit parity
- full EPGSearch result parity
- live-like UI state around VDR being edited/locked/busy

Priority:

- Medium-high.

Recommended Phase 55.x work:

- Do not attempt all Live parity at once.
- Split into narrow backend contracts:
  - SearchTimer native update contract
  - SearchTimer discovery provider
  - Timer form encoding
  - Recording marks/cut/play/rewind audit
  - Native EPGSearch preview mode
  - Conflict status API

## Proposed Phase 55.x roadmap

### Phase 55.1 - RESTfulAPI SearchTimer update contract fix

Goal:

- Verify and fix VDR-Suite SearchTimer update against RESTfulAPI.

Work:

- Confirm whether RESTfulAPI rejects `PUT /searchtimers/<id>`.
- Change update to the upstream-compatible method if needed.
- Likely target: `POST /searchtimers` with `id` in body.
- Add request-builder contract tests.
- Add real backend smoke test for update + readback.

Risk:

- High if left unfixed: update workflow may be green in mock tests but fail on real VDR.

### Phase 55.2 - SearchTimer JSON array payload parity

Goal:

- Make `blacklist_ids` and `ext_epg_info` payloads match RESTfulAPI expectations.

Work:

- Replace string-only command fields with structured arrays or add parallel structured fields.
- Serialize arrays in create/update JSON.
- Add mapper/readback tests.

Risk:

- Advanced SearchTimer options may appear saved but not actually apply upstream.

### Phase 55.3 - Timer form body URL encoding

Goal:

- Make timer create/update safe for real titles, directories and aux data.

Work:

- Add form-url-encoding helper.
- Apply it in `RestfulApiVdrTimerActionRequestBuilder`.
- Add tests for reserved characters.

Risk:

- Timer creation/update can break with normal recording names containing reserved form characters.

### Phase 55.4 - RESTfulAPI SearchTimer discovery provider verification/completion

Goal:

- Complete the bridge from RESTfulAPI discovery endpoints to VDR-Suite discovery API.

Work:

- Verify provider and daemon wiring.
- Map channel groups, recording directories, blacklists and extended EPG info.
- Keep conflicts separate or expose as operational status.

Risk:

- Advanced SearchTimer UI cannot be reliably built without these option catalogs.

### Phase 55.5 - Native EPGSearch preview mode

Goal:

- Make preview results comparable to epgsearch where the backend supports it.

Work:

- Add native provider path using RESTfulAPI `/events/search.json` or `/searchtimers/search/<id>.json`.
- Keep local matcher as fallback.
- Mark result authority explicitly.

Risk:

- Current preview may disagree with epgsearch for advanced criteria.

### Phase 55.6 - Live recording operations parity audit

Goal:

- Decide which Live/RESTfulAPI recording operations belong in VDR-Suite.

Work:

- Audit marks, cut, editedfile, play, rewind, updates and sync.
- Create separate capability flags.
- Add read-only vs mutation policy gates.

Risk:

- Some operations are interactive or destructive and need stronger safety gates than normal reads.

## Immediate recommendation

Start Phase 55.1 next.

Reason:

SearchTimer update is already part of the implemented VDR-Suite workflow and appears to have the highest chance of being wrong against real RESTfulAPI. It is narrow, testable and high-value.

## Verification notes

No local tests were run for this audit.

After any implementation phase derived from this audit, use GitHub Actions as the default test gate and real yaVDR smoke tests only where the behaviour depends on actual VDR/RESTfulAPI runtime semantics.
