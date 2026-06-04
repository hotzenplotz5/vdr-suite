# ADR-002 Backend Federation Strategy

## Status

Accepted

## Context

VDR-Suite is designed to support future multi-VDR and multi-backend environments.

ADR-001 introduced BackendId as the stable backend identity.

A federation model is required to manage multiple backend systems simultaneously.

---

## Decision

VDR-Suite shall support backend federation.

A single VDR-Suite installation may manage multiple backends.

Examples:

- home-vdr
- holiday-vdr
- parents-vdr
- lab-vdr

---

## Backend Registry

Future architectures may introduce a BackendRegistry.

Example:

BackendRegistry
    home-vdr
    holiday-vdr
    parents-vdr

The registry becomes the authoritative source of available backends.

---

## Routing

Future API routing may become backend-aware.

Examples:

GET /api/backends

GET /api/backends/home-vdr/recordings

GET /api/backends/holiday-vdr/recordings

---

## Permissions

Permissions may later be assigned per backend.

Examples:

View
Control
Admin

---

## Consequences

Benefits:

- supports multiple VDR systems
- supports remote installations
- supports backend-specific permissions
- supports future federation

Tradeoffs:

- additional architecture complexity
