# Phase 63 Slice 5 Runtime — Explicit Local Provider Ownership and Selection

## Status

Runtime implementation stacked on the accepted contract in
`phase-63-local-provider-ownership.md`.

This slice is deliberately bounded to the existing side-effect-free command:

```text
vdr.native.probe
sideEffectClass = none
mutations = disabled
```

It does not authorize Timer, Recording, SearchTimer, Remote, configuration or
metadata mutations.

## Goal

Make the provider used by a native command an explicit, durable and replayable
choice instead of an implicit consequence of whichever adapter happens to be
available.

The runtime separates three independent facts:

```text
observed provider facts
        !=
configured provider ownership
        !=
persisted command provider selection
```

Availability is not authority.

## Provider facts

The Backend Agent publishes descriptive local provider facts through the
existing authenticated Agent command-poll request. No new public provider
endpoint is introduced.

For the bounded native probe the Agent publishes exactly one SuiteBridge fact
after successful native capability discovery:

```text
providerId = suitebridge:local
providerKind = suitebridge
providerInstanceEpoch = current pluginInstanceEpoch
providerGeneration = 1
capabilityRevision = nativeOperationSchema (= 1)
available = true
capabilities = [vdr.native.probe]
```

`providerGeneration = 1` is intentional for this bounded provider runtime. The
SuiteBridge transport binding is immutable for one Backend Agent process; Agent
instance and backend generation already fence process/config replacement.
`providerInstanceEpoch` independently fences VDR/plugin replacement, while
`capabilityRevision` independently fences the exact native-operation contract.

If native capability discovery fails, the Agent advertises neither
`vdr.native.probe` nor a SuiteBridge provider fact for it.

A poll atomically replaces the backend's previously observed command
capabilities and provider facts. Provider disappearance therefore removes the
fact instead of leaving a stale availability record.

## Explicit ownership

Provider ownership is Suite-owned configuration persisted separately from
provider facts.

The local administration utility exposes only the bounded actions required by
this slice:

```text
--provider-ownership-status
--set-native-probe-owner
--clear-native-probe-owner
```

`--set-native-probe-owner` means exactly:

```text
authorityDomain = vdr.native
providerId = suitebridge:local
providerKind = suitebridge
allowedCapabilities = [vdr.native.probe]
```

It accepts no URL, port, arbitrary command name or fallback list.

Ownership has a monotonic `ownershipGeneration`. Reapplying an identical active
configuration is idempotent and preserves the generation. Clearing ownership
increments the generation and leaves an inactive tombstone. Re-enabling it
increments the generation again. A previously selected command can therefore
never become current merely because the same provider name is configured again.

## Selection at assignment

`assignNativeProbe()` now requires all of the following at the same bounded
Control Plane decision point:

- authenticated system assignment context;
- active non-revoked Agent lease;
- exact Agent instance and backend generation;
- current `vdr.native.probe` command capability;
- active `vdr.native` ownership;
- provider facts for the explicitly owned provider;
- the required capability in both ownership and observed provider facts;
- provider availability.

There is no API in this slice that chooses among a list of available providers.
If `suitebridge:local` owns `vdr.native` and only a RESTfulAPI provider is
available, assignment fails closed.

The resulting `BackendAgentLocalProviderSelection` snapshots:

```text
backendId
authorityDomain
providerId
providerKind
ownershipGeneration
providerInstanceEpoch
providerGeneration
capabilityRevision
requiredCapability
```

The command row and provider-selection row commit atomically.

## Native probe payload v2

New assignments use:

```text
commandType = vdr.native.probe
payloadVersion = 2
verificationPolicy = readback_required
```

The strict payload contains the probe nonce plus the complete provider
selection. Its provider fields are therefore part of the existing normalized
command request fingerprint.

This means an otherwise identical command with a different ownership
Generation, provider epoch, provider Generation or capability revision has a
different command identity.

The v2 payload remains typed. It contains no provider URL, host, port,
credential, plugin command text, SVDRP command, shell fragment or arbitrary JSON
extension.

## Delivery fence

Before returning a selected native command from the command poll, the Control
Plane requires the persisted selection to still match:

- current active ownership and ownership generation;
- exact provider identity and kind;
- exact Agent/instance/backend generation that supplied the provider fact;
- exact provider instance epoch;
- exact provider generation;
- exact capability revision;
- provider availability.

If any value changed, the command is not delivered. Another available provider
is not a continuation candidate.

The same selection is checked again when the Agent submits command receipt and
result evidence. This closes the race between assignment/poll and local native
dispatch for this slice.

## Agent-side fence

The Agent derives its advertised SuiteBridge provider fact from the same native
capability discovery that decides whether `vdr.native.probe` is advertised in
the poll. It does not perform a second independent discovery just to construct
the provider fact.

Before first local dispatch, the Agent parses the v2 selection and requires it
to match the currently negotiated SuiteBridge capability:

```text
providerId = suitebridge:local
providerKind = suitebridge
authorityDomain = vdr.native
requiredCapability = vdr.native.probe
providerInstanceEpoch = current pluginInstanceEpoch
providerGeneration = 1
capabilityRevision = 1
```

It then persists `starting` before local execution exactly as Phase 63 already
requires.

For an incomplete v2 native command, loading protected local command state
forces an idempotent receipt re-confirmation before another reconciliation
step. A lost-response exact replay therefore crosses the current Control Plane
selection fence again before local replay.

The existing SuiteBridge epoch-scoped native receipt ledger remains the final
local exact-replay fence.

## Legacy v1 behavior

Payload version 1 remains structurally parseable so old durable command state
can be inspected safely during upgrade.

A newly received v1 `vdr.native.probe` is not eligible for a fresh native
dispatch under this runtime. It is rejected locally with no native execution
because it has no explicit provider selection.

A v1 command that had already persisted a SuiteBridge plugin epoch before an
upgrade remains reconciliation-only and may only use the same SuiteBridge epoch.
It is never converted to another provider.

## RESTfulAPI and other adapters

The provider contract recognizes `suitebridge`, `restfulapi` and `external` as
separate provider kinds. Recognition is descriptive, not permission.

This runtime wires only:

```text
vdr.native -> suitebridge:local -> vdr.native.probe
```

RESTfulAPI availability does not authorize native execution and cannot satisfy
SuiteBridge ownership. No RESTfulAPI execution route is added by this slice.

The historical preferred adapter order remains architectural guidance only. It
is not a runtime fallback chain.

## BackendNode.online

`BackendNode.online` remains untouched.

Backend online state continues to describe the existing backend/direct-adapter
health authority. Provider availability is a separate local fact and cannot set
or clear `BackendNode.online`.

A backend can therefore be online while its selected local provider is
unavailable, and a provider can be locally observed without acquiring backend
online authority.

## Persistence

The existing Suite SQLite database gains three bounded internal tables:

```text
backend_agent_local_provider_facts
backend_agent_local_provider_ownership
backend_agent_command_provider_selections
```

They contain identities, generations, availability and allowlisted capability
names only. They contain no provider URL, socket, credential or secret.

## Fail-closed cases

The runtime refuses assignment, delivery or continuation when applicable for:

- no explicit ownership;
- owned provider fact missing;
- owned provider unavailable;
- required capability not authorized;
- required capability not observed;
- provider identity or kind replacement;
- ownership generation replacement;
- provider instance epoch replacement;
- provider generation replacement;
- capability revision replacement;
- native v2 selection not matching the currently negotiated SuiteBridge fact;
- newly delivered legacy v1 native command;
- unknown provider kind.

No failure selects another provider automatically.

## Security boundary

This slice adds no public route and returns no private provider endpoint to
clients or the Control Plane command payload.

It does not add:

- generic SVDRP command tunnelling;
- free-form SuiteBridge command execution;
- shell/process execution;
- arbitrary provider registration by remote callers;
- automatic provider switching;
- production VDR mutation.

`mutations=disabled` remains mandatory in SuiteBridge capability and result
verification.

## Testing

Hosted tests cover:

- provider fact poll JSON round-trip;
- explicit ownership required before assignment;
- idempotent ownership set and monotonic revoke/re-enable generation;
- selected provider persisted in payload v2 and command sidecar;
- RESTfulAPI fact does not become fallback for SuiteBridge ownership;
- provider epoch replacement fences delivery and receipt;
- ownership replacement fences completion/reconciliation;
- capability revision replacement fences continuation;
- selected v2 native execution and readback;
- receipt re-confirmation before lost-response exact replay;
- plugin epoch replacement prevents blind native replay;
- newly delivered v1 native command performs no native dispatch;
- all pre-existing command, Agent, packaging and frontend regressions.

Because this slice changes installed daemon/Agent command behavior, the final
candidate also requires bounded real-yaVDR acceptance before it is considered
complete.

## Deferred before protected writes

This selection fence is deliberately not a general mutation lease.

Protected writes still require their own accepted contract for:

- domain-specific resource identity and expected revision;
- authorization freshness at the mutation boundary;
- durable mutation idempotency across VDR restart;
- authoritative post-write readback;
- mutation-specific reconciliation and unknown-outcome policy;
- provider capability that explicitly permits the exact write.

No protected write may infer authorization from the existence of this native
probe selection runtime.
