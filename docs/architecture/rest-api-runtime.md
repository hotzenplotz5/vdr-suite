# REST API Runtime Architecture

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
