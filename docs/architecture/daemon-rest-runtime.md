# Daemon REST Runtime Architecture


> **Historical early daemon composition sketch.** Its “future IHttpServer”
> and “future request flow” headings describe design work before the current
> HTTP listener, security gate, runtime and API routes were implemented. This
> page is **not** a deployment or current DaemonRuntime wiring guide.
> Use [current architecture](../development/current-architecture-state.md),
> [Core runtime ownership](../../core/README.md) and
> [REST implementation boundary](../../api/rest/README.md) instead.

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

## Purpose

This document defines ownership and lifecycle of REST runtime components inside VDR-Suite.

The goal is to define how REST API components are created, owned, started and stopped by the daemon runtime.

No networking implementation is introduced by this document.

---

## Runtime Ownership

The daemon runtime owns all REST-related runtime components.

Current architecture:

    main()
            │
            ▼
    DaemonApp
            │
            ▼
    DaemonRuntime

DaemonRuntime is responsible for initialization, runtime ownership and shutdown.

---

## REST Runtime Ownership

The daemon runtime owns:

    Database

    Repositories
        ├── JobRepository
        ├── RecordingRepository
        └── MetadataRepository

    Services
        ├── JobDashboardService
        ├── RecordingDashboardService
        ├── VdrService
        └── VdrOverviewService

    Facades
        └── DashboardFacade

    Serializers
        ├── DashboardJsonSerializer
        └── VdrOverviewJsonSerializer

    Controllers
        ├── DashboardController
        ├── JobsController
        ├── RecordingsController
        ├── MetadataController
        └── VdrController

    ApiRouter

All components are created during initialization and destroyed during shutdown.

---

## Future HTTP Runtime Ownership

Future architecture:

    DaemonRuntime
            │
            ├── ApiRouter
            │
            └── IHttpServer

DaemonRuntime owns both components.

The HTTP server must not own the router.

The router must not own the HTTP server.

---

## Request Lifecycle

Future request flow:

    Client
            │
            ▼
    IHttpServer
            │
            ▼
    HttpServerRequest
            │
            ▼
    ApiRouter
            │
            ▼
    Controller
            │
            ▼
    Service
            │
            ▼
    Repository / VDR
            │
            ▼
    ApiResponse
            │
            ▼
    HttpServerResponse
            │
            ▼
    Client

---

## Dependency Direction

Allowed:

    IHttpServer
            │
            ▼
    ApiRouter
            │
            ▼
    Controllers
            │
            ▼
    Services

Not allowed:

    ApiRouter
            │
            ▼
    IHttpServer

The router must remain transport-independent.

---

## Runtime Lifecycle

Initialization:

    DaemonRuntime::initialize()
            │
            ├── Database
            ├── Repositories
            ├── Services
            ├── Controllers
            ├── ApiRouter
            └── IHttpServer (future)

Runtime:

    DaemonRuntime::run()

Shutdown:

    DaemonRuntime::shutdown()
            │
            ├── IHttpServer
            ├── ApiRouter
            ├── Controllers
            ├── Services
            ├── Repositories
            └── Database

---

## Out of Scope

This document does not introduce:

* TCP sockets
* bind()
* listen()
* accept()
* threading
* TLS
* authentication
* authorization
* HTTP parsing
* web server libraries
* network transports

---

## Design Goals

* Clear runtime ownership
* Clean shutdown order
* Transport independence
* Testability
* Future frontend support
* Future HTTP server flexibility
---

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
