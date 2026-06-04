# ADR-003 Backend Capability Strategy

## Status

Accepted

## Context

Different backend implementations may provide different features.

The frontend must not depend on backend implementation types.

---

## Decision

Backends shall expose capabilities.

Examples:

- Recordings
- Timers
- Channels
- EPG
- Live TV
- Streaming

User interfaces shall query capabilities.

User interfaces shall not identify backend types.

Bad:

if backend == VDR

Good:

if supportsRecordings()

---

## Consequences

Benefits:

- backend-neutral frontend architecture
- future backend extensibility
- reduced coupling
