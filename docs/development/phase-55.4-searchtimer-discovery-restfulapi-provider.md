# Phase 55.4 - RESTfulAPI SearchTimer Discovery Provider

## Navigation

- [Development Index](index.md)
- [Phase 55.0 - VDR-Suite Feature-Parity and Adapter Audit](phase-55-vdr-parity-adapter-audit.md)
- [Phase 55.1 - SearchTimer Update RESTfulAPI Contract Fix](phase-55.1-searchtimer-update-restfulapi-contract.md)
- [Phase 55.2 - SearchTimer JSON Array Payload Parity](phase-55.2-searchtimer-json-array-payload-parity.md)
- [Phase 55.3 - Timer Form Body URL Encoding](phase-55.3-timer-form-body-url-encoding.md)
- [Current Project Status](current-status.md)

---

## Status

Implemented. Awaiting GitHub Actions verification.

## Purpose

Phase 55.0 identified that VDR-Suite already had the SearchTimer discovery domain, service, controller and router route, but the RESTfulAPI provider still behaved as a placeholder.

Before this phase, `RestfulApiSearchTimerDiscoveryProvider::discover()` only returned an empty catalog with the selected backend id.

That meant the public VDR-Suite discovery API could exist while still not exposing the actual RESTfulAPI option catalogs required for advanced SearchTimer editing.

## RESTfulAPI endpoints covered

The provider now fetches these RESTfulAPI endpoints when constructed with an `IHttpClient`:

```text
GET /searchtimers/channelgroups.json
GET /searchtimers/recordingdirs.json
GET /searchtimers/blacklists.json
GET /searchtimers/extepginfo.json
```

Each request sends:

```text
Accept: application/json
```

The provider also supports a configured base path, for example `/api/searchtimers/channelgroups.json`.

## Mapping

The provider maps RESTfulAPI responses into `SearchTimerDiscoveryCatalog`:

```text
channelgroups.json  -> channelGroups
recordingdirs.json  -> recordingDirectories
blacklists.json     -> blacklists
extepginfo.json     -> extendedEpgInfo
```

The provider keeps the old constructor behaviour: if no HTTP client is configured, discovery returns an empty catalog. This preserves earlier tests and keeps static/offline usage safe.

## Real RESTfulAPI smoke result

A real yaVDR RESTfulAPI smoke check against `127.0.0.1:8002` confirmed that all four endpoints are available.

Observed shape:

```text
/searchtimers/channelgroups.json -> {"groups":[],"count":0,"total":0}
/searchtimers/recordingdirs.json -> {"dirs":[...],"count":50,"total":50}
/searchtimers/blacklists.json    -> {"blacklists":[],"count":0,"total":0}
/searchtimers/extepginfo.json    -> {"ext_epg_info":[],"count":0,"total":0}
```

The real smoke output also exposed JSON Unicode escapes in recording directory names, for example `\u00f6`, `\u00fc` and `\u00df`.

The provider now decodes JSON Unicode escapes into UTF-8 before storing catalog values.

## Changed files

```text
core/vdr/include/RestfulApiSearchTimerDiscoveryProvider.h
core/vdr/src/RestfulApiSearchTimerDiscoveryProvider.cpp
core/vdr/tests/test_restfulapi_search_timer_discovery_provider_contract.cpp
```

## Regression coverage

The existing target is extended:

```text
test-restfulapi-search-timer-discovery-provider-contract
```

New coverage:

- old no-client constructor still returns an empty catalog
- custom legacy `discoveryEndpoint()` value is preserved
- HTTP-backed provider performs exactly four GET requests
- requested endpoints are:
  - `/searchtimers/channelgroups.json`
  - `/searchtimers/recordingdirs.json`
  - `/searchtimers/blacklists.json`
  - `/searchtimers/extepginfo.json`
- base path joining is verified with `/api`
- channel groups are mapped
- recording directories are mapped
- blacklists with `id` and `search` are mapped
- extended EPG info with `id`, `name`, `values` and `config` is mapped
- JSON Unicode escapes such as `\u00f6`, `\u00fc` and `\u00df` are decoded into UTF-8

## Remaining follow-up

This phase completes the RESTfulAPI provider contract itself.

Potential follow-up work:

- Verify daemon/runtime wiring uses the HTTP-backed provider rather than the no-client constructor.
- Add real yaVDR smoke validation for all four discovery endpoints through VDR-Suite runtime once wired.
- Decide whether conflicts belong into discovery or a separate operational status API.

## Verification

Real RESTfulAPI smoke check completed against:

```text
http://127.0.0.1:8002/searchtimers/channelgroups.json
http://127.0.0.1:8002/searchtimers/recordingdirs.json
http://127.0.0.1:8002/searchtimers/blacklists.json
http://127.0.0.1:8002/searchtimers/extepginfo.json
```

Expected GitHub Actions/local narrow target:

```bash
make test-restfulapi-search-timer-discovery-provider-contract
```

Expected documentation gates:

```bash
make test-docs
make test-phase
```

## Back

- [Back to Development Index](index.md)
- [Back to Phase 55.0 Audit](phase-55-vdr-parity-adapter-audit.md)
- [Back to Phase 55.3](phase-55.3-timer-form-body-url-encoding.md)
