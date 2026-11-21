# Runtime Diagnostics Measurement Model

## Navigation

- [README](../../../README.md)
- [Documentation Index](../../index.md)
- [Project Overview](../../project-overview.md)

---

Runtime diagnostics use structured domain objects instead of log parsing.

## RuntimeMeasurement

`RuntimeMeasurement` represents one recorded runtime event.

Current fields:

```text
component
operation
durationMs
statusCode
sizeBytes
itemCount
```

Field meaning:

- `component`: producer component name, for example `BasicHttpClient`, `VdrSnapshotBuilder` or `PollingService`
- `operation`: measured operation name
- `durationMs`: measured duration in milliseconds
- `statusCode`: optional status code, currently used by HTTP measurements
- `sizeBytes`: byte size, currently used for HTTP response bodies
- `itemCount`: item count, used for snapshot domains, change detection and update-plan related counts

## RuntimeMeasurementSummary

`RuntimeMeasurementSummary` represents grouped measurement state for one `(component, operation)` pair.

Current fields:

```text
component
operation
count
minDurationMs
maxDurationMs
lastDurationMs
lastStatusCode
lastSizeBytes
lastItemCount
```

Field meaning:

- `count`: number of retained measurements in this group
- `minDurationMs`: shortest retained duration
- `maxDurationMs`: longest retained duration
- `lastDurationMs`: newest retained duration
- `lastStatusCode`: status code from the newest retained measurement
- `lastSizeBytes`: byte size from the newest retained measurement
- `lastItemCount`: item count from the newest retained measurement

## Phase 10.20 item count separation

Before Phase 10.20, some collection-domain counts were represented through `sizeBytes` even though they were not byte sizes.

Phase 10.20 separated the semantics:

- byte sizes stay in `sizeBytes`
- item counts are stored in `itemCount`
- summary item counts are exposed through `lastItemCount`

This keeps HTTP body sizes and collection sizes distinct and avoids overloading `sizeBytes`.
---

## Back

- [Back to Documentation Index](../../index.md)
- [Back to Project Overview](../../project-overview.md)
- [Back to README](../../../README.md)
