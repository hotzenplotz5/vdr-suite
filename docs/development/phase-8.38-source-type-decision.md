# Phase 8.38 – SourceType Architecture Decision

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)

---

## Status

Architecture decision for the next implementation step.

## Decision

The minimal Source domain model should include a dedicated SourceType concept.

SourceType should be a closed, strongly typed domain category in the first implementation.

Recommended initial values:

* LocalVdr
* RemoteVdr
* RemoteSuite
* Archive
* NasImport
* Mock

## Why SourceType Is Needed

A Source has two separate meanings:

* source.id: stable identity of one concrete source
* source.type: stable source category

Examples:

* id house-a, name House A VDR, type LocalVdr
* id house-b, name House B VDR-Suite, type RemoteSuite
* id archive-main, name Recording Archive, type Archive

Without SourceType, future code may infer source meaning from id, name or backendMode. That would mix stable identity, display naming and backend selection.

## SourceType Is Not BackendMode

SourceType describes the functional origin category.

backendMode describes adapter or backend selection.

They must remain separate.

Examples:

* LocalVdr may use restfulapi, svdrp or plugin backend modes.
* RemoteSuite may use a remote-suite API backend mode.
* Archive may not use a VDR adapter at all.

SourceType must not become RestfulApi, Svdrp, Http or Tcp. Those are adapter or transport concepts.

## Why Strong Type Instead Of Free String

SourceType should be a strong type because the initial set is architectural and small.

Benefits:

* compile-time safety
* explicit tests
* no magic strings in later policy logic
* easier switch-style handling
* stable source classification
* cleaner future capability defaults

Free strings can be introduced later for user labels or custom grouping, but not as the core source category.

## Initial Source Types

### LocalVdr

A VDR controlled directly by this VDR-Suite instance.

### RemoteVdr

A remote VDR accessed directly, without a remote VDR-Suite policy layer.

### RemoteSuite

A remote VDR-Suite instance.

This is different from RemoteVdr because it can expose its own policy, capabilities, users and federation behavior.

### Archive

A recording archive that is not a live VDR backend.

### NasImport

A non-VDR import source such as a NAS folder or import directory.

### Mock

A deterministic test source.

## Relationship To Capability Model

SourceType should not replace capabilities.

SourceType may provide future default capability assumptions, but final capability evaluation must remain separate.

Capability still answers what a Source can provide.

Permission still answers what an Actor may do.

## Relationship To Library Model

A Library remains a user-facing content view.

SourceType may later help filter or group library content internally.

This must not force frontend navigation to become source-first.

Library remains content-first.

Source remains origin and policy context.

## Implementation Boundary

The first SourceType implementation must not introduce:

* database schema
* REST output
* frontend logic
* permission checks
* capability checks
* adapter refactoring
* SourceRegistry
* federation
* remote discovery

## Recommended First Implementation Scope

A small implementation phase may include:

* SourceType enum class
* string conversion helper if needed by tests
* minimal tests for known values
* no runtime integration

The first Source object phase may then use SourceType as a field.

## Final Recommendation

Do not implement Source without SourceType.

Proceed with SourceType as a small, strongly typed domain concept before or together with the first minimal Source domain object.

Keep SourceType as a source category, not an adapter mode.
---

## Back

- [Back to Development Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
