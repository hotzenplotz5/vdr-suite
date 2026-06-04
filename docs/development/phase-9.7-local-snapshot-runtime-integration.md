# Phase 9.7 – Local Snapshot Runtime Integration Test

## Purpose

Phase 9.7 introduces the first optional integration test against a real local VDR/RESTfulAPI setup through the complete snapshot runtime path.

The goal is to verify that the snapshot runtime can operate with the real RESTfulAPI adapter and production HTTP client, while keeping the normal unit-test suite independent from a local VDR installation.

---

## Verified Runtime Path

The test validates this path:

```text
VDR
    ↓
RESTfulAPI
    ↓
BasicHttpClient
    ↓
RestfulApiVdrAdapter
    ↓
VdrService
    ↓
VdrSnapshotBuilder
    ↓
PollingService
    ↓
SnapshotCacheService
    ↓
SnapshotCache
```

This is the first committed test that proves VDR-Suite can build and retain a real snapshot through the polling runtime using a local RESTfulAPI endpoint.

---

## Test Target

The optional target is:

```text
make test-local-snapshot-runtime-integration
```

It is intentionally not part of:

```text
make test
```

Reason:

- unit tests must stay independent from a local VDR installation
- CI should not require a running VDR/RESTfulAPI instance
- developers can run the integration test explicitly on a configured VDR host

---

## Default Endpoint

Default local endpoint:

```text
127.0.0.1:8002
```

Override variables:

```text
VDR_SUITE_LOCAL_RESTFULAPI_HOST
VDR_SUITE_LOCAL_RESTFULAPI_PORT
```

Example:

```text
VDR_SUITE_LOCAL_RESTFULAPI_HOST=127.0.0.1 \
VDR_SUITE_LOCAL_RESTFULAPI_PORT=8002 \
make test-local-snapshot-runtime-integration
```

---

## Verified Runtime Behavior

The integration test verifies:

- `PollingService::poll()` can run against a real local RESTfulAPI adapter
- the first poll builds a snapshot through `VdrSnapshotBuilder::buildSnapshot()`
- `SnapshotCache` contains a snapshot after polling
- the snapshot contains a valid RESTfulAPI status
- the snapshot contains real local VDR channels
- a second immediate poll with unchanged `change-state.json` does not create refresh work
- the cached domain counts remain stable across the second poll

---

## Observed Local Result

The first successful local run reported:

```text
Running local snapshot runtime integration test against 127.0.0.1:8002
Local snapshot runtime integration test passed
Channels: 342
Recordings: 973
Timers: 0
Events: 36299
```

---

## Architecture Boundary

Phase 9.7 validates the real snapshot runtime path.

It does not yet prove that a real VDR domain change triggers only the matching partial refresh domain.

That is intentionally deferred to Phase 9.8.

---

## Next Phase

Phase 9.8 should introduce an optional local partial-refresh runtime integration test:

```text
real VDR domain change
    ↓
/change-state.json version change
    ↓
PollingService
    ↓
ChangeDetectionService
    ↓
SnapshotRefreshPlanner
    ↓
SnapshotUpdatePlan
    ↓
selected domain refresh only
```

That phase should verify the core optimization behind the Phase 9 architecture against a real local VDR/RESTfulAPI setup.

---

## Status

Phase 9.7 is complete.

Milestone tag:

```text
v2.7-phase9-local-snapshot-runtime-integration
```
