# Runtime Diagnostics Aggregation

## Navigation

- [README](../../../README.md)
- [Documentation Index](../../index.md)
- [Project Overview](../../project-overview.md)

---

## Purpose

Aggregation converts individual RuntimeMeasurement entries into compact RuntimeMeasurementSummary objects.

The goal is long-term diagnostics visibility without exposing every recorded measurement.

## Aggregation Key

Measurements are aggregated by:

```text
component
operation
```

Example:

```text
component = snapshot-builder
operation = recordings
```

## Aggregated Values

Each summary contains:

- count
- itemCount
- minDurationMs
- maxDurationMs
- lastDurationMs
- lastStatusCode
- lastSizeBytes

## Item Count Support

Phase 10.20 introduced explicit item count aggregation.

Examples:

```text
recordings fetched
channels fetched
events fetched
timers fetched
```

This allows diagnostics consumers to distinguish:

- slow requests
- large result sets
- backend performance regressions

## Architectural Rule

Aggregation must remain deterministic.

Summaries must be derived exclusively from RuntimeMeasurement records.

No external state may participate in aggregation.
---

## Back

- [Back to Documentation Index](../../index.md)
- [Back to Project Overview](../../project-overview.md)
- [Back to README](../../../README.md)
