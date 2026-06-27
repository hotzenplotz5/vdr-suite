# Phase 55.4b - Shared JSON String Decoder

## Navigation

- [Development Index](index.md)
- [Phase 55.4 - RESTfulAPI SearchTimer Discovery Provider](phase-55.4-searchtimer-discovery-restfulapi-provider.md)
- [Current Project Status](current-status.md)

---

## Status

Completed. GitHub Actions passed for commit `9f470076`.

## Purpose

This phase adds a shared JSON string decoder for RESTfulAPI mapper code and moves the existing mapper string extraction paths away from duplicated local decoding logic.

## Changed files

```text
core/vdr/include/JsonStringDecoder.h
core/vdr/src/RestfulApiChannelMapper.cpp
core/vdr/src/RestfulApiEventMapper.cpp
core/vdr/src/RestfulApiTimerMapper.cpp
core/vdr/src/RestfulApiRecordingMapper.cpp
core/vdr/src/RestfulApiSearchTimerMapper.cpp
core/vdr/tests/test_json_string_decoder.cpp
core/vdr/tests/test_restful_api_event_mapper.cpp
mk/local-test-groups.mk
```

## Regression coverage

New target:

```text
test-json-string-decoder
```

Additional mapper-level coverage:

```text
test-restful-api-event-mapper
```

Validated gates included:

```bash
make test-docs
make test-phase
make test-vdr
```

Final warning-free verification was recorded for commit `9f470076`.

## Back

- [Back to Development Index](index.md)
- [Back to Phase 55.4](phase-55.4-searchtimer-discovery-restfulapi-provider.md)
