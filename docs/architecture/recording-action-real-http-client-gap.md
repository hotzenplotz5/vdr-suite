# Recording Action Real HTTP Client Gap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

## Purpose

Phase 40.2 documents the remaining gap between the RESTfulAPI recording action executor and a real VDR backend smoke test.

The recording action executor is already transport-oriented through IHttpClient.

However, the repository currently has only MockHttpClient for tests.

A real network HTTP client is still required before VDR-Suite can contact a real RESTfulAPI endpoint.

---

## Current State

The current stack is:

1. RecordingActionExecutionController
2. RecordingActionExecutionService
3. RecordingActionBackendExecutorAdapterRegistry
4. RestfulApiRecordingActionBackendExecutorAdapter
5. IHttpClient
6. MockHttpClient in tests

This proves request construction and response handling.

It does not yet prove real network connectivity.

---

## Confirmed Gap

A real HTTP implementation is missing.

Required future component:

- CurlHttpClient or equivalent real IHttpClient implementation

Required behavior:

- execute HttpRequest
- return HttpResponse
- support host, port, path, method and body
- handle connection failure
- handle HTTP status codes
- avoid mutation by default in smoke tests

---

## Safety Rule

The first real HTTP smoke test must remain non-mutating.

The default configuration must keep:

- readOnly=true
- allowExecution=false

Real mutation must not be enabled by default.

---

## Next Step

Phase 40.3 should introduce a minimal real HTTP client boundary or implementation.

Phase 40.4 may then add the first real RESTfulAPI connectivity smoke test.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Architecture Index](index.md)
