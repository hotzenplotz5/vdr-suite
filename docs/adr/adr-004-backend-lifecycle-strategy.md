# ADR-004 Backend Lifecycle Strategy

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)

---

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
---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
