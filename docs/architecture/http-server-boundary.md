# HTTP Server Boundary Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

## Purpose

This document defines the inbound HTTP server boundary for VDR-Suite.

The goal is to expose the existing REST API runtime to external clients without coupling the application core to a specific networking implementation.

Possible clients:

* Windows frontend
* Future web frontend
* Mobile applications
* External integrations

---

## Current REST Runtime

The internal REST runtime already exists.

    DaemonRuntime
            │
            ▼
    ApiRouter
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
    Services
            │
            ▼
    Repositories / VDR adapters

Implemented routes:

    GET /api/dashboard
    GET /api/jobs
    GET /api/recordings
    GET /api/metadata
    GET /api/vdr/overview

---

## Inbound HTTP Boundary

Phase 8.28 introduced the inbound HTTP server contract:

    HttpServerRequest
    HttpServerResponse
    IHttpServer

This boundary is for requests coming into VDR-Suite.

It is separate from the outbound HTTP client boundary:

    HttpRequest
    HttpResponse
    IHttpClient
    MockHttpClient

Outbound HTTP is used for communication from VDR-Suite to external systems such as vdr-plugin-restfulapi.

Inbound HTTP is used for communication from clients to VDR-Suite.

The two boundaries must remain separate.

---

## Target Architecture

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
    Controllers
            │
            ▼
    Services
            │
            ▼
    HttpServerResponse
            │
            ▼
    Client

IHttpServer is the transport boundary.

ApiRouter remains the application route dispatcher.

---

## IHttpServer Responsibilities

An IHttpServer implementation is responsible for:

* receiving inbound HTTP-like requests
* converting transport-specific requests into HttpServerRequest
* forwarding requests to the REST runtime
* converting application responses into HttpServerResponse
* hiding transport details from the application core

---

## IHttpServer Must Not

IHttpServer implementations must not:

* contain business logic
* access repositories directly
* call VDR adapters directly
* call RESTfulAPI directly
* create JSON manually
* duplicate ApiRouter route logic
* decide application-level authorization rules

---

## ApiRouter Responsibilities

ApiRouter remains responsible for route dispatching.

It maps paths to controllers.

Current examples:

    /api/dashboard      -> DashboardController
    /api/jobs           -> JobsController
    /api/recordings     -> RecordingsController
    /api/metadata       -> MetadataController
    /api/vdr/overview   -> VdrController

The router must remain independent of:

* sockets
* TCP
* TLS
* threading
* web server libraries
* frontend technologies

---

## Future Implementations

Possible future server implementations:

    EmbeddedHttpServer
    UnixSocketServer
    WindowsNamedPipeServer
    ReverseProxyBridge
    TestHttpServer

All implementations must depend on the same inbound HTTP contract.

---

## Out of Scope

This architecture document does not implement:

* TCP sockets
* bind()
* listen()
* accept()
* threading
* TLS
* authentication
* authorization
* static file serving
* WebSocket support
* real HTTP parsing
* HTTP library selection

---

## Design Goals

* Keep REST runtime transport-independent
* Keep ApiRouter independent from networking
* Allow multiple frontend technologies
* Allow future Windows service integration
* Keep testing possible without sockets
* Avoid early dependency on a concrete HTTP library
---

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
