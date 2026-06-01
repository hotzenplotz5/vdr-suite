# RESTfulAPI Integration Architecture

## Purpose

This document defines how VDR-Suite should integrate with `vdr-plugin-restfulapi`.

The goal is to use existing VDR functionality instead of reimplementing it inside VDR-Suite. RESTfulAPI must be treated as one possible VDR backend behind the existing `IVdrAdapter` abstraction.

## Current Backend Architecture

```text
RuntimeConfig
↓
VdrConfig
↓
VdrAdapterFactory
↓
IVdrAdapter
├── ExternalVdrAdapter
├── MockVdrAdapter
└── future adapters
↓
VdrStatus
```

This architecture remains valid for RESTfulAPI integration.

## Target RESTfulAPI Architecture

```text
RuntimeConfig
↓
VdrConfig
↓
VdrAdapterFactory
↓
IVdrAdapter
↓
RestfulApiVdrAdapter
↓
IHttpClient
↓
vdr-plugin-restfulapi
```

The daemon and service layers must not know whether VDR data comes from RESTfulAPI, SVDRP, a mock backend, a plugin bridge, or another future backend.

## RESTfulAPI Role

RESTfulAPI is not a special daemon dependency.

It is a backend implementation of `IVdrAdapter`.

```text
IVdrAdapter
├── ExternalVdrAdapter
├── MockVdrAdapter
├── RestfulApiVdrAdapter
├── SvdrpVdrAdapter
└── PluginBridgeVdrAdapter
```

## Relevant RESTfulAPI Endpoints

### Status and Capabilities

```text
/info.json
```

Primary endpoint for VDR availability, current channel, disk usage, available services, plugins, devices and signal information.

This endpoint is the first candidate for mapping into `VdrStatus`.

### Channels

```text
/channels.json
/channels/groups.json
/channels/image/<channelid>
```

Relevant for future channel lists, channel groups and channel logos.

This data should not be stored in `VdrStatus`.

Future domain model:

```text
VdrChannel
```

### Events / EPG

```text
/events.json
/events/<channelid>.json
/events/<channelid>/<eventid>.json
/events/search.json
/events/contentdescriptors.json
/events/image/<eventid>/<imagenumber>
```

Relevant for EPG, search, metadata enrichment and frontend program views.

Future domain model:

```text
VdrEvent
```

### Recordings

```text
/recordings.json
/recordings/cut
/recordings/marks
/recordings/play
```

Relevant for synchronizing VDR recordings into the VDR-Suite database and for later playback/control operations.

Future domain model:

```text
VdrRecording
```

### Timers

```text
/timers.json
/searchtimers.json
```

Relevant for timer overview, timer creation, timer updates, timer deletion and search timers.

Future domain models:

```text
VdrTimer
VdrSearchTimer
```

### Remote and OSD

```text
/remote/<key>
/remote/switch/<channelid>
/osd.json
```

Relevant for future frontend control, remote key handling and OSD mirroring.

This must remain optional and backend-dependent.

### Scraper Images

```text
/scraper/image/...
```

Relevant for future metadata artwork integration.

This should not be mixed into `VdrStatus`.

### Wirbelscan and Femon

```text
/wirbelscan/...
/femon
```

Relevant for later diagnostics and setup tools.

This belongs to future diagnostic or setup services, not to the core dashboard status.

## VdrStatus Boundary

`VdrStatus` should remain small.

Recommended future fields:

```text
enabled
mode
host
port
state
reachable
backendName
backendVersion
lastError
currentChannelId
currentChannelName
currentVideoFile
diskFreeMb
diskUsedPercent
deviceCount
pluginCount
servicesAvailable
```

The following data must not be added to `VdrStatus`:

```text
full channel list
full EPG data
recording list
timer list
scraper metadata
OSD content
wirbelscan setup data
```

These belong into separate domain objects and adapter methods.

## HTTP Boundary

RESTfulAPI requires HTTP transport.

HTTP communication must not be implemented directly inside `RestfulApiVdrAdapter`.

A separate HTTP abstraction is required:

```text
IHttpClient
├── MockHttpClient
└── future real HTTP implementation
```

The adapter should depend on an HTTP interface, not on a concrete transport implementation.

## Recommended Future HTTP Types

```text
HttpRequest
HttpResponse
IHttpClient
MockHttpClient
```

Responsibilities:

```text
RestfulApiVdrAdapter:
- chooses RESTfulAPI endpoint
- requests JSON through IHttpClient
- maps response into VDR domain objects

IHttpClient:
- handles HTTP method
- handles URL/path
- handles status code
- handles response body
- handles timeout/error state
```

## Mapping Strategy

RESTfulAPI responses should be mapped into VDR-Suite domain objects.

RESTfulAPI JSON structures must not leak into service layers.

Correct:

```text
RESTfulAPI JSON
↓
RestfulApiVdrAdapter
↓
VdrStatus / VdrChannel / VdrEvent / VdrRecording / VdrTimer
↓
services
```

Incorrect:

```text
RESTfulAPI JSON
↓
services
```

## Error Handling Strategy

RESTfulAPI integration must represent connection and parsing problems without crashing the daemon.

Expected error categories:

```text
backend disabled
host unreachable
connection timeout
HTTP error status
invalid JSON
missing expected field
unsupported RESTfulAPI version
unsupported endpoint
```

For `VdrStatus`, these should map to:

```text
reachable = false
state = "unreachable" or "error"
lastError = descriptive error text
```

## Design Decision

Phase 8.7 validates the architecture only.

No HTTP communication is implemented in this phase.

The next implementation phase should introduce HTTP abstractions and tests first, before a real RESTfulAPI adapter performs network requests.

## Future Phase Proposal

```text
Phase 8.8:
- introduce IHttpClient
- introduce HttpRequest
- introduce HttpResponse
- introduce MockHttpClient
- add tests

Phase 8.9:
- introduce RestfulApiVdrAdapter skeleton
- inject IHttpClient
- map mocked /info.json response into VdrStatus
- no real network dependency yet

Phase 8.10:
- add real HTTP implementation
- keep adapter tests based on MockHttpClient
```
