# ADR-008: Runtime Observability Strategy

## Status

Accepted

## Context

VDR-Suite now contains several runtime-observability building blocks:

- `IRuntimeLogger`
- `ConsoleRuntimeLogger`
- `RuntimeLogEntry`
- `RuntimeMeasurement`
- `RuntimeDiagnostics`
- `IRuntimeMeasurementSink`
- `RuntimeDiagnosticsService`
- structured measurements from `BasicHttpClient`
- structured measurements from `VdrSnapshotBuilder`
- structured measurements from `PollingService`
- daemon-owned `RuntimeDiagnosticsService` wiring

These components make runtime behavior visible without coupling core services to console output or REST controllers.

However, logging, diagnostics and debugging must not collapse into one unclear mechanism.

Without a clear strategy, future endpoints such as `/api/runtime` could become a mixed dump of human log text, timing measurements, internal state, backend payloads and troubleshooting data.

That would make the runtime API harder to keep stable, harder to secure and harder to evolve.

## Decision

VDR-Suite separates runtime observability into three distinct concerns:

```text
Logging
    ↓
Text

Diagnostics
    ↓
Measurements

Debugging
    ↓
State information
```

### Logging

Logging is human-readable runtime progress and error information.

Logging is represented by:

- `IRuntimeLogger`
- `RuntimeLogEntry`
- `RuntimeLogLevel`
- `ConsoleRuntimeLogger`

Logging may be written to console output, files or another future log sink.

Logging is not a structured diagnostics source.

Runtime diagnostics must not parse log output.

### Diagnostics

Diagnostics are structured runtime measurements.

Diagnostics are represented by:

- `RuntimeMeasurement`
- `RuntimeDiagnostics`
- `IRuntimeMeasurementSink`
- `RuntimeDiagnosticsService`

Diagnostics answer questions such as:

- how long did an HTTP request take?
- how large was the response body?
- how long did a snapshot domain build take?
- how long did a poll cycle take?
- which refresh path was executed?

Diagnostics are machine-readable and suitable for JSON serialization and future API exposure.

Diagnostics are not a free-form debug dump.

### Debugging

Debugging is structured state information used to understand why something does not work.

Debugging answers questions such as:

- what was the last successful poll?
- what was the last failed poll?
- what was the last backend error?
- which backend is currently active?
- what is the current backend lifecycle state?
- when was the snapshot last refreshed?
- which domains were refreshed last?
- what is the last known change state?
- what is the current cache state?

Debugging is not the same as diagnostics.

Debugging describes current or recent system state.

Diagnostics describe measured runtime events.

## Boundaries

The following boundaries must be preserved:

```text
Logging
    = text for humans

Diagnostics
    = measurements for machines

Debugging
    = state for troubleshooting
```

Runtime components may emit both log entries and measurements, but they must do so through separate interfaces.

Runtime diagnostics must be produced directly as `RuntimeMeasurement` values.

Runtime diagnostics must not be derived from `ConsoleRuntimeLogger` output.

Debugging data must not be hidden inside log text.

Debugging data should be represented explicitly when the project introduces debug state models.

## API Direction

Future API work should keep observability endpoints separated by purpose.

Recommended direction:

```text
/api/runtime
    diagnostics measurements and runtime measurement summaries

/api/debug
    structured troubleshooting state, if later needed
```

A future `/api/runtime` endpoint should not become a general debug dump.

A future `/api/debug` endpoint should be read-only by default and should not expose unsafe backend payloads without explicit design review.

## Security and Data-Minimization Rules

Debugging and diagnostics must avoid exposing unnecessary or sensitive data.

Future debug output should avoid dumping complete backend payloads such as:

- complete EPG data
- complete recording descriptions
- complete HTTP responses
- full filesystem paths unless explicitly needed
- credentials, tokens or authentication data

Debugging should prefer compact state summaries over raw payload dumps.

## Consequences

This decision allows Phase 10 diagnostics work to continue with a focused JSON serializer for `RuntimeDiagnostics`.

It also reserves a separate architectural space for future debugging support without polluting runtime measurements.

The next implementation phase can therefore remain:

```text
RuntimeDiagnostics
    ↓
RuntimeDiagnosticsJsonSerializer
    ↓
JSON
```

without deciding the full debug-state model yet.

Future debugging work should introduce explicit debug-state objects or services only when real troubleshooting needs are known from daemon runtime usage.
