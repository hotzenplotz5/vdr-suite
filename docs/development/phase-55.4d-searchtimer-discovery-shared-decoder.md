# Phase 55.4d - SearchTimer Discovery Shared Decoder Cleanup

## Navigation

- [Development Index](index.md)
- [Phase 55.4 - RESTfulAPI SearchTimer Discovery Provider](phase-55.4-searchtimer-discovery-restfulapi-provider.md)
- [Phase 55.4b - Shared JSON String Decoder](phase-55.4b-shared-json-string-decoder.md)
- [Phase 55.4c - SearchTimer Discovery Runtime Wiring](phase-55.4c-searchtimer-discovery-runtime-wiring.md)
- [Current Project Status](current-status.md)

---

## Status

Implemented. Awaiting GitHub Actions verification.

## Purpose

Phase 55.4 introduced JSON escape decoding inside the RESTfulAPI SearchTimer discovery provider.

Phase 55.4b later introduced the shared `JsonStringDecoder` utility for RESTfulAPI mapper code.

This phase removes the duplicated local decoder logic from `RestfulApiSearchTimerDiscoveryProvider.cpp` and routes discovery string decoding through the shared decoder.

## Behaviour

The discovery provider continues to decode JSON string escapes for channel groups, recording directories, blacklists and Extended EPG info.

The provider now preserves raw JSON escape sequences while parsing string boundaries and delegates the actual escape decoding to `vdrsuite::decodeJsonStringEscapes`.

The external API contract and discovery JSON response shape remain unchanged.

## Changed files

```text
core/vdr/src/RestfulApiSearchTimerDiscoveryProvider.cpp
docs/development/phase-55.4d-searchtimer-discovery-shared-decoder.md
docs/development/index.md
```

## Verification coverage

Expected local checks:

```bash
make test-restfulapi-search-timer-discovery-provider-contract
make test-json-string-decoder
make test-vdr
make test-docs
make test-phase
make daemon
```

## Back

- [Back to Development Index](index.md)
- [Back to Phase 55.4](phase-55.4-searchtimer-discovery-restfulapi-provider.md)
- [Back to Phase 55.4b](phase-55.4b-shared-json-string-decoder.md)
- [Back to Phase 55.4c](phase-55.4c-searchtimer-discovery-runtime-wiring.md)
