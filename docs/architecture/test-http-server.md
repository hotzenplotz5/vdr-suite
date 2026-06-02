# Test HTTP Server Architecture

## Purpose

This document defines the architecture of TestHttpServer.

The goal is to validate the complete REST request lifecycle without introducing sockets, networking or a real HTTP server implementation.

TestHttpServer provides an in-memory transport layer for integration testing.

---

## Motivation

Current architecture:

    HttpServerRequest
    HttpServerResponse
    IHttpServer

    ApiRouter

    Controllers

The HTTP server boundary exists but no implementation currently exercises the complete request flow.

TestHttpServer fills this gap.

---

## Ownership

DaemonRuntime remains the owner of runtime components.

Future architecture:

    DaemonRuntime
            │
            ├── ApiRouter
            └── TestHttpServer

TestHttpServer must not own ApiRouter.

ApiRouter must not own TestHttpServer.

---

## Dependency Direction

Allowed:

    TestHttpServer
            │
            ▼
    ApiRouter
            │
            ▼
    Controllers

Not allowed:

    ApiRouter
            │
            ▼
    TestHttpServer

The router remains transport-independent.

---

## Request Lifecycle

Request flow:

    HttpServerRequest
            │
            ▼
    TestHttpServer
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
    ApiResponse
            │
            ▼
    HttpServerResponse

No networking layer exists in this flow.

All processing occurs in memory.

---

## ApiResponse Mapping

Controllers currently return ApiResponse.

TestHttpServer converts:

    ApiResponse
            │
            ▼
    HttpServerResponse

Mapped fields:

    statusCode
    contentType
    body

Transport details remain inside TestHttpServer.

---

## Supported Methods

Initial scope:

    GET

Supported routes:

    /api/dashboard
    /api/jobs
    /api/recordings
    /api/metadata
    /api/vdr/overview

POST, PUT and DELETE remain future work.

---

## Test Strategy

Primary purpose:

    End-to-end request testing

Example:

    HttpServerRequest
            │
            ▼
    TestHttpServer
            │
            ▼
    ApiRouter
            │
            ▼
    DashboardController
            │
            ▼
    DashboardJsonSerializer
            │
            ▼
    HttpServerResponse

Tests validate:

* routing
* controller selection
* response generation
* JSON output
* status codes

without sockets or networking.

---

## Out of Scope

This architecture does not introduce:

* TCP sockets
* bind()
* listen()
* accept()
* TLS
* threading
* authentication
* authorization
* HTTP parsing
* HTTP libraries
* web servers

---

## Design Goals

* End-to-end request testing
* Transport independence
* Router reuse
* Runtime consistency
* No networking dependencies
* Future compatibility with real HTTP servers
