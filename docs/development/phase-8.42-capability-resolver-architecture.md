# Phase 8.42 – Capability Resolver Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)

---

## Status

Architecture decision document.

No implementation has been performed by this document.

## Context

ADR-0012 introduced Source capabilities as a future first-class architecture concept.

The Phase 8.41 platform action review clarified that capabilities and permissions should eventually share the same operation vocabulary.

A capability should not be modeled as an unrelated string hierarchy if VDR-Suite already has or will have platform actions.

The important future question is:

```text
Can this Source support this PlatformAction?
```

## Decision

VDR-Suite should treat capability evaluation as a resolver concept, not merely as a static field embedded directly into Source.

Short term, a simple CapabilitySet may be enough.

Long term, VDR-Suite should have a CapabilityResolver or equivalent service that answers whether a Source supports a PlatformAction.

Conceptual rule:

```text
supports(Source, PlatformAction) -> bool
```

## Why CapabilitySet Alone Is Not Enough

A static list is simple, but future capabilities may depend on more than SourceType.

Examples:

* backend mode
* installed VDR plugins
* RESTfulAPI endpoint availability
* local policy
* remote VDR-Suite exposure
* federation trust level
* archive mode
* read-only maintenance mode
* runtime health

A Source may be of type LocalVdr, but timer creation may still be unavailable if the selected backend cannot support it.

A RemoteSuite may expose only a subset of actions.

An Archive may gain extra actions when an offline worker is available.

Therefore capability evaluation should be resolvable, not permanently hardcoded into Source.

## Source Should Not Own Capability Logic

Source should remain a domain entity describing origin and identity.

Do not put action-specific capability logic into Source methods.

Avoid future design such as:

```text
source.canDeleteRecording()
source.canCreateTimer()
source.canStreamLiveTv()
```

That would make Source grow with every new operation.

Better future direction:

```text
CapabilityResolver.supports(source, action)
```

## CapabilitySet Role

CapabilitySet may still be useful as a value object.

Possible role:

* store explicit supported actions
* represent remote-exposed actions
* represent default action groups
* support tests
* support frontend hints later

But CapabilitySet should not be the only architecture concept.

CapabilitySet is data.

CapabilityResolver is decision logic.

## Relationship To SourceType

SourceType can provide default capability assumptions.

Examples:

* LocalVdr: candidate for recording, timer, EPG, channel and Live TV actions
* Archive: candidate for view and stream recording actions
* NasImport: candidate for import-related actions
* RemoteSuite: candidate capabilities depend on remote exposure
* Mock: deterministic test capabilities

However, SourceType must not be the final capability decision.

## Relationship To PlatformAction

Capabilities should be expressed in terms of PlatformAction.

This avoids maintaining two unrelated vocabularies.

Future examples:

* ViewRecording
* StreamRecording
* RenameRecording
* DeleteRecording
* CreateTimer
* ModifyTimer
* DeleteTimer
* ViewEpg
* SearchEpg
* ViewChannel
* StreamLiveTv
* ExecuteRemoteControl

Capability answers whether the Source supports the PlatformAction.

Permission answers whether the Actor may perform the PlatformAction.

## Relationship To Permission

Capability and permission must remain separate.

Capability check:

```text
Can this Source support this PlatformAction?
```

Permission check:

```text
May this Actor perform this PlatformAction on this ActionTarget?
```

Final future rule:

```text
allowed = source exists AND capabilityResolver.supports(source, action) AND permissionResolver.allows(actor, action, target)
```

This document does not implement permission resolution.

## Relationship To Backend Adapters

Backend adapters may provide technical capability evidence later.

Examples:

* RestfulApiVdrAdapter may know which endpoints are implemented.
* SvdrpVdrAdapter may support different operations.
* PluginBridgeVdrAdapter may support local-only operations.

But capability evaluation should remain above adapters.

Adapters may report facts.

The resolver makes the decision.

## Relationship To Frontend

Future frontends may use capability information to hide unavailable actions.

But frontend behavior is not a security boundary.

The backend must still evaluate capability and permission before executing actions.

## Implementation Boundary

Do not implement CapabilityResolver before Source, PlatformAction and ActionTarget have stable minimal models.

Do not add capability fields to Source in the first Source implementation.

Do not expose capabilities through REST yet.

Do not connect frontend behavior yet.

Do not infer security from frontend hiding.

## Recommended Future Phase Order

Architecture-safe order:

1. SourceType
2. Source
3. SourceRegistry
4. ContentIdentity
5. ActionTarget
6. PlatformAction
7. CapabilitySet value object
8. CapabilityResolver
9. Permission model implementation
10. Source-aware job execution
11. REST/frontend capability exposure
12. Federation capability exchange

## Non-Goals

This document does not implement:

* CapabilitySet
* CapabilityResolver
* PlatformAction
* ActionTarget
* permission checks
* adapter capability reporting
* REST output
* frontend behavior
* federation capability exchange

## Final Recommendation

Capabilities should be interpreted as Source support for PlatformActions.

Do not bake capabilities directly into Source.

Use SourceType for default assumptions, CapabilitySet for data, and CapabilityResolver for final decision logic when the implementation phase arrives.
---

## Back

- [Back to Development Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
