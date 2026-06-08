# Runtime Diagnostics Documentation

## Navigation

- [README](../../../README.md)
- [Documentation Index](../../index.md)
- [Project Overview](../../project-overview.md)

---

This directory contains the split runtime diagnostics documentation for VDR-Suite.

The previous single Phase 10 runtime diagnostics document mixed architecture, model, producer, REST, testing and roadmap details in one place. The documentation is now split by topic so each file stays small and focused.

## Documents

- [Architecture](architecture.md)
- [Architecture Rules](architecture-rules.md)
- [Measurement Model](measurement-model.md)
- [Measurement Producers](measurement-producers.md)
- [Aggregation](aggregation.md)
- [Serialization](serialization.md)
- [REST API](rest-api.md)
- [Daemon Integration](daemon-integration.md)
- [Testing](testing.md)
- [Roadmap](roadmap.md)

## Current verified state

Runtime diagnostics are implemented as structured measurements, not as parsed log output.

Implemented Phase 10 diagnostics capabilities include:

- `IRuntimeMeasurementSink`
- `RuntimeMeasurement`
- `RuntimeDiagnostics`
- `RuntimeDiagnosticsService`
- `RuntimeMeasurementSummary`
- bounded in-memory retention
- grouped measurement summaries
- `RuntimeDiagnosticsJsonSerializer`
- `RuntimeDiagnosticsController`
- `GET /api/runtime`
- `GET /api/runtime/summary`
- separate item count reporting through `itemCount` and `lastItemCount`

The current completed implementation step is Phase 10.20: complete runtime item count pipeline.

## Current verified commit

```text
0c97d86 Phase 10.20: complete runtime item count pipeline
```

Verified locally with:

```text
make test-runtime-diagnostics-service
make test-runtime-diagnostics-json-serializer
make test-runtime-diagnostics-controller
make test-vdr-snapshot-builder
make test
make daemon
git status
```

The verified result was green tests, successful daemon build and a clean synchronized branch.
---

## Back

- [Back to Documentation Index](../../index.md)
- [Back to Project Overview](../../project-overview.md)
- [Back to README](../../../README.md)
