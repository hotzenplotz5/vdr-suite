# Phase 64 – commands.state v3 generic extension owner

This slice moves the bounded command-state extension from the Slice 28 contract
into the existing protected Agent `commands.state` owner. It does not open a
Timer-delete execution path.

## commands.state v3

The loader keeps three explicit, exact-key schemas:

- v1 is the unchanged legacy command state.
- v2 is the unchanged native-probe command state.
- v3 is exactly v2 plus one `state_extension` key.

Unknown or duplicate top-level keys remain invalid. Reading v1 or v2 does not
rewrite the file merely because an older schema was loaded. New lifecycle
persistence writes v3 through the existing protected atomic write path.

`state_extension` is either empty or contains one single-line `cse1` encoded
`BackendAgentCommandStateExtension`. The generic owner parses the envelope,
binds it to the enclosing command ID and request fingerprint, and delegates
supported typed payload validation back to the extension module.

The first supported type remains
`vdr.timer.delete.local-state.v1`. Its payload is decoded as the complete Slice
27 `BackendAgentNativeTimerDeleteLocalState` and correlated against the generic
`BackendAgentCommandAssignment`. A payload from another command or request
fingerprint is rejected. Unknown extension types are fail-closed because this
state can carry future mutation hazard evidence.

## Durability

There is still one state owner and one state file. The existing persistence
sequence remains unchanged:

1. open a temporary sibling with mode `0600`,
2. write the complete state,
3. `fsync` the file,
4. close it,
5. atomically rename it over `commands.state`,
6. `fsync` the parent directory.

No second Timer-delete state file is introduced.

## Runtime source boundary

Only the code required to validate and preserve the generic extension is added
to `AGENT_COMMAND_CLIENT_SRC`: the generic extension codec, the typed local
Timer-delete state contract, and its domain validator. No daemon source set and
no SuiteBridge write transport is expanded.

## No Timer-delete execution

This slice deliberately keeps the mutation unavailable:

- normal Agent configuration still rejects `vdr.timer.delete`,
- packaged configuration does not contain it,
- command availability suppresses it even if a caller constructs an in-memory
  command-type list manually,
- there is no Timer-delete executor,
- there is no SuiteBridge Timer-write transport,
- there is no RESTfulAPI or SVDRP Timer delete call,
- there is no shell fallback,
- and no VDR-native Timer mutation is performed.

The typed extension can therefore be recovered and durably carried by the
runtime state owner without crossing the side-effect boundary. The following
slice may integrate the typed local state into the command lifecycle, but must
still preserve durable `starting` before any future side effect and recover an
uncertain `starting` state as reconciliation-only.
