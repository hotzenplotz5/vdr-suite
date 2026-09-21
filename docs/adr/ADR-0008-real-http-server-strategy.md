# ADR-0008: Real HTTP Server Strategy

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)

---

## Status

Accepted

## Context

VDR-Suite has an inbound HTTP server boundary.

Implemented components:

- HttpServerRequest
- HttpServerResponse
- IHttpServer
- TestHttpServer

TestHttpServer is an in-memory implementation of IHttpServer.

It validates the REST request lifecycle without sockets, networking, threading or a real HTTP stack.

As of Phase 8.33, DaemonRuntime owns an IHttpServer instance.

Current runtime chain:

DaemonRuntime
        ↓
IHttpServer
        ↓
TestHttpServer
        ↓
ApiRouter
        ↓
Controllers
        ↓
Services
        ↓
Repositories / VDR

Future production deployments require a networking-capable HTTP server implementation.

That future implementation must not change ApiRouter, controllers, services, repositories or VDR adapter code.

## Decision

A future RealHttpServer will be introduced as a production implementation of IHttpServer.

Target implementation model:

IHttpServer
        ↑
   +-------------+
   |             |
TestHttpServer   RealHttpServer

DaemonRuntime must depend on IHttpServer.

ApiRouter remains the application routing boundary.

HTTP server implementations may forward requests to ApiRouter.

ApiRouter must not know whether requests came from TestHttpServer, RealHttpServer or another future server implementation.

## Responsibilities

TestHttpServer is responsible for:

- in-memory request handling
- integration testing
- request lifecycle validation
- ApiResponse to HttpServerResponse mapping
- avoiding networking dependencies

TestHttpServer must not:

- open sockets
- listen on ports
- parse real HTTP traffic
- contain business logic

RealHttpServer will later be responsible for:

- accepting real client connections
- converting transport requests into HttpServerRequest
- forwarding requests to ApiRouter
- converting HttpServerResponse into transport responses
- hiding networking details from the application core

RealHttpServer must not:

- access repositories directly
- call services directly
- call VDR adapters directly
- duplicate ApiRouter route logic
- contain business logic
- expose HTTP library specific types to controllers or services

## Consequences

Benefits:

- production HTTP support can be added later without changing controllers
- tests remain possible without sockets
- DaemonRuntime stays implementation-independent
- HTTP library choice can be postponed
- future replacement of the HTTP stack remains possible

Trade-offs:

- one additional abstraction layer
- lifecycle handling must remain explicit
- request and response mapping must be maintained carefully

## Non-Goals

This ADR does not implement RealHttpServer.

This ADR does not choose an HTTP library.

This ADR does not introduce:

- TCP sockets
- bind()
- listen()
- accept()
- TLS
- threading
- epoll
- HTTP parsing
- authentication
- authorization
- WebSocket support
- static file serving

## Follow-Up Work

Future phases may introduce:

- RealHttpServer
- RealHttpServerConfig
- HttpServerFactory
- DaemonRuntime HTTP server selection
- basic GET request parsing
- local-only development listener

Any future implementation must preserve this boundary:

DaemonRuntime
        ↓
IHttpServer
        ↓
ApiRouter
        ↓
Controllers
        ↓
Services
        ↓
Repositories / VDR
---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
