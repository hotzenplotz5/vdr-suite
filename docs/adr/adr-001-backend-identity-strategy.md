# ADR-001 Backend Identity Strategy

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)

---

## Status

Accepted

## Context

VDR-Suite is designed as a backend-neutral architecture.

Current architecture already prepares:

- adapter-based backend integration
- daemon-owned snapshots
- change-state detection
- future multi-VDR deployments
- future permission-aware frontends

The project must avoid a permanent single-VDR assumption.

Future deployments may include:

- multiple VDR systems
- remote VDR systems
- read-only external systems
- mixed backend environments

A stable backend identity model is required before backend federation can be introduced.

---

## Problem

Current architecture assumes a single backend instance.

Future architectures require unambiguous identification of individual backend systems.

Examples:

- home VDR
- vacation house VDR
- test VDR
- remote family VDR

Hostnames, IP addresses and URLs must not be used as primary identifiers because they may change over time.

---

## Decision

Every backend shall have a stable BackendId.

BackendId is:

- globally unique within a VDR-Suite installation
- backend-neutral
- independent from hostname
- independent from IP address
- independent from transport protocol

Examples:

    home-vdr
    holiday-vdr
    parents-vdr
    lab-vdr

BackendId becomes the canonical backend identity used throughout the architecture.

---

## Future Usage

BackendId may later become part of:

- routing decisions
- backend federation
- permission systems
- synchronization logic
- event distribution
- frontend navigation

Future domain objects may optionally reference BackendId where required.

Examples:

- Recording
- Timer
- Channel
- Event
- Snapshot

This decision does not require BackendId fields to be added immediately.

The decision only reserves the architectural concept.

---

## Consequences

Benefits:

- enables future multi-VDR support
- enables backend federation
- avoids hostname coupling
- avoids IP-based identities
- provides stable backend references

Tradeoffs:

- additional architectural concept
- future configuration must manage BackendIds

---

## Related Architecture

See also:

- Snapshot Architecture
- IVdrAdapter architecture
- Future Backend Federation Strategy
- Future Permission Architecture
---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
