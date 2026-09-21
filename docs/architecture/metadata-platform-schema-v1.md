# Suite Metadata Platform Schema v1

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Architecture Index](index.md)
- [Metadata Identity Foundation](metadata-identity-foundation.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Implementation Dependency Map](../planning/implementation-dependency-map.md)
- [ADR-0038](../adr/ADR-0038-suite-metadata-database-and-external-provider-strategy.md)

---

## Purpose

Phase 61.2 defines the first canonical SQLite schema contract for the Suite-owned metadata platform.

The schema is stored in:

```text
database/schema/metadata-platform-v1.sql
```

It is deliberately separate from runtime migration and repository integration. Phase 61.3 owns additive installation into existing databases and the first repositories.

---

## Legacy Coexistence

The repository already contains Phase-1 tables named:

```text
metadata
artwork
```

Those tables use integer IDs, direct Recording foreign keys and provider/path fields. They remain valid legacy storage for the existing dashboard foundation.

Schema v1 does not:

- rename them;
- reinterpret their integer IDs as `MetadataEntityId`;
- turn their `external_id` into Suite identity;
- turn their artwork paths into `ArtworkAsset` identity;
- delete or migrate their rows.

The normalized platform uses names beginning with:

```text
suite_metadata_
```

This permits an additive migration and an explicit later compatibility bridge.

---

## Schema Version

`suite_metadata_schema_versions` records applied metadata-platform schema versions independently from legacy database creation history.

Version 1 covers:

- normalized entity identity;
- Suite target identity;
- provider registry and backend scope;
- immutable provider evidence;
- target-to-entity assignment;
- assignment evidence links;
- provider-scoped external-ID bindings.

---

## Entity Table

`suite_metadata_entities` owns canonical metadata entity identity.

Core fields:

```text
metadata_entity_id
media_type
lifecycle_state
revision
merged_into_entity_id
```

The first media-type vocabulary is:

```text
unknown
movie
series
season
episode
programme
person
```

Version 1 intentionally does not add a provider-specific title or payload column to the canonical identity row. Normalized field selection belongs to the Phase 61.4 resolver and the Phase 61.5 field provenance model.

Merged entities retain identity history and point to another Suite entity. An entity cannot merge into itself.

---

## Target Table

`suite_metadata_targets` persists the resource identity that receives metadata assignments.

Initial target types:

```text
recording
program-event
timer-intent
```

A target row does not contain a Recording path, backend list position, inode or provider external ID.

Phase 61.3 must add explicit bindings from existing persisted resources to `MetadataTargetId`. Until then, the table is a schema contract and not a claim that the current Recording cache has durable Suite identity.

---

## Provider Registry and Scope

`suite_metadata_providers` stores stable provider registrations.

Provider kinds:

```text
epg
plugin
external-catalog
sidecar
manual
suite-db
```

`suite_metadata_provider_scopes` separates global registration from backend-scoped availability.

Examples:

```text
provider=manual
scope=global
backend_id=

provider=restfulapi-scraper-bridge
scope=backend
backend_id=house-a
```

Scope state, priority and last error are operational provider-registry data. They are not metadata evidence and do not leak backend provider configuration into public entity identity.

---

## Immutable Evidence

`suite_metadata_evidence` stores one immutable provider observation for one Suite target.

Evidence includes:

```text
metadata_evidence_id
metadata_target_id
provider_id
backend_id when relevant
provider-scoped source type and external ID
observation time
language
provider revision
payload schema version
normalized provider payload
payload fingerprint
confidence
evidence state
```

Evidence is append-only. SQLite triggers reject `UPDATE` and `DELETE`.

A provider correction or retraction creates a new evidence row. The resolver decides which evidence remains usable; old evidence remains available for explanation and audit.

The normalized payload is internal provider evidence. It is not the public API contract and is not the canonical entity row.

---

## Assignment Model

`suite_metadata_assignments` links one Suite target to one normalized entity.

Assignment fields include:

```text
metadata_assignment_id
metadata_target_id
metadata_entity_id
assignment_state
confidence
manual_assignment
relationship_locked
supersedes_assignment_id
created_by_ref
revision
```

States:

```text
selected
superseded
disputed
withdrawn
```

A partial unique index permits only one `selected` assignment per target. Disputed alternatives may coexist for later review without becoming the active relationship.

Manual and locked flags prepare for later manual correction and RBAC ownership. Phase 62 defines actor identity and authorization; version 1 therefore keeps `created_by_ref` as a non-authoritative transition field.

---

## Assignment Evidence

`suite_metadata_assignment_evidence` records why an assignment exists.

Evidence roles:

```text
supporting
contradicting
manual-override
```

This relation keeps assignment decisions explainable without embedding mutable provider rows directly into the entity or assignment identity.

---

## External IDs

`suite_metadata_entity_external_ids` records provider-scoped external-ID bindings.

The binding key includes:

```text
metadata_entity_id
provider_id
external_namespace
external_id
```

External IDs are lookup and reconciliation evidence. They are not the primary key of `MetadataEntity`.

The schema permits disputed bindings. A global uniqueness rule is intentionally not imposed because provider errors and duplicate/conflict resolution must remain representable.

---

## Enforced Constraints

The Phase 61.2 regression test proves:

- the schema can be applied after the legacy schema;
- applying schema v1 twice is idempotent;
- legacy and normalized tables coexist;
- opaque ID prefixes and lowercase-hex payloads are enforced;
- provider IDs reject URLs and invalid slugs;
- provider global/backend scope consistency is enforced;
- foreign keys remain valid;
- Evidence cannot be updated or deleted;
- one selected assignment per target is enforced;
- disputed alternatives remain representable.

Test target:

```text
make test-metadata-schema-contract
```

The test is included in `test-fast` through `test-metadata-foundation`.

---

## Non-Goals

Phase 61.2 does not implement:

- runtime schema migration;
- migration of existing Recording rows;
- metadata repositories;
- provider adapters;
- resolver or normalization services;
- field-level selected values;
- field provenance and conflict resolution;
- artwork assets;
- async refresh;
- public APIs or frontend changes.

---

## Next Slice

Phase 61.3 installs schema v1 additively into existing databases, introduces persistent target bindings for current resources, and proves repository and recovery behavior.

---

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
