# Phase 9.6 – Local RESTfulAPI Integration Test

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)

---

## Purpose

Phase 9.6 introduces the first optional integration test against a real local VDR/RESTfulAPI setup.

The goal is to verify the real adapter boundary without making the normal unit-test suite depend on a local VDR installation.

---

## Verified Local Runtime Path

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
VDR-Suite domain objects
```

This is the first committed test that proves VDR-Suite can communicate with a real local RESTfulAPI endpoint through the production HTTP client and adapter boundary.

---

## Test Target

The optional target is:

```text
make test-local-restfulapi-integration
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
make test-local-restfulapi-integration
```

---

## Verified Endpoints

The integration test checks:

```text
/info.json
/channels.json
/events.json
/recordings.json
/timers.json
/change-state.json
```

The `/change-state.json` response must contain:

```text
statusVersion
channelsVersion
recordingsVersion
timersVersion
eventsVersion
```

---

## Observed Local Result

The first successful local run reported:

```text
Running local RESTfulAPI integration test against 127.0.0.1:8002
Local RESTfulAPI integration test passed
Channels: 342
Change state: status=0, channels=2, recordings=0, timers=0, events=0
```

---

## Architecture Boundary

Phase 9.6 validates the real RESTfulAPI adapter boundary only.

It does not yet test the complete snapshot runtime path.

That is intentionally deferred to Phase 9.7.

---

## Next Phase

Phase 9.7 should introduce an optional local snapshot runtime integration test:

```text
RESTfulAPI
    ↓
RestfulApiVdrAdapter
    ↓
VdrService
    ↓
VdrSnapshotBuilder
    ↓
PollingService
    ↓
SnapshotCache
```

That phase should verify that the runtime snapshot path can run against a real local RESTfulAPI/VDR setup without mocks.

---

## Status

Phase 9.6 is complete.

Milestone tag:

```text
v2.6-phase9-local-restfulapi-integration
```
---

## Back

- [Back to Development Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
