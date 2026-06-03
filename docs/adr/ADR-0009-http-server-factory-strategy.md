# ADR-0009: HTTP Server Factory Strategy

## Status

Accepted

## Context

VDR-Suite already uses a factory pattern for VDR backend selection.

Existing VDR backend architecture:

RuntimeConfig
        ↓
VdrConfig
        ↓
VdrAdapterFactory
        ↓
IVdrAdapter

The HTTP server side now has an inbound server abstraction:

IHttpServer
        ↑
   +-------------+
   |             |
TestHttpServer   RealHttpServer

As of Phase 8.33, DaemonRuntime owns an IHttpServer instance.

As of Phase 8.34, RealHttpServer is documented as the future production implementation.

However, DaemonRuntime should not permanently decide directly which concrete IHttpServer implementation is created.

## Decision

HTTP server creation will be separated behind a future HttpServerFactory.

Target architecture:

RuntimeConfig
        ↓
HttpServerConfig
        ↓
HttpServerFactory
        ↓
IHttpServer

DaemonRuntime should eventually request an IHttpServer from HttpServerFactory instead of directly constructing TestHttpServer or RealHttpServer.

HttpServerFactory will be responsible for selecting the concrete implementation.

Possible future implementations:

- TestHttpServer
- RealHttpServer

ApiRouter remains the routing boundary.

Controllers, services, repositories and VDR adapters remain independent from HTTP server selection.

## Consequences

Benefits:

- DaemonRuntime stays independent from concrete HTTP server implementations
- TestHttpServer and RealHttpServer can be selected through one boundary
- future HTTP server replacement remains possible
- the HTTP side becomes symmetric with the VDR adapter architecture
- RealHttpServer can be introduced later without changing controllers or services

Trade-offs:

- additional factory layer
- HttpServerConfig will be needed before implementation
- lifecycle ownership must remain explicit

## Non-Goals

This ADR does not implement HttpServerFactory.

This ADR does not implement HttpServerConfig.

This ADR does not implement RealHttpServer.

This ADR does not introduce:

- TCP sockets
- bind()
- listen()
- accept()
- TLS
- threading
- HTTP parsing
- HTTP library selection

## Follow-Up Work

Future phases should introduce:

- HttpServerConfig
- HttpServerFactory
- DaemonRuntime integration with HttpServerFactory

The next implementation phase should begin with HttpServerConfig.
