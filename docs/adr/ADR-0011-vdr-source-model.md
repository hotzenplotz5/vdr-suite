# ADR-0011: VDR Source Model

## Status

Superseded by [ADR-0011: VDR Source Model Architecture](ADR-0011-vdr-source-model-architecture.md).

## Note

This document is kept for historical context only.

The canonical ADR for the Source model is:

- [ADR-0011: VDR Source Model Architecture](ADR-0011-vdr-source-model-architecture.md)

Do not extend this document for future architecture work.

Future Source-related changes should update or reference the canonical architecture ADR instead.

---

## Historical Context

ADR-0010 introduced the Library First VDR Architecture.

The target model was:

```text
User
        ↓
Library
        ↓
Content
        ↓
Source
        ↓
Backend
        ↓
VDR / Remote VDR-Suite / Archive
```

This created a new core architecture question:

```text
What is a Source?
```

VDR-Suite needed to answer this early because Source identity affects future library views, permissions, capabilities, federation, remote access and destructive operations.

The Source model must remain VDR-centered.

VDR remains the primary backend domain.

## Historical Decision

VDR-Suite would treat Source as a future first-class architecture concept.

A Source represents the origin and policy context of content or capabilities exposed through a library.

A Source is not the same as a backend implementation.

A backend is the technical adapter or transport.

A Source is the user- and policy-visible origin of content.

Conceptual relationship:

```text
Library
        ↓
Content Item
        ↓
Source
        ↓
Backend Adapter
        ↓
VDR / Remote VDR-Suite / Archive
```

## Historical Summary

This document established the early Source concept.

The later canonical ADR expanded and clarified:

- Source responsibility
- Source non-responsibility
- Source placement above backend adapters
- relationship to `VdrConfig`
- relationship to `IVdrAdapter`
- source-aware content identity
- capability relationship
- permission relationship
- architectural rules
- non-goals
- follow-up work

Use [ADR-0011: VDR Source Model Architecture](ADR-0011-vdr-source-model-architecture.md) for the accepted current architecture.
