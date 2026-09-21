# Phase 21.3 - Selective RESTfulAPI EPG Validation

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Completed Phases](completed-phases.md)
- [Roadmap](../planning/roadmap.md)
- [ADR-0021 Selective Backend Query Strategy](../adr/ADR-0021-selective-backend-query-strategy.md)

---

## Purpose

This document records the real VDR validation results for selective RESTfulAPI EPG access.

The validation checks whether existing RESTfulAPI event filters can avoid the expensive full `/events.json` transfer observed in Phase 21.0.

---

## Test Environment

Date:

```text
2026-06-12
```

Backend:

```text
Local VDR through vdr-plugin-restfulapi
RESTfulAPI endpoint: http://127.0.0.1:8002
```

Measurement method:

```text
curl -w TIME_TOTAL and SIZE_DOWNLOAD
JSON files written to /tmp
Event counts extracted with Python json parsing
```

---

## Measured Requests

### Full EPG Transfer

Request:

```text
GET /events.json
```

Result:

```text
TIME_TOTAL=29.647277
SIZE_DOWNLOAD=34302796
File size: 33 MB
Events: 39131
```

### One Hour Time Window

Request:

```text
GET /events.json?from=<now>&timespan=3600
```

Result:

```text
TIME_TOTAL=0.298154
SIZE_DOWNLOAD=463295
File size: 453 KB
Events: 593
```

### Two Hour Window With Per-Channel Event Limit

Request:

```text
GET /events.json?from=<now>&timespan=7200&chevents=2
```

Result:

```text
TIME_TOTAL=0.293206
SIZE_DOWNLOAD=417340
File size: 408 KB
Events: 556
```

### Single Channel 24 Hour Window

Request:

```text
GET /events/<channel-id>.json?from=<now>&timespan=86400
```

Result:

```text
TIME_TOTAL=0.102077
SIZE_DOWNLOAD=61530
File size: 61 KB
Events: 32
```

---

## Findings

Selective RESTfulAPI EPG access is dramatically faster than full-domain EPG transfer.

The full `/events.json` request needed almost 30 seconds and transferred about 33 MB.

Selective requests completed in about 0.10 to 0.30 seconds and transferred between about 61 KB and 453 KB.

The tested RESTfulAPI filters are sufficient to prove that VDR-Suite must not use full `/events.json` as the default runtime EPG access path.

The real bottleneck is not RESTfulAPI itself.

The real bottleneck is full-domain EPG access.

---

## Architecture Consequence

Phase 21.3 validates ADR-0021.

VDR-Suite should prefer selective backend queries over full-domain transfers whenever possible.

For EPG runtime use cases, preferred access patterns are:

```text
time-window EPG queries
channel-scoped EPG queries
limited per-channel event queries
single-event detail queries
change-hint driven selective refresh
```

Full `/events.json` should remain reserved for explicit full refreshes, diagnostics or controlled background work.

---

## Implementation Consequence

The `VdrEventQuery` contract introduced in Phase 21.2 is the correct first implementation step.

Future runtime polling must use domain-aware refresh decisions and selective EPG queries before considering full EPG refresh.

Phase 22 runtime polling must not introduce a fixed interval full `/events.json` loop.

---

## Validation Result

Status:

```text
Validated
```

Conclusion:

```text
Existing RESTfulAPI filters can provide live-like EPG access patterns for common runtime views.
```

New RESTfulAPI endpoints are not required before the existing selective filters are integrated and measured in VDR-Suite runtime behavior.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
