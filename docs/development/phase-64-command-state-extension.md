# Phase 64 Slice 28: Agent command-state extension envelope

Slice 28 defines the bounded extension envelope that will let the existing
protected Agent `commands.state` owner carry typed mutation-specific local state
without adding a new top-level field for every future command family.

## Generic extension envelope

The envelope is bound to the exact generic command assignment by both command ID
and request fingerprint. It carries:

- schema version 1;
- a safe extension type identifier;
- command ID;
- request fingerprint; and
- a bounded typed payload.

Its serialized form starts with `cse1.` and hex-encodes all variable fields. The
result is a single-line value with no embedded newline or `=` delimiter, so it
can later be stored as one field in the current line-oriented `commands.state`
format without weakening that parser.

The payload remains bounded to 16 KiB and the complete encoded envelope to
40 KiB, below the existing 64 KiB protected command-state file limit.

## Timer-delete binding

The first typed extension is `vdr.timer.delete.local-state.v1`. Its payload is
exactly the Slice-27 `BackendAgentNativeTimerDeleteLocalState` encoding.

Wrapping requires the typed state's complete Timer-delete command envelope to
match the generic assignment, including operation, binding, TimerAssignment,
backend-native Timer, Agent generation and provider-selection fences. Parsing
repeats that exact correlation after decoding.

This prevents a durable local `starting` or completed outcome record from being
adopted by a different command or a changed request fingerprint.

## Why an extension envelope

Putting Timer-delete-specific keys directly into the generic command state would
make every later write command require another version-specific set of top-level
keys. A single strictly typed extension slot keeps the generic state owner
stable while mutation domains retain their own stronger invariants.

The next integration slice can therefore add one generic extension field to
`commands.state` v3 and delegate its contents to the appropriate typed contract.
Old v1/v2 state remains a separate compatibility concern of that state owner.

## No runtime wiring

`BackendAgentCommandStateExtension.cpp` is intentionally absent from
`mk/agent-sources.mk`. The existing `BackendAgentCommandClient.cpp` state owner,
Agent command advertisement, packaged configuration and SuiteBridge transports
are unchanged.

There is still no Timer-delete executor, no SuiteBridge Timer write transport,
no SVDRP/RESTfulAPI delete and no native VDR Timer mutation.

## Next bounded slice

The next slice may move `commands.state` to version 3 by adding one bounded
extension field, preserving v1/v2 loading and the current protected atomic write
semantics. Timer-delete must remain non-advertised and non-executable until its
separately gated executor/transport boundary exists.
