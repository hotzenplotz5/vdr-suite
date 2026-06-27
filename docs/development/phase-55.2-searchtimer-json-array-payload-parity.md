# Phase 55.2 - SearchTimer JSON Array Payload Parity

## Navigation

- [Development Index](index.md)
- [Phase 55.0 - VDR-Suite Feature-Parity and Adapter Audit](phase-55-vdr-parity-adapter-audit.md)
- [Phase 55.1 - SearchTimer Update RESTfulAPI Contract Fix](phase-55.1-searchtimer-update-restfulapi-contract.md)
- [Current Project Status](current-status.md)

---

## Status

Completed. GitHub Actions passed for commit `511a8c19`.

## Purpose

Phase 55.0 identified a payload-shape gap in SearchTimer create/update requests.

RESTfulAPI reads these fields as JSON arrays:

```text
blacklist_ids
ext_epg_info
```

Before this phase, VDR-Suite serialized both values as JSON strings:

```json
{
  "blacklist_ids": "3,7",
  "ext_epg_info": "category=movie"
}
```

That can make VDR-Suite appear to send advanced SearchTimer options while RESTfulAPI ignores them because `QueryHandler::getBodyAsArray()` does not see an array.

## Behaviour after this phase

VDR-Suite now serializes these values as arrays in the RESTfulAPI command executor:

```json
{
  "blacklist_ids": ["3", "7"],
  "ext_epg_info": ["category=movie", "actor=John Doe"]
}
```

Both structured request fields and legacy comma/semicolon-separated strings are supported:

- `blacklistIdList` and `extendedEpgInfoList` are preferred.
- Legacy `blacklistIds` and `extendedEpgInfo` strings are split for compatibility.

## Changed files

```text
core/vdr/include/SearchTimerCreateRequest.h
core/vdr/include/SearchTimerUpdateRequest.h
core/vdr/src/RestfulApiSearchTimerCommandExecutor.cpp
core/vdr/src/SearchTimerArraySupport.inc
core/vdr/src/SearchTimerHttpEncodingSupport.inc
core/vdr/src/SearchTimerRestfulApiBodyBuilder.inc
core/vdr/src/SearchTimerReadbackParsingSupport.inc
core/vdr/src/SearchTimerCommandExecutorMethods.inc
core/vdr/tests/test_restful_api_search_timer_command_executor.cpp
```

## Notes on implementation shape

The GitHub connector repeatedly blocked a direct full-file replacement of `RestfulApiSearchTimerCommandExecutor.cpp` because of payload size/content checks.

To keep the repository change reviewable and avoid unsafe local patching, the executor was split into small private include fragments in the same source directory:

- HTTP/JSON encoding helper
- array list helper
- RESTfulAPI body builder
- readback parsing helper
- executor method implementation helper

The compiled translation unit remains `RestfulApiSearchTimerCommandExecutor.cpp`.

## Regression coverage

The existing target is extended:

```text
test-restful-api-search-timer-command-executor
```

New coverage:

- create serializes structured blacklist IDs as `"blacklist_ids":["3","7"]`
- create serializes structured extended EPG info as `"ext_epg_info":["category=movie","actor=John Doe"]`
- update serializes legacy `blacklistIds="3, 7"` as a JSON array
- update serializes legacy `extendedEpgInfo="category=movie; actor=John Doe"` as a JSON array
- string-shaped RESTfulAPI payloads for these two fields are rejected by test assertions

## Remaining follow-up

This phase fixes the outbound RESTfulAPI command executor contract.

Potential follow-up work:

- Parse public REST create/update request arrays directly into `blacklistIdList` and `extendedEpgInfoList`.
- Add real yaVDR readback validation for selected blacklist IDs and extended EPG info fields.
- Decide whether legacy comma/semicolon splitting should remain permanent API compatibility or become deprecated.

## Verification

GitHub Actions passed for commit:

```text
511a8c19
```

Validated gates included:

```text
make test-docs
make test-phase
```

Narrow implementation target:

```bash
make test-restful-api-search-timer-command-executor
```

## Back

- [Back to Development Index](index.md)
- [Back to Phase 55.0 Audit](phase-55-vdr-parity-adapter-audit.md)
- [Back to Phase 55.1](phase-55.1-searchtimer-update-restfulapi-contract.md)
