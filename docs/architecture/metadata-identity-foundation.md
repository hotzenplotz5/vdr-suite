# Metadata Identity Foundation

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Architecture Index](index.md)
- [Current State](../CURRENT.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Implementation Dependency Map](../planning/implementation-dependency-map.md)
- [ADR-0038](../adr/ADR-0038-suite-metadata-database-and-external-provider-strategy.md)
- [ADR-0014](../adr/ADR-0014-recording-identity-strategy.md)

---

## Purpose

Phase 61.1 introduces the identity and value-type boundary for the Suite-owned metadata platform.

This foundation exists before database tables, provider adapters and resolver behavior so later persistence cannot accidentally make provider rows, Recording paths, backend list positions or titles into canonical Suite identities.

---

## Canonical Identity Types

```text
MetadataEntityId
    Suite-owned identity of one normalized metadata entity.

MetadataAssignmentId
    Suite-owned identity of one assignment decision between a target resource
    and a MetadataEntity.

MetadataTargetId
    Suite-owned identity used when a resource participates in metadata
    assignment.

MetadataProviderId
    Stable operator-facing provider registry slug.
```

Opaque Suite IDs use distinct type prefixes plus 128 bits represented as lowercase hexadecimal text:

```text
mdent_<32 lowercase hex characters>
mdasg_<32 lowercase hex characters>
mdtgt_<32 lowercase hex characters>
```

The prefixes are part of validation. An Entity ID cannot be accepted as an Assignment ID or Target ID.

Provider IDs are stable lowercase registry slugs such as:

```text
restfulapi-scraper-bridge
sidecar.local
manual
```

Provider IDs are not external URLs, database row IDs or credentials.

---

## Target References

A `MetadataTargetRef` combines:

```text
target type
Suite-owned MetadataTargetId
```

Initial target types are:

```text
recording
program-event
timer-intent
```

The target type and Target ID together form the canonical assignment target key.

A target reference does not contain:

- a Recording path;
- a VDR Recording list number;
- an inode;
- a backend hash;
- a provider external ID;
- a title-derived fingerprint;
- an artwork path.

Those values remain backend bindings, addresses, fingerprints or provider evidence.

---

## Recording Migration Rule

ADR-0014 defines Recording address, stable-fingerprint and change-fingerprint concepts, but the current runtime does not yet persist a Suite-owned Recording resource identity.

Therefore Phase 61.1 defines the Target ID contract without pretending the current `recording_id`, path or cache key is durable identity.

Phase 61.2 and 61.3 must:

1. add a Suite-owned Target ID to persisted Recording resources;
2. retain backend identity, current address and fingerprints as separate bindings;
3. migrate existing cache rows additively;
4. create Metadata assignments only after a valid Target ID exists;
5. keep destructive Recording operations bound to current verified backend addresses and permissions.

---

## Media and Target Type Vocabulary

The initial normalized media vocabulary is:

```text
unknown
movie
series
season
episode
programme
person
```

Media type describes the normalized metadata entity. Target type describes the Suite resource receiving an assignment. They are deliberately separate.

A Recording may be assigned to a movie or episode MetadataEntity. A ProgramEvent may be assigned to the same entity through a different MetadataAssignment.

---

## Security and Compatibility Properties

- Opaque IDs are identifiers, not authentication secrets.
- IDs are never generated from mutable presentation text.
- External provider IDs remain evidence and lookup bindings.
- Provider replacement does not change Suite entity identity automatically.
- Frontends never need provider-specific IDs to navigate normalized metadata.
- Unknown, malformed or cross-type IDs fail validation deterministically.
- The identity layer has no VDR, RESTfulAPI, TVScraper or filesystem dependency.

---

## Non-Goals

Phase 61.1 does not implement:

- metadata database tables;
- Recording Target ID migration;
- provider evidence persistence;
- matching or resolver behavior;
- confidence or conflict policy;
- artwork ingestion or derivative storage;
- asynchronous refresh;
- public metadata API routes;
- frontend enrichment beyond Phase 60.15.

---

## Next Slice

Phase 61.2 defines the metadata, provider, evidence and assignment schema using these identities. Phase 61.3 then proves additive migrations, repository behavior and recovery.

---

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
