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
## Safety behavior

By default, the created SearchTimer is deleted at the end of the validation run.

Use `--keep-created` only when you intentionally want to inspect the temporary SearchTimer in VDR after the test.

## Failure interpretation

If the create request fails, the backend may not expose the expected SearchTimer write endpoint.

If readback does not show `mode=5`, the backend may normalize or ignore native fuzzy mode.

If readback does not show the requested tolerance, the backend may ignore native fuzzy tolerance.

If cleanup fails, manually delete the temporary SearchTimer named with the configured validation prefix.

## Phase status

Phase 49.14 adds the safe validation harness and runbook. It does not yet make runtime capability decisions based on live validation results.

## Back

Back to [Development Index](index.md).
