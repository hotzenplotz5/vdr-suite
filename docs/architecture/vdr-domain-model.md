# VDR Domain Model

## Purpose

This document defines the backend-neutral VDR domain model used by VDR-Suite.

The goal is to keep VDR concepts independent from specific transport mechanisms such as RESTfulAPI, SVDRP, plugin bridges or mock adapters.

RESTfulAPI JSON structures, SVDRP protocol details and plugin-specific payloads must not leak into higher service layers.

---

## Architecture Boundary

```text
VDR backend
↓
Adapter implementation
↓
VDR domain objects
↓
IVdrAdapter
↓
daemon / services / frontends
```

Examples:

```text
RESTfulAPI JSON
↓
RestfulApiVdrAdapter
↓
VdrStatus / VdrChannel / VdrEvent / VdrTimer / VdrRecording
```

```text
SVDRP response
↓
SvdrpVdrAdapter
↓
VdrStatus / VdrChannel / VdrTimer / VdrRecording
```

The domain model is the shared language between adapters and the rest of VDR-Suite.

---

## General Rules

VDR domain objects must be:

* Backend-neutral
* Transport-neutral
* Suitable for tests and mock adapters
* Free from RESTfulAPI JSON field names
* Free from SVDRP protocol details
* Free from plugin-specific raw payloads
* Stable enough for service and frontend consumers

Domain objects should represent VDR concepts, not API response formats.

---

## VdrStatus

Purpose:

Represents the current runtime state of a VDR backend.

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

Must not contain:

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
plugin-specific raw data
```

Rationale:

`VdrStatus` is a small health and runtime summary. It is not a container for all VDR data.

---

## VdrChannel

Purpose:

Represents one VDR channel in a backend-neutral way.

Candidate fields:

```text
id
number
name
provider
group
radio
encrypted
enabled
```

Notes:

* `id` is a VDR-Suite domain identifier, not necessarily a raw RESTfulAPI id.
* Channel logo or artwork references should be handled separately.
* Channel groups may later become their own domain concept if needed.

Must not contain:

```text
RESTfulAPI raw channel object
logo binary data
EPG events
timer lists
scraper metadata
```

---

## VdrEvent

Purpose:

Represents one EPG event.

Candidate fields:

```text
id
channelId
title
subtitle
description
startTime
endTime
durationSeconds
contentDescriptors
parentalRating
```

Notes:

* `VdrEvent` is the base for EPG views, search results and metadata enrichment.
* Event images and scraper artwork should remain separate resources.

Must not contain:

```text
RESTfulAPI raw event object
scraper image payload
recording state machine data
search timer rules
```

---

## VdrTimer

Purpose:

Represents one concrete VDR recording timer.

Candidate fields:

```text
id
channelId
eventId
title
subtitle
startTime
endTime
priority
lifetime
enabled
recording
```

Notes:

* A timer is a concrete scheduled recording.
* It may reference a channel and optionally an EPG event.
* It is not the same concept as a search timer.

Must not contain:

```text
EPGSearch search rules
full EPG event object
RESTfulAPI raw timer object
plugin-specific timer payload
```

---

## VdrSearchTimer

Purpose:

Represents a future search-based timer rule, especially for EPGSearch integration.

Candidate fields:

```text
id
name
query
channelScope
enabled
caseSensitive
useTitle
useSubtitle
useDescription
priority
lifetime
```

Notes:

* A search timer is a rule that may generate concrete timers.
* It must remain separate from `VdrTimer`.
* EPGSearch-specific fields should be mapped into neutral concepts where possible.
* Fields that cannot be normalized should stay behind a plugin-specific boundary.

Must not contain:

```text
concrete timer state
recording runtime state
raw EPGSearch payload
RESTfulAPI raw searchtimer object
```

---

## VdrRecording

Purpose:

Represents one VDR recording known to the VDR backend.

Candidate fields:

```text
id
title
subtitle
path
channelId
eventId
startTime
durationSeconds
deleted
```

Notes:

* `VdrRecording` describes the VDR-side recording identity.
* Rich metadata belongs to the existing metadata services.
* Artwork belongs to artwork or scraper-related services.
* Processing jobs belong to the existing job and workflow services.

Must not contain:

```text
full metadata object
artwork binary data
job queue state
rectools processing state
RESTfulAPI raw recording object
```

---

## Plugin-Backed Resources

Some RESTfulAPI resources expose data from VDR plugins.

Examples:

```text
epgsearch
tvscraper
weather
muggle
wirbelscan
femon
```

These resources must not be forced into core domain objects if they are not generic VDR concepts.

Preferred strategy:

```text
generic VDR concept
↓
core VDR domain object
```

```text
plugin-specific concept
↓
plugin-specific service or adapter extension
```

Examples:

* EPGSearch search timers may map to `VdrSearchTimer`.
* TVScraper artwork should integrate with metadata/artwork services.
* Femon data should become diagnostics, not `VdrStatus` bulk data.
* Wirbelscan data should become setup/diagnostics, not channel state.

---

## RESTfulAPI Mapping Rule

RESTfulAPI endpoint responses must be mapped inside `RestfulApiVdrAdapter` or a dedicated internal mapper.

Allowed:

```text
RESTfulAPI JSON
↓
RestfulApiVdrAdapter
↓
VDR domain object
```

Not allowed:

```text
RESTfulAPI JSON
↓
service layer
```

Not allowed:

```text
RESTfulAPI JSON field names
↓
public VDR-Suite domain contract
```

---

## Future Implementation Order

Recommended follow-up phases:

```text
Phase 8.11:
- introduce VdrChannel, VdrTimer and VdrRecording structs
- add compile-level domain tests
- do not extend IVdrAdapter yet

Phase 8.12:
- extend IVdrAdapter with read methods for domain objects
- update MockVdrAdapter first

Phase 8.13:
- map RESTfulAPI channels through MockHttpClient tests

Phase 8.14:
- map RESTfulAPI timers and recordings through MockHttpClient tests
```

---

## Non-Goals

This document does not implement:

* C++ domain headers
* JSON parsing
* real HTTP communication
* RESTfulAPI endpoint calls
* SVDRP integration
* EPGSearch integration
* frontend APIs

It defines the domain boundary only.
