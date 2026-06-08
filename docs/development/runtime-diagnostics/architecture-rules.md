# Runtime Diagnostics Architecture Rules

## Navigation

- [README](../../../README.md)
- [Documentation Index](../../index.md)
- [Project Overview](../../project-overview.md)

---

## Rule 1

Runtime diagnostics are based on structured RuntimeMeasurement objects.

Log parsing is forbidden.

## Rule 2

Measurement producers must remain independent from REST controllers.

Producers record data only.

## Rule 3

Aggregation is performed centrally by RuntimeDiagnosticsService.

No component-specific aggregation logic is allowed.

## Rule 4

Serialization is performed exclusively by RuntimeDiagnosticsJsonSerializer.

## Rule 5

Diagnostics endpoints are read-only.

No runtime diagnostics modification API may be introduced.

## Rule 6

Diagnostics data must remain backend-neutral.

The runtime subsystem must not depend on RESTfulAPI-specific types.

## Rule 7

Memory usage must remain bounded through central retention control.

## Rule 8

Runtime diagnostics are an observability feature.

They must not become a replacement for business-domain APIs.
---

## Back

- [Back to Documentation Index](../../index.md)
- [Back to Project Overview](../../project-overview.md)
- [Back to README](../../../README.md)
