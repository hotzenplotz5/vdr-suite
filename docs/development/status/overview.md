# VDR-Suite – Status Overview

## Branch

```text
phase-2-actions
```

## Current milestone

The project has reached the first successful live VDR integration through `vdr-plugin-restfulapi`.

Validated stack:

```text
VDR
vdr-plugin-restfulapi
BasicHttpClient
RestfulApiVdrAdapter
VdrService
VdrOverviewService
VdrOverviewJsonSerializer
apps/vdr-probe
```

## Live validation environment

```text
RESTfulAPI version: 0.2.6.6
Host: 127.0.0.1
Port: 8002
HTTP status: 200
```

Observed live data:

```text
Recordings: 973
Channels: 342
Events: 37228+
Timers: 0
```

## Current focus

Return from architecture-only work to real VDR usefulness.

Priority order:

1. Recordings
2. Timers
3. Channels
4. EPG
5. VDR status

## Current technical direction

The next useful development step is to expose the already validated VDR overview through the real VDR-Suite REST/daemon path.

Target path:

```text
Daemon
ApiRouter
VdrController
VdrOverviewService
VdrService
RestfulApiVdrAdapter
BasicHttpClient
RESTfulAPI
VDR
```

## Guardrail

The current implementation may use one local RESTfulAPI backend, but this must remain an implementation detail.

No permanent single-VDR assumption should be introduced.
