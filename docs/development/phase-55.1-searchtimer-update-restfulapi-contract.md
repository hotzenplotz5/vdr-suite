# Phase 55.1 - SearchTimer Update RESTfulAPI Contract Fix

## Status

Implemented. Awaiting GitHub Actions verification.

## Purpose

Phase 55.0 identified a high-priority adapter gap: VDR-Suite used `PUT /searchtimers/<id>` for SearchTimer updates, while the inspected `yavdr/vdr-plugin-restfulapi` SearchTimer responder handles `GET`, `POST` and `DELETE`, but not `PUT`.

The upstream-compatible update path is therefore treated like a save through the normal SearchTimer endpoint:

```text
POST /searchtimers
```

with the SearchTimer id included in the JSON request body.

## Changed files

```text
core/vdr/src/RestfulApiSearchTimerCommandExecutor.cpp
core/vdr/tests/test_restful_api_search_timer_command_executor.cpp
```

## Behaviour before this phase

VDR-Suite SearchTimer update built this request:

```text
PUT /searchtimers/<backendNativeId>
Content-Type: application/json
```

The request body did not include an `id` field.

## Behaviour after this phase

VDR-Suite SearchTimer update now builds this request:

```text
POST /searchtimers
Content-Type: application/json
Accept: text/plain
```

The request body includes the backend-native SearchTimer id as a numeric JSON field:

```json
{
  "id": 17,
  "search": "..."
}
```

This matches the inspected RESTfulAPI behaviour where `SearchTimer::LoadFromQuery()` reads `id` from the request body and `SearchTimers::Save()` persists the SearchTimer.

## Regression coverage

The existing target is extended:

```text
test-restful-api-search-timer-command-executor
```

New coverage:

- update uses `POST`
- update uses `/searchtimers`
- update does not use `/searchtimers/<id>`
- update sends `Content-Type: application/json`
- update sends `Accept: text/plain`
- update body contains numeric `"id":17`
- update preserves the returned backend-native id in the result

## Remaining follow-up

This phase intentionally does not solve the separate Phase 55.2 payload-shape gap:

- `blacklist_ids` still needs JSON-array parity
- `ext_epg_info` still needs JSON-array parity

That remains a separate implementation phase because it touches request models and parsing semantics beyond the update transport contract.

## Verification

No local test was run by this change author.

Expected GitHub Actions/local target:

```bash
make test-restful-api-search-timer-command-executor
```

If real yaVDR validation is needed after CI, use the existing SearchTimer real smoke path instead of treating the mock contract test as full runtime proof.
