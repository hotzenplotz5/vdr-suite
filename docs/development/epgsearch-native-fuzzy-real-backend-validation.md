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
    PASS: native fuzzy real-backend validation completed

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
