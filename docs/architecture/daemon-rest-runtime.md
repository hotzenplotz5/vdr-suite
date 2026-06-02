# Daemon REST Runtime Architecture

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
