# REST API Runtime Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

## Overview

The REST API layer is responsible for exposing VDR-Suite functionality to external clients.

Examples:

* Windows frontend
* Future web frontend
* Mobile applications
* External integrations

The REST API layer must remain independent from specific frontend implementations.

---

## Runtime Ownership

The daemon runtime owns all REST API components.

```text
DaemonRuntime
        │
        ▼
Database
        │
        ▼
Repositories
    ├── JobRepository
    ├── RecordingRepository
    └── MetadataRepository
        │
        ▼
Services
    ├── JobDashboardService
    ├── RecordingDashboardService
    ├── VdrService
    └── VdrOverviewService
        │
        ▼
Facades
    └── DashboardFacade
        │
        ▼
Serializers
    ├── DashboardJsonSerializer
    └── VdrOverviewJsonSerializer
        │
        ▼
Controllers
    ├── DashboardController
    ├── JobsController
    ├── RecordingsController
    ├── MetadataController
    └── VdrController
        │
        ▼
ApiRouter
```

---

## Controller Responsibilities

Controllers expose application functionality through REST endpoints.

Controllers must:

* remain lightweight
* avoid business logic
* delegate to services
* delegate serialization

Controllers must not:

* access databases directly
* perform VDR communication directly
* contain backend-specific logic

---

## ApiRouter Responsibilities

ApiRouter provides route dispatching.

Implemented routes:

```text
GET /api/dashboard
GET /api/jobs
GET /api/recordings
GET /api/metadata
GET /api/vdr/overview
```

ApiRouter selects the responsible controller.

Business logic must remain outside the router.

---

## SearchTimer Preview API Contract

SearchTimer preview routes:

```text
GET /api/searchtimers/preview
GET /api/vdr/searchtimers/preview
```

Relevant query parameters:

```text
backend=<backend-id>
query=<search-text>
text=<search-text fallback>
name=<display-name>
limit=<max-results>
offset=<result-offset>
```

The preview response contains three important top-level sections:

```text
searchTimer
preview
statistics
epgInput
```

The `epgInput` object describes whether the EPG input used by the preview is authoritative:

```json
{
  "epgInput": {
    "status": "ready",
    "available": true,
    "warnings": []
  }
}
```

Supported `epgInput.status` values:

```text
ready
warming
stale
unknown
unavailable
```

Client semantics:

* `ready` with `available=true` means the preview input is authoritative.
* `ready` with zero returned matches is a valid zero-result preview.
* `warming`, `stale`, `unknown` and `unavailable` with `available=false` mean the preview input is not authoritative.
* Non-authoritative preview results must not be presented as a final valid "0 matches" result.
* Clients should surface `warnings` when `available=false`.

This contract prevents a missing or warming warm EPG cache from being hidden as a normal empty SearchTimer preview result.

---

## VDR Integration

```text
RestfulApiVdrAdapter
        │
        ▼
VdrService
        │
        ▼
VdrOverviewService
        │
        ▼
VdrOverviewJsonSerializer
        │
        ▼
VdrController
        │
        ▼
ApiRouter
```

The REST API layer remains backend-neutral.

Controllers must never communicate directly with RESTfulAPI.

---

## Future HTTP Boundary

Future architecture:

```text
HttpServer
        │
        ▼
ApiRouter
        │
        ▼
Controllers
```

The HTTP server implementation must remain replaceable.

Possible future implementations:

* Embedded HTTP server
* Reverse proxy integration
* Unix socket transport
* Windows service transport

---

## Design Goals

* Frontend neutrality
* Backend neutrality
* Testability
* Runtime ownership clarity
* Future HTTP server flexibility
* Separation of transport and business logic

---

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
