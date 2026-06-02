# RESTfulAPI Integration Architecture

## Purpose

This document defines how VDR-Suite integrates with `vdr-plugin-restfulapi`.

The goal is to use existing VDR functionality instead of reimplementing it inside VDR-Suite. RESTfulAPI is one possible VDR backend behind the existing `IVdrAdapter` abstraction.

RESTfulAPI details must stay behind the adapter boundary.

---

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
├── RestfulApiVdrAdapter
└── future adapters
↓
VDR domain objects
```

The daemon and service layers must not know whether VDR data comes from RESTfulAPI, SVDRP, a mock backend, a plugin bridge, or another future backend.

---

## RESTfulAPI Role

RESTfulAPI is one backend implementation of `IVdrAdapter`.

```text
IVdrAdapter
├── ExternalVdrAdapter
├── MockVdrAdapter
├── RestfulApiVdrAdapter
├── SvdrpVdrAdapter
└── PluginBridgeVdrAdapter
```

The daemon, REST API controllers, dashboard services and recording services must never call RESTfulAPI directly.

All communication must pass through the adapter boundary.

---

## RESTfulAPI Adapter Boundary

```text
RestfulApiVdrAdapter
↓
IHttpClient
↓
vdr-plugin-restfulapi
```

`RestfulApiVdrAdapter` is responsible for:

* choosing RESTfulAPI endpoints
* requesting JSON through `IHttpClient`
* validating HTTP status codes
* mapping RESTfulAPI JSON into backend-neutral VDR domain objects

`IHttpClient` is responsible for the transport layer.

Higher layers must never receive RESTfulAPI JSON directly.

---

## Implemented Infrastructure

Implemented:

```text
IHttpClient
HttpRequest
HttpResponse
MockHttpClient
RestfulApiVdrAdapter
```

Future:

```text
RealHttpClient
real network communication
timeout handling
connection retry logic
```

---

## Implemented RESTfulAPI Mappings

Implemented as of Phase 8.18:

```text
/info.json       -> VdrStatus
/events.json     -> VdrEvent
/channels.json   -> VdrChannel
/recordings.json -> VdrRecording
/timers.json     -> VdrTimer
```

Implemented mapper classes:

```text
RestfulApiStatusMapper
RestfulApiEventMapper
RestfulApiChannelMapper
RestfulApiRecordingMapper
RestfulApiTimerMapper
```

---

## Relevant RESTfulAPI Endpoints

### Status and Capabilities

```text
/info.json
```

Implemented mapping:

```text
/info.json
↓
RestfulApiStatusMapper
↓
VdrStatus
```

Primary endpoint for VDR availability, current channel, disk usage, available services, plugins, devices and signal information.

---

### Channels

```text
/channels.json
/channels/groups.json
/channels/image/<channelid>
```

Implemented mapping:

```text
/channels.json
↓
RestfulApiChannelMapper
↓
VdrChannel
```

Channel groups and channel images remain future work.

Channel data must not be stored in `VdrStatus`.

---

### Events / EPG

```text
/events.json
/events/<channelid>.json
/events/<channelid>/<eventid>.json
/events/search.json
/events/contentdescriptors.json
/events/image/<eventid>/<imagenumber>
```

Implemented mapping:

```text
/events.json
↓
RestfulApiEventMapper
↓
VdrEvent
```

EPGSearch integration remains out of scope.

---

### Recordings

```text
/recordings.json
/recordings/cut
/recordings/marks
/recordings/play
```

Implemented mapping:

```text
/recordings.json
↓
RestfulApiRecordingMapper
↓
VdrRecording
```

Recording list mapping is implemented.

Playback control and recording operations remain future work.

---

### Timers

```text
/timers.json
/searchtimers.json
```

Implemented mapping:

```text
/timers.json
↓
RestfulApiTimerMapper
↓
VdrTimer
```

Search timers remain out of scope.

Timer creation, modification and deletion remain future work.

---

### Remote and OSD

```text
/remote/<key>
/remote/switch/<channelid>
/osd.json
```

Future work.

These endpoints may later be used for frontend control, remote key handling and OSD mirroring.

---

### Scraper Images

```text
/scraper/image/...
```

Future work.

These endpoints may later be used for metadata artwork integration.

They must not be mixed into `VdrStatus`.

---

### Wirbelscan and Femon

```text
/wirbelscan/...
/femon
```

Future work.

These endpoints belong to diagnostics and setup services, not to dashboard status.

---

## VdrStatus Boundary

`VdrStatus` should remain small.

Current fields:

```text
enabled
mode
host
port
state
```

Allowed future fields:

```text
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
search timer list
scraper metadata
OSD content
wirbelscan setup data
femon diagnostics
```

These belong into separate domain objects and adapter methods.

---

## Adapter Method Boundary

Implemented adapter methods:

```text
IVdrAdapter::getStatus()
IVdrAdapter::getEvents()
IVdrAdapter::getChannels()
IVdrAdapter::getRecordings()
IVdrAdapter::getTimers()
```

Current RESTfulAPI request mapping:

```text
getStatus()     -> /info.json
getEvents()     -> /events.json
getChannels()   -> /channels.json
getRecordings() -> /recordings.json
getTimers()     -> /timers.json
```

The adapter boundary must remain backend-neutral.

No service, controller, frontend or daemon component may depend on RESTfulAPI endpoint names.

---

## Mapper Boundary

Mapper classes convert RESTfulAPI JSON into VDR-Suite domain objects.

```text
RESTfulAPI JSON
↓
RestfulApi*Mapper
↓
VDR domain object
```

Mapper classes must:

* stay below the adapter boundary
* avoid leaking RESTfulAPI field names upward
* tolerate invalid JSON by returning empty results where appropriate
* avoid introducing a shared parser framework until repetition becomes a real maintenance problem
* avoid introducing an external JSON library before the architecture requires it

Current mapper style:

* small hand-written parsing helpers
* endpoint-specific mapper classes
* deterministic unit tests
* no real network communication

---

## Out of Scope

Still out of scope after Phase 8.18:

```text
real network communication
RealHttpClient
timer creation
timer modification
timer deletion
search timers
EPGSearch integration
OSD mirroring
remote control
recording control operations
diagnostic endpoints
shared parser framework
external JSON library
```

---

## Current Architecture Summary

```text
RESTfulAPI endpoint
↓
IHttpClient
↓
RestfulApiVdrAdapter
↓
RestfulApi*Mapper
↓
VDR domain object
↓
IVdrAdapter
↓
future services/frontends
```

This keeps VDR-Suite backend-independent and allows future SVDRP, plugin bridge or mock backends to provide the same domain objects without changing higher layers.
