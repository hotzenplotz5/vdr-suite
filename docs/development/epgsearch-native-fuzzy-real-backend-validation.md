# EPGSearch Native Fuzzy Real-Backend Validation

This document describes the real-backend validation harness introduced in Phase 49.14.

## Navigation

- [Development Index](index.md)
- [EPGSearch Capability Matrix](epgsearch-capability-matrix.md)
- [EPGSearch Test Coverage Audit](epgsearch-test-coverage-audit.md)
- [Current Status](current-status.md)
- [Completed Phases](completed-phases.md)

## Purpose

The validator checks whether a real RESTfulAPI/epgsearch backend accepts and returns native fuzzy SearchTimer configuration:

- native epgsearch mode `5`
- native fuzzy `tolerance`
- SearchTimer create path
- SearchTimer readback path
- SearchTimer cleanup path

The validator is intentionally safe by default. Without `--execute`, it prints the request payload and does not send anything to the backend.

## Dry run

    cd /home/yavdr/vdr-suite

    python3 tools/validate_real_epgsearch_native_fuzzy.py \
      --host 127.0.0.1 \
      --port 8002

## Real validation

This creates one temporary SearchTimer, reads it back, verifies `mode=5` and `tolerance=<int>`, and deletes it again.

    cd /home/yavdr/vdr-suite

    python3 tools/validate_real_epgsearch_native_fuzzy.py \
      --host 127.0.0.1 \
      --port 8002 \
      --tolerance 2 \
      --execute

## Authentication

If the RESTfulAPI endpoint is protected by HTTP basic authentication:

    cd /home/yavdr/vdr-suite

    python3 tools/validate_real_epgsearch_native_fuzzy.py \
      --host 127.0.0.1 \
      --port 8002 \
      --user USERNAME \
      --password PASSWORD \
      --execute

## Expected success output

A successful validation includes:

    PASS: readback mode=5
    PASS: readback tolerance=2
    capabilityProbe={"cleanupSucceeded": true, "createAccepted": true, "modePreserved": true, "readbackAvailable": true, "tolerancePreserved": true}
    capability: epg.search.fuzzy.native=true
    PASS: native fuzzy real-backend validation completed

## Capability signal

A successful execution emits a deterministic capability signal:

    capability: epg.search.fuzzy.native=true

This signal is valid only when the complete validation lifecycle succeeded:

- create accepted
- readback available
- native fuzzy mode preserved
- tolerance preserved
- cleanup succeeded

The C++ capability detector introduced in Phase 49.15 maps this probe result onto `VdrCapabilitySet::epgSearchFuzzyNative`.
## Runtime capability wiring

Phase 49.16 wires the successful native fuzzy probe result into backend runtime capability state.

The runtime wiring does not run an automatic VDR mutation probe by itself.

The intended flow is:

    real validation succeeds
    capability probe result is complete
    detector computes native fuzzy capability
    BackendRegistryService updates the backend capability set
    capability report can expose epg.search.fuzzy.native as available

A missing backend is never created by a capability update.
## Capability persistence

Phase 49.17 adds persistence for native fuzzy capability probe results.

Persisted probe results are stored per backend id in SQLite.

Persisted values are not treated as an automatic permission to run a new probe.

The intended flow is:

    real validation succeeds
    capability probe result is persisted for the backend id
    a later runtime phase can reload the persisted result
    the detector can recompute epg.search.fuzzy.native
    backend runtime capabilities can be updated without repeating the mutation probe

A failed or incomplete probe may also be persisted and must keep native fuzzy disabled when reloaded.
## Persisted capability restore

Phase 49.18 restores persisted native fuzzy capability probe results into runtime backend capabilities.

Restore is backend-scoped and safe:

    load persisted probe result by backend id
    compute native fuzzy availability using the detector
    update only an existing backend capability set
    do not create missing backends
    do not execute a new VDR mutation probe

A persisted failed or incomplete probe restores `epg.search.fuzzy.native=false`.
## Startup restore integration

Phase 49.19 integrates persisted native fuzzy capability restore into daemon startup.

Startup restore is intentionally non-mutating:

    open database
    create backend registry
    assign baseline snapshot-read capabilities
    load persisted native fuzzy probe results
    update existing backend capability state
    build the capability report from the restored backend capability set

No SearchTimer is created, modified or deleted during startup restore.
## Restore diagnostics

Phase 49.20 adds structured diagnostics for persisted native fuzzy startup restore.

Diagnostics expose:

    schemaReady
    backendsSeen
    persistedResultsFound
    backendsUpdated
    nativeFuzzyAvailable
    nativeFuzzyUnavailable
    status
    reason

The daemon also records a runtime diagnostics measurement:

    component=epgsearch-native-fuzzy
    operation=startup-restore
    statusCode=200 when restore was processed
    statusCode=204 when no persisted result was available
    statusCode=503 when the persistence schema was unavailable
    itemCount=persistedResultsFound

This keeps startup restore observable without running a new SearchTimer mutation probe.
## Restore freshness policy

Phase 49.21 adds a freshness policy for persisted native fuzzy probe results.

Default policy:

    maxAgeSeconds = 604800
    maxAge = 7 days

A persisted result is trusted only when its SQLite `updated_at` age is within the freshness window.

Fresh result:

    may restore epg.search.fuzzy.native=true when the detector result is complete

Stale result:

    is counted as persisted
    is counted as stale ignored
    must not enable epg.search.fuzzy.native
    forces epg.search.fuzzy.native=false for the existing backend
    does not create or mutate any SearchTimer probe object

## Stale probe administration

Phase 49.22 adds a read-only/non-mutating administration service for stale persisted native fuzzy probe results.

Administration supports:

    list stale persisted probe results
    delete stale persisted probe results
    keep fresh persisted probe results untouched
    treat future timestamps as untrusted/stale
    create the persistence schema safely when missing

This administration service does not contact VDR and does not create, modify or delete SearchTimer objects.

## Stale probe administration API

Phase 49.23 exposes stale native fuzzy probe administration through REST.

Endpoints:

    GET  /api/vdr/epgsearch/native-fuzzy/stale-probes
    GET  /api/epgsearch/native-fuzzy/stale-probes
    POST /api/vdr/epgsearch/native-fuzzy/stale-probes/delete
    POST /api/epgsearch/native-fuzzy/stale-probes/delete

GET returns stale or future-timestamp persisted probe rows.

POST delete removes only stale or future-timestamp persisted rows and keeps fresh rows untouched.

The API does not contact VDR and does not create, modify or delete SearchTimer objects.

## Operator refresh API

Phase 49.25 exposes the operator-triggered native fuzzy refresh workflow through REST.

Endpoints:

    POST /api/vdr/epgsearch/native-fuzzy/refresh
    POST /api/epgsearch/native-fuzzy/refresh

Accepted JSON fields:

    backend or backendId
    query or probeQuery
    tolerance
    keepProbeSearchTimer
    updateBackendCapabilities

The endpoint intentionally performs a real temporary SearchTimer probe.

It is not executed automatically during startup.

## Operator refresh workflow

Phase 49.24 adds a core service for an operator-triggered native fuzzy refresh workflow.

The workflow performs an explicit temporary SearchTimer probe:

    create temporary SearchTimer with native fuzzy mode=5
    read back the temporary SearchTimer
    verify preserved mode and tolerance
    delete the temporary SearchTimer
    persist the resulting native fuzzy capability probe
    update backend capability state

This is intentionally not run during daemon startup.

A failed probe is persisted as an unavailable native fuzzy capability and updates the backend capability to false.

The workflow is prepared for a later REST/operator endpoint.

## Operator refresh operational validation

Phase 49.27 validates the operator refresh workflow against a real yaVDR backend through VDR-Suite itself.

The validated path is:

    POST /api/epgsearch/native-fuzzy/refresh

The validation confirmed:

    create temporary native fuzzy SearchTimer
    RESTfulAPI returns HTTP 200 with an empty create body
    VDR-Suite falls back to /searchtimers.json readback
    exact probe query resolves the created backend-native id
    readback preserves mode=5
    readback preserves tolerance=2
    cleanup deletes the temporary SearchTimer
    probe result is persisted
    backend capability state is updated
    epg.search.fuzzy.native=true is reported by the refresh result

The operational helper is:

    tools/validate_vdr_suite_native_fuzzy_operator_refresh.py

Safe dry-run:

    python3 tools/validate_vdr_suite_native_fuzzy_operator_refresh.py

Real execution:

    python3 tools/validate_vdr_suite_native_fuzzy_operator_refresh.py --execute --unique-query

The helper defaults to the daemon endpoint:

    http://127.0.0.1:18080/api/epgsearch/native-fuzzy/refresh

The RESTfulAPI empty-body create response is now part of the supported behavior. The command executor treats HTTP 200 without an Id body as accepted only when a unique created SearchTimer can be read back by the exact probe query.


## Capability report validation

Phase 49.28 validates that the native fuzzy operator refresh result is visible through the public VDR-Suite capability report.

The validated capability report endpoint is:

    GET /api/vdr/capabilities

The validated capability is:

    epg.search.fuzzy.native

The validation sequence is:

    run operator refresh through /api/epgsearch/native-fuzzy/refresh
    update backend capability state from the successful probe result
    read /api/vdr/capabilities
    verify epg.search.fuzzy.native supported=true
    verify epg.search.fuzzy.native availability=available
    verify epg.search.fuzzy.native availableNow=true

The helper is:

    tools/validate_vdr_suite_native_fuzzy_capability_report.py

Safe dry-run:

    python3 tools/validate_vdr_suite_native_fuzzy_capability_report.py

Real execution:

    python3 tools/validate_vdr_suite_native_fuzzy_capability_report.py --execute

The daemon capability report now resolves capability state dynamically from the backend registry. This prevents the report from staying stale after an operator-triggered native fuzzy refresh updates backend capability state.


## Persisted restore validation

Phase 49.29 validates that the native fuzzy capability survives a daemon restart.

The validated sequence is:

    run operator refresh through /api/epgsearch/native-fuzzy/refresh
    persist the successful native fuzzy probe result in SQLite
    stop the daemon
    restart the daemon with the same database
    verify startup restore diagnostics
    verify /api/vdr/capabilities still reports native fuzzy support

The startup restore runtime measurement is exposed through:

    GET /api/runtime/diagnostics

The relevant runtime measurement is:

    component=epgsearch-native-fuzzy
    operation=startup-restore
    statusCode=200
    itemCount>=1

The public capability report is checked through:

    GET /api/vdr/capabilities

The validated capability state is:

    epg.search.fuzzy.native supported=true
    epg.search.fuzzy.native availability=available
    epg.search.fuzzy.native availableNow=true

The helper is:

    tools/validate_vdr_suite_native_fuzzy_persisted_restore.py

Safe dry-run:

    python3 tools/validate_vdr_suite_native_fuzzy_persisted_restore.py

Real execution after daemon restart:

    python3 tools/validate_vdr_suite_native_fuzzy_persisted_restore.py --execute

This proves that native fuzzy support is not only detected during the current daemon lifetime, but also restored from persisted probe state on the next daemon startup.


## Safety behavior

By default, the created SearchTimer is deleted at the end of the validation run.

Use `--keep-created` only when you intentionally want to inspect the temporary SearchTimer in VDR after the test.

## Failure interpretation

If the create request fails, the backend may not expose the expected SearchTimer write endpoint.

If readback does not show `mode=5`, the backend may normalize or ignore native fuzzy mode.

If readback does not show the requested tolerance, the backend may ignore native fuzzy tolerance.

If cleanup fails, manually delete the temporary SearchTimer named with the configured validation prefix.

## Phase status

Phase 49.29 validates that a persisted native fuzzy capability probe result is restored after daemon restart and remains visible through /api/vdr/capabilities.

## Back

Back to [Development Index](index.md).
