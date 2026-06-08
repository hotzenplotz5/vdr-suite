# ADR-0007: RESTfulAPI Adapter Boundary

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)

---

## Status

Accepted

## Context

VDR-Suite should communicate with an existing VDR installation without duplicating functionality already provided by VDR plugins.

`vdr-plugin-restfulapi` already exposes many relevant VDR capabilities through HTTP endpoints, including status information, channels, events, recordings, timers, OSD, remote control, scraper images, Femon and Wirbelscan.

The current VDR-Suite architecture already contains an adapter boundary:

```text
VdrAdapterFactory
↓
IVdrAdapter
↓
VdrStatus
```

Existing implementations:

```text
ExternalVdrAdapter
MockVdrAdapter
```

RESTfulAPI integration introduces HTTP transport, JSON parsing, network errors and endpoint compatibility concerns.

## Decision

RESTfulAPI will be implemented as a future `IVdrAdapter` implementation.

```text
RestfulApiVdrAdapter : IVdrAdapter
```

RESTfulAPI must not be called directly from daemon, REST API controllers, dashboard services or recording services.

HTTP communication will be separated behind an `IHttpClient` abstraction.

```text
RestfulApiVdrAdapter
↓
IHttpClient
```

The RESTfulAPI adapter will map RESTfulAPI JSON responses into VDR-Suite domain objects.

RESTfulAPI JSON structures must not leak into higher service layers.

## Consequences

The daemon remains backend-independent.

The existing mock backend remains useful for tests.

RESTfulAPI integration can be tested without a running VDR instance by using a mock HTTP client.

Future backends such as SVDRP or plugin bridge integration can reuse the same `IVdrAdapter` boundary.

`VdrStatus` remains a small status object and must not become a container for channels, EPG, recordings, timers or OSD data.

## Non-Goals

This ADR does not implement HTTP communication.

This ADR does not implement JSON parsing.

This ADR does not add real RESTfulAPI network access.

This ADR does not define all future VDR domain models.

## Follow-Up Work

Future phases should introduce:

```text
IHttpClient
HttpRequest
HttpResponse
MockHttpClient
RestfulApiVdrAdapter
```

The first RESTfulAPI mapping target should be `/info.json` to `VdrStatus`.
---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
