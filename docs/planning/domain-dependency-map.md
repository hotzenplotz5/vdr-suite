# VDR-Suite Domain Dependency Map

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Planning Index](index.md)
- [Current State](../CURRENT.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Implementation Dependency Map](implementation-dependency-map.md)
- [Strict Roadmap](roadmap.md)
- [Architecture Audit Gap Matrix](architecture-audit-gap-matrix.md)
- [ADR Index](../adr/index.md)

---

## Purpose

This document defines dependency direction between VDR-Suite domain models. Accepted architecture remains authoritative; ADR-0054 is represented as accepted architecture while runtime completion remains separately governed by the Strict Roadmap.

It answers:

```text
Which domain concepts must exist before another domain can be implemented safely?
Which identities may reference each other?
Which dependencies are forbidden?
```

This is a domain map, not a C++ include graph, database schema or runtime completion claim.

---

# 1. Dependency Rules

The map follows five rules:

1. Stable identity and trust concepts are lower-level prerequisites for protected domain actions.
2. Suite-owned canonical identities do not depend on frontend, transport or plugin representations.
3. Native backend references may be attached to Suite-owned resources, but they never replace Suite identity.
4. Orchestration domains depend on observations and policy; observations do not depend on orchestration.
5. Audit references domain facts without becoming the owner of those domains.

Notation:

```text
A -> B
  A depends on B

A -x-> B
  A must not depend on B
```

---

# 2. Foundation Domain Layers

```text
ActorIdentity
BackendId
SiteId
BackendGeneration
ResourceRevision
RequestId
CorrelationId
OperationId
JobId
AttemptId
SagaInstanceId
```

These identities are orthogonal. They are not aliases.

```text
ActorIdentity
  -> AuthenticationContext
  -> AuthorizationDecision

BackendId
  -> BackendAccessPolicy
  -> BackendCapabilityState
  -> BackendLifecycleState

BackendGeneration
  -> AgentCommandFence
  -> NativeBindingObservation
  -> MediaRoute
  -> BroadcastApplicationRef
  -> TeletextServiceRef
  -> OsdSurfaceRef
```

Foundation invariants:

- an actor is not a backend;
- a backend is not a site;
- a request is not an operation;
- an operation is not a job;
- a job retry creates a new attempt, not a new user intent;
- a resource revision is not a backend generation;
- a producer sequence is not a resource revision.

---

# 3. Actor, Trust and Authorization Domain

```text
ActorIdentity
  +--> UserIdentity
  +--> ServiceAccountIdentity
  +--> AgentIdentity
  +--> SystemActorIdentity
  +--> ExternalNativeActor

AuthenticationContext
  -> ActorIdentity
  -> CredentialReference
  -> SessionReference

AuthorizationDecision
  -> ActorIdentity
  -> Permission
  -> ResourceScope
  -> BackendScope
  -> PolicyVersion
  -> DecisionReason
```

```text
AgentTrustRecord
  -> AgentIdentity
  -> BackendId
  -> SiteId
  -> CredentialReference
  -> AllowedProtocolVersion
  -> TrustState

BackendLifecycleState
  -> BackendId
  -> BackendGeneration
  -> AgentIdentity
  -> LeaseState
  -> HealthState
```

Forbidden dependencies:

```text
ActorIdentity -x-> frontend session object
AuthorizationDecision -x-> UI visibility state
AgentIdentity -x-> unrestricted administrator identity
Permission -x-> backend capability implementation
```

A successful authentication establishes identity. It does not grant an operation.

---

# 4. Backend and Capability Domain

```text
BackendNode
  -> BackendId
  -> SiteId
  -> BackendAccessPolicy
  -> BackendLifecycleState
  -> BackendCapabilityReport

BackendCapabilityReport
  -> BackendId
  -> BackendGeneration
  -> CapabilityRevision
  -> CapabilityOrigin
  -> CapabilityAvailability
  -> DegradationReason
```

Capability and permission remain separate:

```text
operation allowed
  = authenticated actor
  AND authorization decision allows action
  AND backend access policy allows action
  AND required capability is available
  AND current backend generation is valid
  AND domain preconditions pass
```

Forbidden dependencies:

```text
Capability -x-> ActorIdentity
Capability -x-> role name
Permission -x-> adapter type
BackendAccessPolicy -x-> frontend control state
```

---

# 5. Recording Domain

```text
Recording
  -> RecordingId
  -> ResourceRevision
  -> NativeRecordingBinding
  -> RecordingTechnicalData
  -> MetadataAssignment
  -> ArtworkReference

NativeRecordingBinding
  -> BackendId
  -> BackendGeneration
  -> BackendNativeRecordingId
  -> NativeRevisionOrFingerprint
  -> BindingState
```

```text
RecordingMutationOperation
  -> ActorIdentity
  -> AuthorizationDecision
  -> RecordingId
  -> ExpectedRevision
  -> BackendId
  -> BackendGeneration
  -> IdempotencyScope
  -> OperationId
  -> VerificationPolicy
```

Recording dependencies:

- technical VDR data can exist without external metadata;
- metadata enrichment depends on stable `RecordingId`, not on a mutable path;
- artwork references depend on Suite-owned asset identity or an explicit temporary placeholder contract;
- move, rename, trash, restore and purge depend on the common mutation contract;
- cross-site storage mutation remains blocked until storage ownership and Agent semantics are defined.

Forbidden dependencies:

```text
RecordingId -x-> native filesystem path
RecordingId -x-> title
MetadataAssignment -x-> frontend card layout
Recording mutation -x-> implicit default backend
```

---

# 6. Metadata and Artwork Domain

```text
MetadataEntity
  -> MetadataEntityId
  -> MetadataEntityType
  -> CanonicalMetadataRevision
  -> FieldProvenance
  -> ProviderEvidence

MetadataAssignment
  -> MetadataAssignmentId
  -> RecordingId or ProgramEventId
  -> MetadataEntityId
  -> AssignmentConfidence
  -> AssignmentEvidence

ArtworkAsset
  -> ArtworkAssetId
  -> MetadataEntityId or RecordingId
  -> AssetVariant
  -> AssetOrigin
  -> AssetRevision
  -> DeliveryPolicy
```

Provider boundary:

```text
ProviderAdapter
  -> ProviderObservation
  -> ProviderEvidence
  -> normalization/resolution
  -> Suite-owned MetadataEntity or ArtworkAsset
```

The provider database is never the public or Agent protocol.

Forbidden dependencies:

```text
MetadataEntityId -x-> provider-native ID alone
ArtworkAssetId -x-> temporary provider URL alone
MetadataEntity -x-> TVScraper database schema
MetadataEntity -x-> frontend locale selection
```

---

# 7. Programme Event and EPG Domain

```text
ProgramEvent
  -> ProgramEventId
  -> ProgramEventRevision
  -> ChannelRequirement
  -> Schedule
  -> EventFieldEvidence
  -> MetadataEntityId optional

BackendEventRef
  -> BackendId
  -> BackendGeneration
  -> BackendNativeChannelId
  -> BackendNativeEventId
  -> SourceIdentity

EventObservation
  -> BackendEventRef or provider source reference
  -> ObservationId
  -> ObservedAt
  -> SourceSequence
  -> ObservedFields
  -> ObservationConfidence

EventFieldEvidence
  -> ProgramEventId
  -> ProgramEventRevision
  -> FieldName
  -> ObservationId
  -> ResolverReason
  -> Confidence
```

Dependencies:

- a `ProgramEvent` may link to a `MetadataEntity`, but broadcast identity and editorial content identity remain separate;
- a `TimerIntent` of type `programme_event` depends on `ProgramEventId` and retained schedule evidence;
- SearchTimer matching may use canonical events where available;
- event observations exist before canonical resolution.

Forbidden dependencies:

```text
ProgramEventId -x-> title + start time fingerprint
ProgramEventId -x-> one backend-native event ID
EventObservation -x-> TimerAssignment
ProgramEvent -x-> frontend timeline coordinates
```

---

# 8. Timer and Automation Domain

```text
AutomationSource
  +--> SearchTimerDefinition
  +--> epgsearch provider definition
  +--> future rule provider

AutomationProposal
  -> AutomationSourceId
  -> ProgramEventId optional
  -> BackendEventRef evidence
  -> ChannelRequirement
  -> ProposedSchedule
  -> DuplicateEvidence
  -> ReviewRequirement

TimerIntent
  -> TimerIntentId
  -> ActorIdentity or AutomationSourceId
  -> IntentType
  -> ProgramEventId optional
  -> ChannelRequirement
  -> DesiredSchedule
  -> RecordingOptions
  -> AssignmentPolicy
  -> ReplicaPolicy
  -> DuplicatePolicy
  -> IntentRevision

TimerAssignment
  -> TimerAssignmentId
  -> TimerIntentId
  -> BackendId
  -> BackendGeneration
  -> AssignmentRole
  -> SchedulerDecisionEvidence
  -> NativeTimerSpecification
  -> OperationId optional
  -> NativeTimerBindingId optional

NativeTimerBinding
  -> NativeTimerBindingId
  -> TimerAssignmentId
  -> BackendId
  -> BackendGeneration
  -> BackendNativeTimerId
  -> NativeRevisionOrFingerprint
  -> ObservedTimerState
```

Dependency direction:

```text
AutomationSource
  -> AutomationProposal
  -> TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
```

The reverse direction is forbidden as ownership:

```text
NativeTimerBinding -x-> owns TimerIntent
SearchTimerDefinition -x-> owns cross-backend scheduler
Plugin native timer -x-> global TimerIntent identity
```

A deliberate replica is a separate assignment. An unexplained second native timer is drift or duplicate evidence.

---

# 9. Mutation Operation, Job and Saga Domain

```text
Operation
  -> OperationId
  -> ActorIdentity
  -> AuthorizationDecision
  -> TargetResourceRef
  -> BackendId optional
  -> BackendGeneration optional
  -> ExpectedRevision optional
  -> IdempotencyScope
  -> NormalizedRequestFingerprint
  -> VerificationPolicy
  -> OperationState

Job
  -> JobId
  -> OperationId optional
  -> SagaInstanceId optional
  -> BackendId optional
  -> BackendGeneration optional
  -> ResourceRef optional
  -> PayloadVersion
  -> RetryPolicy
  -> ConcurrencyKey

Attempt
  -> AttemptId
  -> JobId
  -> ClaimEpoch
  -> ClaimToken
  -> WorkerIdentity
  -> DispatchState
  -> VerificationState
  -> ResultCategory

SagaInstance
  -> SagaInstanceId
  -> OperationId optional
  -> OrderedStepDefinitions
  -> CompensationPolicy
  -> SagaState
```

Dependencies:

- domain mutation services create or advance an `Operation`;
- jobs advance operations but do not replace public operation state;
- attempts are append-only execution evidence;
- sagas coordinate multiple durable jobs;
- audit references all four identities where applicable.

Forbidden dependencies:

```text
OperationState -x-> inferred only from HTTP status
Job terminal state -x-> silently invents public operation success
Retry -x-> creates a new logical actor request
Compensation -x-> erases original operation history
```

---

# 10. Media Domain

```text
MediaResourceRef
  +--> LiveChannelResource
  +--> RecordingResource

MediaSession
  -> MediaSessionId
  -> ActorIdentity
  -> AuthorizationDecision
  -> MediaResourceRef
  -> MediaPolicy
  -> SessionState
  -> Expiry

MediaRoute
  -> MediaSessionId
  -> BackendId
  -> BackendGeneration
  -> RouteEpoch
  -> ProviderType
  -> TransportMode

ProviderStreamLease
  -> ProviderStreamLeaseId
  -> MediaRoute
  -> AgentIdentity
  -> ProviderCapacityReference
  -> LeaseState

MediaAccessGrant
  -> MediaSessionId
  -> ConnectionScope
  -> Expiry
  -> RevocationState

PlaybackConnection
  -> MediaSessionId
  -> MediaAccessGrantReference
  -> ConnectionId
  -> BoundedConnectionState
```

Dependencies:

- media authorization depends on actor, backend and resource policy;
- route selection depends on backend lifecycle, capability and capacity;
- provider lease depends on a valid route and Agent generation;
- a connection depends on a short-lived access grant.

Forbidden dependencies:

```text
MediaSessionId -x-> bearer credential
Client -x-> permanent Streamdev URL
MediaSession -x-> native recording path
PlaybackConnection -x-> persistent Recording mutation right
```

---

# 10A. Broadcast Companion Domain — ADR-0054

This section reflects accepted architecture. Runtime remains planned for Phase 66 after Phase 65 and requires an explicit Phase-66 start.

```text
TeletextServiceRef
  -> BackendId
  -> BackendGeneration
  -> Channel / BroadcastService identity
  -> ProviderIdentity
  -> ProviderGenerationOrCapabilityRevision
  -> Freshness / Availability

TeletextPageRef
  -> TeletextServiceRef
  -> PageNumber
  -> SubpageIdentity optional

TeletextPage
  -> TeletextPageRef
  -> PageRevisionOrObservationSequence
  -> ObservedAt
  -> ImmutableBoundedPageState
  -> Completeness / Degradation

BroadcastApplicationRef
  -> BackendId
  -> BackendGeneration
  -> Channel / BroadcastService identity
  -> Broadcaster/Application identity
  -> DescriptorRevision

BroadcastApplicationDescriptor
  -> BroadcastApplicationRef
  -> DiscoveryProvenance
  -> ApplicationProfileFacts
  -> Autostart / RedButton semantics
  -> EntryPointEvidence
  -> Availability

BroadcastApplicationSession
  -> BroadcastApplicationRef
  -> ActorIdentity
  -> AuthorizationDecision
  -> BackendGeneration
  -> DescriptorRevision
  -> SessionState
  -> Expiry
  -> ApplicationRuntimeCapabilityProfile
```

Dependencies:

- Teletext and HbbTV discovery depend on current backend/channel/broadcast evidence;
- provider/cache/browser implementation details do not replace Suite resource identity;
- HbbTV application launch depends on actor/backend policy and an isolated runtime boundary;
- Suite-owned media requested from a broadcast application remains subject to Phase-65 `MediaSession`/Gateway semantics;
- channel/backend-generation change invalidates stale application/session context unless an explicit contract proves continuity.

Forbidden dependencies:

```text
TeletextPageRef -x-> provider cache filename
TeletextPage -x-> LegacyOsdSession as normal rendering authority
BroadcastApplicationRef -x-> URL alone
BroadcastApplicationSession -x-> unrestricted browser credential
BroadcastApplicationSession -x-> public raw URL/JavaScript/plugin command tunnel
HbbTV input -x-> arbitrary browser key code
```

Teletext and HbbTV are normal television-domain capabilities. Legacy OSD is not their primary product model.

---

# 11. Legacy OSD Domain

```text
OsdSurfaceRef
  -> BackendId
  -> BackendGeneration
  -> OsdEpoch

LegacyOsdSession
  -> LegacyOsdSessionId
  -> ActorIdentity
  -> AuthorizationDecision
  -> OsdSurfaceRef
  -> SessionState

OsdViewerBinding
  -> LegacyOsdSessionId
  -> ViewerActorId
  -> DeliveryCursor

OsdFrame
  -> OsdSurfaceRef
  -> FrameSequence
  -> ImmutableBoundedState

OsdDelta
  -> OsdSurfaceRef
  -> PreviousSequence
  -> DeltaSequence
  -> ImmutableBoundedChange

OsdControllerLease
  -> LegacyOsdSessionId
  -> ControllerActorId
  -> LeaseEpoch
  -> Expiry
  -> RevocationState

OsdInputCommand
  -> OsdControllerLease
  -> BackendGeneration
  -> OsdEpoch
  -> AllowlistedCommand
  -> Deadline
```

Dependencies:

- viewing depends on `osd.view` permission;
- controlling depends on separate `osd.control` permission and a valid exclusive lease;
- input depends on current backend generation, OSD epoch and lease epoch;
- read-only backend policy prohibits control;
- structured EPG/Timer/Recording/Streaming/Teletext/HbbTV domains take precedence over OSD compatibility when available.

Forbidden dependencies:

```text
OsdInputCommand -x-> arbitrary shell command
OsdInputCommand -x-> raw SVDRP tunnel
OsdInputCommand -x-> replay after reconnect
OsdFrame -x-> durable audit payload
```

---

# 12. Accountability Domain

```text
AccountabilityEvent
  -> AccountabilityEventId
  -> EventType
  -> SchemaVersion
  -> Class audit/security
  -> Severity
  -> SensitivityClass
  -> RetentionClass
  -> ActorContext
  -> AuthorizationDecisionReference optional
  -> TargetResourceRef optional
  -> BackendId/SiteId optional
  -> BackendGeneration optional
  -> RequestId/CorrelationId optional
  -> OperationId/JobId/AttemptId/SagaInstanceId optional
  -> NormalizedAction
  -> Outcome
  -> BoundedEvidence
```

```text
SecurityAlert
  -> SecurityAlertId
  -> one or more AccountabilityEventId values
  -> AlertState
  -> ResponsePolicy

SecurityIncident
  -> SecurityIncidentId
  -> event and alert references
  -> investigation and response history
```

Dependencies:

- accountability events reference domain identities and decisions;
- they do not own the target resource lifecycle;
- pre-dispatch events may be a mandatory operation prerequisite;
- post-dispatch events use outbox/recovery and cannot trigger blind redispatch;
- alerts and incidents derive from immutable source evidence.

Forbidden dependencies:

```text
AccountabilityEvent -x-> console log parsing
AccountabilityEvent -x-> bearer token or password
AccountabilityEvent -x-> full media, OSD or EPG payload
Alert cleared -x-> delete source event
```

---

# 13. Public API Resource Dependency

The public API exposes Suite-owned resources. Illustrative mature resource families include:

```text
/api/v1/backends
/api/v1/channels
/api/v1/program-events
/api/v1/recordings
/api/v1/timer-intents
/api/v1/timer-assignments
/api/v1/operations
/api/v1/jobs
/api/v1/media-sessions
/api/v1/broadcast-applications
/api/v1/teletext-services
/api/v1/legacy-osd-sessions
/api/v1/capabilities
```

Public representations depend on stable domain resources and policy-filtered views. The exact Phase-68 stable public inventory must be selected from actually implemented mature domains rather than inferred from this illustrative list.

Domain resources do not depend on:

```text
HTTP path aliases
JSON field ordering accidents
frontend module names
RESTfulAPI payload shapes
SVDRP reply text
plugin-local schema fields
```

---

# 14. High-Level Domain Graph

```text
ActorIdentity ------------------------------+
     |                                      |
     v                                      v
AuthenticationContext              AccountabilityEvent
     |                                      ^
     v                                      |
AuthorizationDecision ---------------------+
     |
     +----------------------+------------------------------+
     |                      |                              |
     v                      v                              v
Recording domain       TimerIntent domain      Media / Broadcast / OSD sessions
     |                      |                              |
     v                      v                              v
Operation ----------------+------------------------------+
     |
     v
Job -> Attempt -> Verification/Reconciliation

BackendNode -> Lifecycle/Generation -> Capability/Access Policy
     |                 |                         |
     +-----------------+-------------------------+
                       |
                       v
             native bindings, providers and routes

EventObservation -> ProgramEvent -> TimerIntent
ProviderEvidence -> MetadataEntity -> Recording/ProgramEvent enrichment
BroadcastEvidence -> TeletextService / BroadcastApplication
```

---

# 15. Phase Dependency View

```text
Phase 60.15
Recording metadata representation preparation
        |
        v
Phase 61
MetadataEntity, provenance and ArtworkAsset
        |
        v
Phase 62
ActorIdentity, authorization and AccountabilityEvent
        |
        v
Phase 63
AgentIdentity, BackendGeneration, secure remote execution
        |
        v
Phase 64
TimerIntent, assignment, native binding and reconciliation
        |
        v
Phase 65
MediaSession, route, grant, provider lease and playback adaptation
        |
        v
Phase 66
TeletextService/Page + BroadcastApplication discovery/session [ADR-0054 accepted]
        |
        v
Phase 67
LegacyOsdSession, viewer, controller lease and input
        |
        v
Phase 68
Stable /api/v1 representations for implemented mature domains
        |
        v
Phase 69
Recommendations and knowledge graph over mature identities/provenance
```

Later phases may prepare isolated internal code only when they do not publish, activate or bypass prerequisites. The strict roadmap controls runtime start and completion. Accepted ADR-0054 does not authorize Phase-66 runtime before Phase 65 closes and Phase 66 is explicitly started.

---

## Domain Acceptance Rules

A domain slice is not ready merely because its class names exist. It must prove:

- stable identities;
- explicit ownership;
- persistence and migration behavior where durable;
- revisions and generation semantics where mutable or remote;
- authorization boundaries;
- deterministic error and conflict behavior;
- bounded adapter or provider contracts;
- audit classification;
- failure, restart and reconciliation behavior;
- tests and documentation.

---

## Related Decisions

- [ADR-0038](../adr/ADR-0038-suite-metadata-database-and-external-provider-strategy.md)
- [ADR-0039](../adr/ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040](../adr/ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041](../adr/ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0042](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0043](../adr/ADR-0043-job-claim-retry-saga-execution-model.md)
- [ADR-0044](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0045](../adr/ADR-0045-canonical-epg-event-identity-provenance.md)
- [ADR-0046](../adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0053](../adr/ADR-0053-client-playback-engine-media-adaptation-strategy.md)
- [ADR-0054](../adr/ADR-0054-broadcast-companion-teletext-hbbtv.md)
- [ADR-0047](../adr/ADR-0047-legacy-osd-compatibility-bridge.md)
- [ADR-0048](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0049](../adr/ADR-0049-audit-security-event-model.md)

---

## Back

- [Back to Planning Index](index.md)
- [Back to Strict Roadmap](roadmap.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)