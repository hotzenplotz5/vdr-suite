# Phase 63 Slice 5 — Explicit Local Provider Ownership Contract

## Status

**Binding contract for the next bounded Phase-63 runtime slice.**

```text
main baseline: 7ef3f77afef16952c5e1476dd5f30c4fe5190376
completed: Agent lifecycle
completed: read-only observation continuity
completed: durable command delivery
completed: fenced native operations
next: explicit local provider ownership and selection
```

This is a contract-only slice. It does not reroute reads or commands, expose a
provider endpoint, change `BackendNode.online`, enable mutations, or add a VDR
write.

## Core rule

Availability is not authority. Capability advertisement is not authority. A
preferred adapter order is not authority. SuiteBridge, RESTfulAPI or another
adapter must never become active merely because it is reachable.

The contract separates three objects.

### Provider facts

`BackendAgentLocalProviderFacts` describes one observed local provider:

```text
providerId
providerKind
providerInstanceEpoch
providerGeneration
capabilityRevision
available
capabilities[]
```

Facts are descriptive only. `available=true` and an advertised capability do
not authorize use. `providerInstanceEpoch` identifies the current local runtime
instance; for SuiteBridge a later runtime may bind it to the existing
`pluginInstanceEpoch`. `providerGeneration` fences replacement or
reconfiguration of the logical provider. `capabilityRevision` fences the exact
advertised capability snapshot.

### Ownership

`BackendAgentLocalProviderOwnership` is explicit authority for one Backend and
one VDR authority domain:

```text
backendId
authorityDomain
providerId
providerKind
ownershipGeneration
allowedCapabilities[]
```

Ownership is policy/configuration state, never discovery. Its capability list
is an explicit allowlist and may not be empty. Changing provider identity, kind,
domain or allowed capabilities requires a new `ownershipGeneration`.

### Selection

`BackendAgentLocalProviderSelection` is the immutable snapshot pinned to one
read/command/attempt:

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

A selection is valid only when explicit ownership names the exact provider,
the capability is allowed by ownership, the same capability is currently
observed, the provider is available, and all identity/generation fences are
valid.

There is no API in this slice that chooses among a list of available providers.
The selector validates exactly one explicitly owned provider. This prevents
silent priority- or availability-based fallback.

## Provider kinds

The contract recognizes only existing production-facing integration families:

- `suitebridge` — typed SuiteBridge/VDR local integration;
- `restfulapi` — vdr-plugin-restfulapi adapter integration;
- `external` — the existing generic external adapter boundary.

Unknown provider kinds fail closed. `mock` is intentionally not a production
ownership kind. Adding a provider family requires an explicit contract change.

The historical "Preferred execution order" in
`docs/architecture/vdr-backends.md` is therefore only architectural preference
when ownership is deliberately configured. It is **not** a runtime chain such
as SuiteBridge -> RESTfulAPI -> SVDRP -> filesystem.

## Capability semantics

Both statements must be true:

```text
provider facts: provider CAN currently perform capability X
ownership:      provider MAY be used for capability X in domain D
```

Neither substitutes for the other. Existing `VdrCapabilitySet`, Backend Agent
capability publication and SuiteBridge capability discovery remain descriptive
until explicit ownership authorizes use. Ownership never manufactures a
missing provider capability.

## Backend online remains separate

`BackendNode.online` keeps its existing meaning. Provider availability is a
separate local fact, so all combinations are legal:

```text
BackendNode.online=true   provider available=true
BackendNode.online=true   provider available=false
BackendNode.online=false  provider available=true
BackendNode.online=false  provider available=false
```

Provider availability must not write `BackendNode.online`, and Backend liveness
must not synthesize provider ownership.

## Fail-closed continuation

Once selected, work remains pinned to the complete selection fence. Before
first dispatch and before replay or reconciliation, the later runtime must
compare persisted selection with current ownership and provider facts.

Continuation fails closed if Backend/domain, provider ID/kind,
`ownershipGeneration`, `providerInstanceEpoch`, `providerGeneration`,
`capabilityRevision`, required capability authorization/observation, or provider
availability differs.

If provider P becomes unavailable while provider Q is healthy, Q is **not** a
continuation candidate. Transparent provider switching is forbidden. Any future
re-selection must be a new explicit policy decision with a new selection fence,
not replay of the old operation.

`backendAgentLocalProviderSelectionIdentity()` includes every selection fence
field, so exact replay identity is provider-bound.

For the existing `vdr.native.probe`, the later runtime slice must bind:

```text
authorityDomain = vdr.native
providerKind = suitebridge
requiredCapability = vdr.native.probe
providerInstanceEpoch = current pluginInstanceEpoch
```

This complements rather than replaces command, claim, Agent instance, backend
generation and native receipt/readback fences.

## Security and mutation boundary

Ownership/facts are internal Agent/Control-Plane state. No provider URL, port,
socket path, credential, SVDRP command or arbitrary adapter method is exposed to
public callers. There is no generic command tunnel and no public caller-selected
provider string.

Production mutations remain disabled. This slice adds no Timer, Recording,
SearchTimer, metadata, Remote/OSD or configuration write and does not introduce
`mutations=enabled`.

Before a protected write exists, its runtime contract must define the authority
domain, required capability, persisted ownership, immutable persisted selection,
provider instance/generation/capability fences, resource identity/revision,
durable idempotency, authoritative readback and reconciliation behavior.

## Deferred runtime slice

A separate Draft PR must add provider fact/ownership persistence and wire the
selection fence into assignment, local dispatch, replay and reconciliation. Its
first proof may explicitly bind `vdr.native.probe` to `suitebridge`, but it must
not add automatic fallback or a production mutation.

## Acceptance

Hosted validation is sufficient because this PR has no installed runtime effect:

- ownership/selection unit tests pass;
- an architecture guard proves the model is not wired into production runtime;
- the existing CI remains green;
- no real yaVDR acceptance is required for this contract-only PR.
