# ADR-0045: Canonical EPG Event Identity and Provenance

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Current State](../CURRENT.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Strict Roadmap](../planning/roadmap.md)
- [ADR-0038: Suite Metadata Database and External Provider Strategy](ADR-0038-suite-metadata-database-and-external-provider-strategy.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0044: Timer Intent, Assignment and Native Timer Model](ADR-0044-timer-intent-assignment-native-timer-model.md)

---

## Status

Accepted

Date: 2026-07-16

---

## Context

VDR-Suite already contains substantial EPG foundations:

- backend-scoped `VdrEvent` read models;
- selective now/next, time-window and channel-window queries;
- RESTfulAPI event mapping;
- an `EpgEventRepository` used as a bounded warm cache;
- EPG search and SearchTimer preview services;
- backend-scoped snapshots and change feeds;
- native VDR, RESTfulAPI, epgsearch, epg2vdr and epgd architecture audit evidence;
- metadata-provider and provenance foundations under ADR-0038;
- TimerIntent references to future canonical programme events under ADR-0044.

The current `VdrEvent` object carries a backend-native event ID, channel ID, title, text and schedule values. The current cache stores events by:

```text
backend_id + channel_id + event_id
```

When a backend-native event ID is unavailable, the repository currently derives a local fallback from:

```text
channelId + startTime + title
```

That behavior is adequate for a transient selective EPG cache. It is not a valid cross-backend or long-lived identity contract.

Backend event identifiers may be:

- meaningful only inside one VDR backend;
- scoped to one channel or EPG source;
- replaced when EPG data is reimported;
- absent from one adapter;
- duplicated across independent backends;
- reused after retention or provider changes;
- represented differently by VDR, RESTfulAPI, epgsearch, epg2vdr or another source.

Title, start time and channel are mutable descriptive values. They are useful matching evidence, but they are not stable public identities.

VDR-Suite must also distinguish an airing from its metadata entity:

```text
programme occurrence
  one scheduled broadcast at one time

content entity
  movie, episode, series, sports fixture or other editorial work
```

Two broadcasts of the same episode are normally two different programme occurrences. One programme occurrence may be observed by several backends or providers. A recording, TimerIntent or SearchTimer proposal needs the occurrence identity, while metadata and recommendation features also need a relationship to the underlying content entity.

Without a canonical event and provenance model, VDR-Suite cannot safely answer:

- whether two backend events represent the same scheduled occurrence;
- whether an EPG update moved one event or replaced it with another;
- which source supplied a selected title, description, rating or schedule value;
- why one conflicting provider value won;
- whether a TimerIntent still targets the same occurrence after an EPG revision;
- whether a missing backend event is withdrawn, expired, temporarily unavailable or remapped;
- whether epg2vdr or epgd data may be imported without becoming a shared-database protocol;
- whether a SearchTimer proposal is a duplicate of an existing intent;
- how to preserve historical evidence after the transient EPG cache expires.

This decision closes the architecture-design portion of gaps G-15 and G-16. Runtime implementation remains future work in Phase 61 and Phase 64.

---

## Decision

VDR-Suite adopts four separate EPG concepts:

```text
ProgramEvent
  suite-owned canonical identity for one programme occurrence

BackendEventRef
  backend-scoped reference to one native or adapter-observed event

EventObservation
  immutable source observation of event fields at one point in time

EventFieldEvidence
  field-level provenance used to resolve the canonical ProgramEvent revision
```

The content metadata entity remains separate:

```text
MetadataEntity
  editorial identity such as movie, episode, series or sports item
```

The relationship is:

```text
MetadataEntity
  0..n ProgramEvents

ProgramEvent
  0..n BackendEventRefs
  1..n EventObservations over time
  1 current canonical revision
```

The Control Plane owns canonical `ProgramEvent` identities, observation history, merge policy and provenance.

The Backend Agent transports backend-scoped event observations and capability facts.

The VDR plugin or another local adapter exposes bounded native EPG observations. It does not assign canonical programme IDs, run global merge policy or access the Control Plane database.

---

## Identity Model

The canonical identity vocabulary is:

| Identity | Meaning |
| --- | --- |
| `programEventId` | Stable opaque Suite identity for one scheduled programme occurrence. |
| `programEventRevision` | Opaque revision of the current canonical event representation. |
| `backendEventRefId` | Stable Suite identity for one binding between a backend/source event and a ProgramEvent. |
| `backendId` | Stable Suite backend identity. |
| `backendGeneration` | Observed backend process or Agent generation where relevant. |
| `nativeEventId` | Event identifier supplied by VDR, RESTfulAPI or another backend adapter. |
| `nativeChannelId` | Backend-native channel identity used by the observation. |
| `eventObservationId` | Stable Suite identity for one immutable provider observation. |
| `sourceId` | Stable configured identity of the EPG source or provider adapter. |
| `sourceEventId` | Provider-specific event or occurrence identifier, when available. |
| `metadataEntityId` | Optional ADR-0038 content entity assigned to the occurrence. |
| `timerIntentId` | Optional ADR-0044 intent targeting the occurrence. |

These identities are not interchangeable.

The public API must not treat the following as a `programEventId`:

- a VDR `EventID` alone;
- a RESTfulAPI event number;
- a channel plus start timestamp;
- a title hash;
- a database row number from epgd or another provider;
- a mutable EPG cache primary key;
- an adapter pointer or native object address.

`programEventId` is generated and owned by VDR-Suite. It remains stable while the occurrence is considered the same event under the reconciliation rules below.

---

## ProgramEvent Semantics

A `ProgramEvent` represents one occurrence of programme content on a broadcast or stream schedule.

Examples:

- one episode airing on one channel at 20:15;
- one live sports event carried by two mapped backends as the same service occurrence;
- one movie broadcast repeated the following week, represented as another ProgramEvent;
- one programme whose start time shifts by five minutes while retaining authoritative source continuity;
- one regional variant with different content or timing, represented separately unless strong source evidence proves equivalence.

A ProgramEvent carries at least:

| Field | Meaning |
| --- | --- |
| `programEventId` | Stable Suite occurrence identity. |
| `programEventRevision` | Opaque current revision. |
| `state` | Current canonical lifecycle state. |
| `canonicalChannelId` | Optional future Suite channel/service identity. |
| `scheduledStart` | Resolved start timestamp with timezone semantics. |
| `scheduledEnd` | Resolved end timestamp. |
| `durationSeconds` | Resolved duration. |
| `title` | Resolved display title. |
| `subtitle` | Resolved subtitle or episode title. |
| `description` | Resolved description. |
| `contentDescriptors` | Resolved or aggregated classification values. |
| `parentalRating` | Resolved rating with jurisdiction/source context. |
| `metadataEntityId` | Optional content identity assignment. |
| `resolutionState` | Whether identity and field conflicts are resolved, provisional or disputed. |
| `firstObservedAt` | First observation time. |
| `lastObservedAt` | Latest contributing observation time. |
| `createdAt` | Canonical record creation. |
| `updatedAt` | Current revision creation. |

The exact persistence schema may normalize these fields. Their ownership and semantics are mandatory.

### Occurrence identity is not content identity

A ProgramEvent is normally unique per scheduled occurrence.

The same `metadataEntityId` may be linked to many ProgramEvents.

A recurring programme title without an episode identity must not cause all broadcasts to collapse into one ProgramEvent.

### Occurrence identity is not one backend schedule row

Several BackendEventRefs may point to one ProgramEvent when they are verified observations of the same occurrence.

One backend event must not point to several active ProgramEvents at the same time. Historical rebinding remains recorded.

---

## ProgramEvent Lifecycle

Canonical lifecycle states are:

| State | Meaning |
| --- | --- |
| `provisional` | A candidate canonical event exists but identity evidence is not yet strong enough for normal automation. |
| `scheduled` | The event is current and schedulable. |
| `in_progress` | The scheduled occurrence is currently running according to resolved time or backend evidence. |
| `completed` | The occurrence ended and remains available for historical linkage. |
| `withdrawn` | Authoritative evidence says the occurrence was removed from the schedule. |
| `superseded` | Another ProgramEvent replaces this event after an explicit identity correction. |
| `merged` | This identity was merged into another canonical ProgramEvent; the redirect is durable. |
| `disputed` | Conflicting evidence prevents safe automatic resolution. |
| `expired_unconfirmed` | The event window elapsed without sufficient evidence to classify it more strongly. |

State changes create a new `programEventRevision` and retain the previous revision or an equivalent audit history.

Deleting an expired warm-cache row does not delete a ProgramEvent that is referenced by a TimerIntent, Recording, audit record or metadata assignment.

---

## BackendEventRef

A `BackendEventRef` records how one backend or local adapter identifies an event.

Required fields include:

| Field | Meaning |
| --- | --- |
| `backendEventRefId` | Stable Suite binding identity. |
| `programEventId` | Current canonical occurrence binding. |
| `backendId` | Owning backend. |
| `backendGeneration` | Optional generation observed with the reference. |
| `sourceId` | Adapter or provider source identity. |
| `nativeEventId` | Backend-native event identifier, when present. |
| `nativeChannelId` | Backend-native channel identifier. |
| `sourceEventId` | Provider event identifier, when distinct from nativeEventId. |
| `firstSeenAt` | First observation. |
| `lastSeenAt` | Latest observation. |
| `bindingState` | Active, stale, withdrawn, superseded or disputed. |
| `bindingConfidence` | Confidence in the canonical identity association. |
| `bindingMethod` | Exact ID, provider mapping, channel/time match, manual decision or other explicit method. |

The natural lookup tuple may include:

```text
backendId + sourceId + nativeChannelId + nativeEventId
```

It is an internal lookup key, not the public identity.

When `nativeEventId` is absent, the observation may still be stored, but its binding begins as provisional. A title/time fingerprint is evidence only and must not silently become the durable identity.

---

## EventObservation

Every source import produces immutable observations before canonical resolution.

An `EventObservation` carries:

| Field | Meaning |
| --- | --- |
| `eventObservationId` | Stable observation identity. |
| `sourceId` | Configured source or adapter identity. |
| `sourceType` | Native VDR, RESTfulAPI, epgsearch, epg2vdr, epgd import, XMLTV, manual or future provider. |
| `backendId` | Backend context where applicable. |
| `backendGeneration` | Generation context where applicable. |
| `nativeChannelId` | Source channel identity. |
| `nativeEventId` | Source event identity. |
| `sourceEventId` | Additional provider occurrence identity. |
| `observedAt` | Time VDR-Suite received or captured the observation. |
| `sourceUpdatedAt` | Provider update timestamp, when trustworthy. |
| `validFrom` | Optional provider validity boundary. |
| `payloadVersion` | Version of the normalized observation contract. |
| `rawFingerprint` | Hash of normalized source content for change detection. |
| `fields` | Immutable normalized source values. |
| `sourceDiagnostics` | Bounded parsing or quality facts. |

Provider-specific raw payloads may be retained according to operational and licensing policy, but they are not required public fields and must not become the domain contract.

Observations are append-only or revision-preserving. A provider refresh does not overwrite the evidence that supported an earlier canonical decision.

---

## EventFieldEvidence

Canonical values require field-level provenance.

For each resolved field, VDR-Suite stores enough evidence to answer:

```text
Which source supplied this value?
Which observation contained it?
When was it observed?
Was it selected, combined, overridden or rejected?
What confidence and authority rules applied?
Was a user override involved?
```

A field-evidence record carries at least:

| Field | Meaning |
| --- | --- |
| `programEventId` | Canonical event. |
| `programEventRevision` | Revision whose value was resolved. |
| `fieldName` | Canonical field. |
| `eventObservationId` | Contributing observation. |
| `sourceId` | Contributing source. |
| `observedValueFingerprint` | Stable fingerprint of the candidate value. |
| `decision` | Selected, combined, fallback, rejected, superseded or manual_override. |
| `confidence` | Normalized confidence or quality score. |
| `authorityClass` | Configured authority category for the field. |
| `reasonCode` | Deterministic resolver reason. |
| `decidedAt` | Resolution time. |
| `decidedBy` | Resolver version or authorized actor. |

A single canonical field may have multiple contributing evidence records.

---

## Source Identity and Capability

Every source has a stable configured `sourceId` and publishes source capabilities.

Examples of capabilities:

```text
epg.event.read
epg.event.native_id
epg.event.source_updated_at
epg.event.running_status
epg.event.content_descriptors
epg.event.parental_rating
epg.event.external_ids
epg.event.incremental_changes
epg.event.withdrawal_signal
epg.event.field_provenance
```

Source capabilities are not inferred only from the adapter class name.

A source definition includes:

- source ID and type;
- owning backend or global provider scope;
- adapter and schema version;
- field coverage;
- authority classes by field;
- freshness expectations;
- identifier stability guarantees;
- language and region context;
- attribution and retention requirements;
- degradation state.

An unavailable provider does not erase previously selected canonical values. It changes freshness and confidence state according to policy.

---

## Canonical Matching and Identity Resolution

Identity resolution is a staged process.

### Stage 1: Exact continuity

Prefer strong continuity evidence:

- an existing BackendEventRef with the same backend, source and stable native ID;
- the same provider occurrence ID under a source contract that guarantees stability;
- an explicit provider replacement or update link;
- a previously persisted import mapping;
- an authorized manual binding.

### Stage 2: Canonical service and schedule evidence

When exact IDs are unavailable, candidate matching may consider:

- canonical or strongly mapped channel/service identity;
- overlapping schedule window;
- normalized title and subtitle;
- episode, series or external content IDs;
- event duration;
- provider region and language;
- running-status continuity;
- source update lineage;
- known schedule-shift tolerances.

This stage produces candidates and confidence. It does not automatically guarantee identity.

### Stage 3: Conflict and ambiguity policy

When multiple candidates remain plausible:

- do not merge silently;
- create or retain a provisional ProgramEvent;
- mark the resolution as disputed when automation safety is affected;
- expose bounded operator-review facts;
- prevent TimerIntent deduplication from assuming equivalence.

### Forbidden identity shortcuts

The resolver must not declare canonical identity solely from:

- equal title;
- equal title and start time;
- equal channel name;
- equal description;
- one provider database row number without source scope;
- one hash that includes mutable display fields but no continuity policy.

---

## Schedule Changes and Event Continuity

EPG schedules change frequently. The resolver distinguishes an update to the same occurrence from replacement by another occurrence.

The same ProgramEvent may retain identity when:

- a stable source occurrence ID remains continuous;
- an existing BackendEventRef explicitly updates its schedule;
- authoritative source lineage reports a reschedule;
- the shift remains within configured policy and no competing occurrence creates ambiguity;
- a manual decision confirms continuity.

A new ProgramEvent is created when:

- authoritative evidence reports cancellation and replacement;
- the content, service or occurrence identity materially changes;
- a schedule shift collides with another plausible event;
- the original source ID is reused without reliable continuity;
- identity confidence falls below the automation threshold.

Relationships may record:

```text
rescheduled_from
replaces
replaced_by
split_from
merged_into
```

These relationships preserve history and support TimerIntent reconciliation.

---

## Canonical Field Resolution

Identity matching and field selection are separate decisions.

Two observations may be bound to the same ProgramEvent while disagreeing on title, description or start time.

Field resolution considers:

- configured field authority;
- source freshness;
- source capability and identifier quality;
- language and region preference;
- completeness;
- confidence;
- manual overrides;
- consistency with running-state observations;
- current ProgramEvent revision;
- whether changing the field affects TimerIntent safety.

### Schedule fields

Start and end time changes are safety-sensitive.

A schedule-field update must:

- create a new ProgramEvent revision;
- preserve previous values and evidence;
- classify the magnitude and source of the shift;
- trigger TimerIntent and assignment reconciliation where relevant;
- not silently retarget a native Timer without ADR-0042 mutation checks.

### Text fields

Title, subtitle and description may use configured fallback and language policy.

Combining text from different sources must remain explainable. The system must not present synthetic text as if it came from one provider.

### Ratings and descriptors

Parental ratings and content descriptors retain source, scheme, region and original values. A plain integer without jurisdiction is insufficient as the final normalized contract.

---

## Revision Model

A ProgramEvent has an opaque `programEventRevision`.

The revision changes when a canonical fact relevant to consumers changes, including:

- schedule;
- channel/service assignment;
- lifecycle state;
- resolved title or descriptive fields;
- metadata entity assignment;
- canonical merge or supersession relationship;
- resolution or dispute state.

Backend observation sequence, snapshot generation and ProgramEvent revision are separate values.

```text
backendGeneration
  fences one backend runtime generation

snapshotGeneration or event sequence
  orders backend observation delivery

programEventRevision
  protects the canonical ProgramEvent state
```

Clients and TimerIntent services must not substitute one value for another.

---

## TimerIntent Integration

ADR-0044 TimerIntents may reference a ProgramEvent.

A programme-event TimerIntent stores at least:

- `programEventId`;
- the ProgramEvent revision used during preview or activation;
- selected schedule evidence;
- channel requirement;
- margins and recording options;
- policy for later EPG changes;
- optional originating BackendEventRef and observation evidence.

A TimerIntent does not depend exclusively on the continued existence of one transient backend event row.

When the ProgramEvent revision changes, policy decides whether to:

- update an unbound assignment automatically within safe limits;
- require operator confirmation;
- update a bound native Timer through an ADR-0042 mutation;
- keep the original fixed schedule;
- cancel or mark the intent disputed.

A lost backend event reference does not by itself authorize creating another native Timer. Reconciliation must inspect the ProgramEvent, assignment and native binding state.

---

## SearchTimer and Automation Integration

SearchTimer, epgsearch and future automation providers produce candidates or TimerIntent proposals.

They do not own canonical event identity.

A candidate carries:

- ProgramEvent ID when already resolved;
- ProgramEvent revision;
- source BackendEventRef or observation;
- match evidence;
- duplicate-detection evidence;
- source rule identity;
- confidence and review requirements.

When only a backend event is available, the proposal may remain backend-scoped until canonical resolution completes.

Automation must not collapse two provisional events into one TimerIntent solely because their titles and times are similar.

---

## Recording and Metadata Integration

A Recording may later retain:

- the TimerIntent that produced it;
- the ProgramEvent associated with the recording;
- the ProgramEvent revision observed at recording start;
- metadata entity assignments;
- original backend event and Timer evidence.

The ProgramEvent remains an occurrence record. ADR-0038 MetadataEntity remains the editorial content record.

A Recording may exist without a ProgramEvent, for example after a manual instant recording or incomplete EPG. Missing EPG identity must not prevent Recording ingestion.

---

## Multi-Backend and Multi-Site Rules

A backend-native event is backend-scoped even when two VDR systems receive nominally identical EPG data.

Cross-backend canonical merging requires explicit evidence and channel/service mapping.

Rules:

- backend IDs remain part of BackendEventRef identity;
- hostname, IP address or channel number does not define canonical equivalence;
- the same native event ID on two backends is not sufficient evidence;
- regional or provider variants remain separate unless occurrence equivalence is proven;
- backend generation changes do not create new ProgramEvents by themselves;
- reconnect and full resync may create new observations while preserving existing refs;
- stale Agent generations cannot overwrite newer observations;
- offline observations retain their source timestamps and are evaluated for freshness on arrival.

---

## Plugin and Backend Agent Boundary

### Control Plane owns

- ProgramEvent identity;
- canonical revisions;
- BackendEventRef persistence;
- source registry and capability policy;
- observation history;
- merge and field-resolution policy;
- provenance and confidence;
- disputes and manual review;
- TimerIntent linkage;
- cross-backend deduplication;
- public API representation.

### Backend Agent owns

- authenticated transport of local EPG observations;
- backend generation and sequence context;
- adapter/source version publication;
- bounded buffering and resynchronization;
- reporting local capability degradation;
- preserving source and observation timestamps;
- no knowledge of Control Plane database tables.

### VDR plugin or local adapter owns

- safe native EPG access under correct VDR lock and thread rules;
- copying bounded event values and native identifiers;
- local event-change or full-snapshot publication where supported;
- truthful capability reporting;
- no global identity merge;
- no provider-priority policy;
- no canonical ProgramEvent database;
- no cross-site deduplication;
- no network or database I/O while VDR locks are held.

The current Suite Bridge plugin remains read-only. ADR-0045 does not require or enable plugin mutations.

---

## epg2vdr and epgd Boundary

VDR-Suite may later implement provider or migration adapters for epg2vdr and epgd.

The approved flow is:

```text
epgd or epg2vdr source
  -> explicit versioned provider or import adapter
  -> immutable EventObservations
  -> canonical resolver
  -> ProgramEvent and provenance store
```

The following is rejected as the platform protocol:

```text
Control Plane or Agent
  -> direct shared provider database coupling
  -> provider rows exposed as VDR-Suite identities
```

Provider database row IDs remain source-scoped evidence. Schema-version checks, read-only import boundaries, attribution, migration and rollback are mandatory before a concrete adapter is accepted.

---

## Warm EPG Cache Boundary

The existing `epg_events` table remains an implementation foundation for selective cached reads.

It is not the final ProgramEvent or provenance store.

The current composite key:

```text
backend_id + channel_id + event_id
```

continues to identify one cached backend event row only.

The fallback:

```text
channelId + startTime + title
```

remains a transient cache key fallback and must not be promoted to canonical identity.

Future migration may:

- add explicit BackendEventRef and observation tables;
- link cached rows to `backendEventRefId` and `programEventId`;
- separate raw observations from resolved canonical values;
- retain the existing cache for fast selective EPG browsing;
- expire cache rows independently from canonical historical records.

ADR acceptance does not change the current database schema.

---

## Conceptual Persistence Model

A future normalized schema contains concepts equivalent to:

```text
program_events
program_event_revisions
program_event_relations
backend_event_refs
event_sources
event_observations
event_observation_fields
event_field_evidence
event_resolution_decisions
program_event_metadata_assignments
```

Important constraints include:

- opaque primary IDs;
- one current canonical revision per ProgramEvent;
- immutable or revision-preserving observations;
- unique active backend/source/native binding where a stable native ID exists;
- durable merge redirects;
- no cascading deletion of ProgramEvents referenced by TimerIntents or Recordings;
- source and backend scope on every native identity;
- migration-safe schema versions.

The exact SQL design belongs to the Phase 61 implementation plan.

---

## Retention and Expiration

Transient EPG payload retention and canonical identity retention are different policies.

- warm-cache rows may expire after their useful query window;
- raw provider payloads may expire according to storage and licensing rules;
- observation fingerprints and selected evidence remain while needed for explainability;
- ProgramEvents referenced by TimerIntents, Recordings, audit or metadata remain durable;
- merged and superseded identities retain redirects;
- unreferenced historical events may be compacted only through an explicit retention policy.

Compaction must not make an existing public ID silently refer to a different occurrence.

---

## Manual Decisions

Authorized users or services may resolve disputed identity or field conflicts.

A manual decision records:

- actor identity;
- time;
- previous and new ProgramEvent revisions;
- selected or rejected observations;
- reason;
- whether the decision locks identity or selected fields;
- later conditions that may reopen the dispute.

Manual decisions are auditable under ADR-0049 and protected by ADR-0042 revision rules where they mutate canonical state.

---

## Error and Resolution Vocabulary

Minimum semantic categories include:

```text
source_unavailable
source_stale
unsupported_source_schema
observation_invalid
native_identity_missing
identity_ambiguous
identity_conflict
channel_mapping_missing
schedule_conflict
provider_conflict
manual_review_required
program_event_superseded
program_event_merged
program_event_withdrawn
revision_conflict
```

Transport-specific or provider-specific diagnostics may accompany these categories but do not replace them.

---

## Observability

Resolution and import diagnostics include bounded facts such as:

- source ID and adapter version;
- backend ID and generation;
- observation counts;
- new, changed, unchanged, withdrawn and disputed events;
- candidate-match counts;
- selected binding method;
- confidence and reason code;
- resolver version;
- ProgramEvent revision;
- TimerIntent reconciliation requests triggered;
- import duration and last successful observation time.

Logs must not contain unrestricted provider payload dumps, credentials or raw VDR pointers.

---

## Compatibility and Schema Evolution

Observation, source, ProgramEvent and provenance schemas evolve independently where appropriate.

Rules:

- existing field meaning is not changed silently;
- new optional fields have documented defaults;
- unsupported source payload versions are rejected or quarantined explicitly;
- canonical IDs are never regenerated merely because a schema version changes;
- migrations preserve merge redirects and BackendEventRef history;
- older Agents may publish a reduced observation contract according to negotiated capabilities;
- missing provenance fields degrade confidence rather than inventing evidence;
- public API versioning and ETags are finalized by ADR-0048.

---

## Security and Trust

EPG observations are data from a backend or provider, not trusted executable instructions.

Adapters and importers must:

- validate sizes and field formats;
- bound titles, descriptions and arrays;
- normalize timestamps and timezones explicitly;
- reject malformed identifiers;
- prevent SQL, HTML and command injection through correct storage and rendering boundaries;
- preserve provider attribution requirements;
- authenticate Agent-delivered observations;
- fence stale backend generations;
- avoid arbitrary file access through provider payloads.

A source may influence canonical EPG data only within its configured authority and capability scope.

---

## Test and Acceptance Standard

Implementation must include layered tests.

### Identity tests

- same strong source identity updates the same ProgramEvent;
- same native ID on different backends does not collide;
- missing native ID creates provisional evidence rather than a false durable key;
- title/time similarity alone does not force a merge;
- repeated broadcasts of the same metadata entity remain separate occurrences;
- explicit merge creates a durable redirect;
- source ID reuse without continuity creates a new candidate or dispute.

### Provenance tests

- every selected canonical field has evidence;
- rejected candidate values remain explainable;
- manual overrides are retained and revisioned;
- unavailable sources do not erase selected values;
- field authority and freshness produce deterministic decisions;
- ratings and descriptors retain original scheme/source context.

### Schedule and Timer tests

- schedule shifts create ProgramEvent revisions;
- TimerIntent reconciliation is requested only after durable event revision;
- ambiguous replacement does not silently retarget a TimerIntent;
- lost backend refs do not authorize duplicate native Timer creation;
- withdrawn and superseded events have deterministic policy results.

### Multi-backend tests

- one occurrence may bind to two verified backend refs;
- regional variants remain separate without strong evidence;
- stale Agent generation observations are rejected;
- reconnect full resync preserves canonical IDs where continuity is proven.

### Migration tests

- current `epg_events` rows can be imported as backend observations;
- fallback cache keys never become public ProgramEvent IDs;
- cache expiration does not delete referenced ProgramEvents;
- schema migration preserves IDs, revisions, refs and redirects.

### Live source acceptance

Concrete VDR, RESTfulAPI, epg2vdr or epgd adapters require controlled live acceptance that proves:

- bounded EPG access;
- correct native identity scope;
- source version detection;
- no VDR lock held during external I/O;
- resync after event-sequence loss;
- safe degradation when the source disappears;
- no mutation of VDR EPG or provider databases unless a later explicit write contract permits it.

---

## Implementation Sequence

The recommended sequence is:

1. define immutable source, BackendEventRef, observation and ProgramEvent value types;
2. define source capabilities and versioned observation envelope;
3. implement ProgramEvent and observation persistence with revisions;
4. migrate the current warm EPG cache into observation-compatible ingestion;
5. implement deterministic exact-identity continuity;
6. implement candidate matching and ambiguity handling;
7. implement field-level provenance and resolver decisions;
8. link EPG search and SearchTimer proposals to canonical events;
9. link TimerIntents to ProgramEvent revisions and reconciliation;
10. add provider/import adapters including optional epg2vdr or epgd adapters;
11. expose versioned public event contracts under ADR-0048;
12. add operational review and dispute tooling.

Phase 61 owns the canonical EPG and provenance platform foundation. Phase 64 consumes it for Timer orchestration.

---

## Consequences

### Benefits

- stable programme occurrence IDs across backend refreshes;
- explicit separation of occurrence, backend event and metadata entity;
- safe multi-backend TimerIntent targeting;
- explainable field selection and provider conflicts;
- provider replacement without changing public identity;
- migration path for epg2vdr and epgd without shared-database coupling;
- durable links from Recordings and Timers to historical programme evidence;
- clearer EPG cache retention and invalidation semantics;
- safer automation deduplication and schedule reconciliation.

### Costs

- canonical identity resolution is more complex than using backend event IDs;
- observation and evidence retention require additional storage;
- ambiguous events require explicit dispute handling;
- channel/service mapping becomes an important prerequisite;
- resolver versions and migration behavior must be maintained;
- source licensing and attribution may affect retention.

These costs are necessary for a trustworthy multi-backend EPG and Timer platform.

---

## Rejected Alternatives

### Use backend event IDs as global IDs

Rejected because they are backend- and source-scoped and may be absent, reused or inconsistent.

### Use channel, start time and title as the canonical ID

Rejected because all three values can change and title/time similarity cannot prove occurrence identity.

### Use the metadata entity ID as the event ID

Rejected because one movie or episode may have many scheduled occurrences.

### Let each automation provider own event identity

Rejected because SearchTimer, epgsearch and provider-specific identities would produce competing scheduling and deduplication domains.

### Expose epgd or provider database rows directly

Rejected because provider schema and row identity must not become the Control Plane or Agent contract.

### Keep only the latest merged event row

Rejected because Timer reconciliation, audit and explainability require previous observations and decisions.

---

## Non-Goals

This ADR does not implement:

- ProgramEvent database tables;
- a complete production matching algorithm;
- canonical channel/service identity;
- EPG write-back;
- a direct epgd or epg2vdr database connection;
- provider credentials;
- TimerIntent scheduler changes;
- public `/api/v1` event routes;
- metadata entity matching;
- frontend dispute tooling;
- final retention durations.

It defines the mandatory identity, provenance and ownership contract.

---

## Acceptance Criteria

ADR-0045 is implemented only when:

- ProgramEvent and BackendEventRef have separate durable identities;
- backend-native IDs are always source- and backend-scoped;
- observations and selected fields retain provenance;
- canonical revisions are distinct from backend generation and snapshot sequence;
- ambiguous identity does not silently merge;
- schedule shifts and replacements have deterministic continuity rules;
- TimerIntents can bind to ProgramEvent revisions;
- the existing warm cache is not treated as the canonical store;
- plugin and Agent ownership remains within the defined boundary;
- provider imports are versioned and do not expose shared databases as protocol;
- required automated and live-source tests pass;
- migration and rollback preserve stable IDs and evidence.

Acceptance of this ADR is not runtime completion.

---

## Related Decisions

- [ADR-0016: Snapshot Change Feed Architecture](ADR-0016-snapshot-change-feed-architecture.md)
- [ADR-0018: Incremental Snapshot Synchronization](ADR-0018-incremental-snapshot-synchronization.md)
- [ADR-0029: Backend-Neutral SearchTimer Architecture](ADR-0029-backend-neutral-searchtimer-architecture.md)
- [ADR-0034: SearchTimer Warm EPG Cache and Change Invalidation](ADR-0034-searchtimer-warm-epg-cache-and-change-invalidation.md)
- [ADR-0038: Suite Metadata Database and External Provider Strategy](ADR-0038-suite-metadata-database-and-external-provider-strategy.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0043: Job Claim, Retry and Saga Execution Model](ADR-0043-job-claim-retry-saga-execution-model.md)
- [ADR-0044: Timer Intent, Assignment and Native Timer Model](ADR-0044-timer-intent-assignment-native-timer-model.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
