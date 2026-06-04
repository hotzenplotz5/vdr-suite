# ADR-004 Backend Lifecycle Strategy

## Status

Accepted

## Context

Future federated environments require explicit backend lifecycle handling.

Remote systems may become unavailable.

---

## Decision

Backends shall have lifecycle states.

States:

- Online
- Offline
- Reconnecting
- Failed
- Disabled

---

## Future Usage

PollingService

ChangeDetectionService

Snapshot management

Event delivery

may react to lifecycle changes.

---

## Consequences

Benefits:

- predictable backend behaviour
- improved monitoring
- future federation support
