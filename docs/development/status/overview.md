# VDR-Suite – Status Overview

## Branch

```text
phase-2-actions
```

## Current milestone

The project has reached the first successful live VDR integration through the real VDR-Suite daemon HTTP path.

Validated daemon stack:

```text
curl
SimpleHttpListener
TestHttpServer
ApiRouter
VdrController
VdrOverviewService
VdrService
RestfulApiVdrAdapter
BasicHttpClient
vdr-plugin-restfulapi
VDR
```

This means the project is no longer architecture-only and no longer limited to the diagnostic `vdr-probe` CLI. A real HTTP client can now request VDR data from the VDR-Suite daemon.

## Live validation environment

```text
VDR-Suite daemon host: 127.0.0.1
VDR-Suite daemon port: 18080
RESTfulAPI version: 0.2.6.6
RESTfulAPI host: 127.0.0.1
RESTfulAPI port: 8002
HTTP status: 200
```

Validated endpoint:

```text
GET /api/vdr/overview
```

Validated command:

```bash
curl http://127.0.0.1:18080/api/vdr/overview
```

Observed live data:

```text
Recordings: 973
Channels: 342
Events: 36807+
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

The current daemon can expose the already validated VDR overview through a real local HTTP listener.

Validated path:

```text
Daemon
SimpleHttpListener
TestHttpServer
ApiRouter
VdrController
VdrOverviewService
VdrService
RestfulApiVdrAdapter
BasicHttpClient
RESTfulAPI
VDR
```

Near-term next steps:

1. Add basic diagnostics for daemon HTTP startup and request handling.
2. Improve listener shutdown behavior.
3. Add focused tests for SimpleHttpListener request/response handling.
4. Prepare CMake as a parallel build system once the current HTTP milestone is stabilized.

## Guardrail

The current implementation may use one local RESTfulAPI backend, but this must remain an implementation detail.

No permanent single-VDR assumption should be introduced.

Remote access, multiple houses and later multi-source frontend use remain compatible with the current direction because the VDR backend is reached through host and port configuration rather than hard-coded local-only assumptions.
