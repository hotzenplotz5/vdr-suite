# VDR Backend Architecture

## Overview

```text
RuntimeConfig
        │
        ▼
VdrConfig
        │
        ▼
VdrAdapterFactory
        │
        ▼
IVdrAdapter
    ├── MockVdrAdapter
    ├── ExternalVdrAdapter
    ├── RestfulApiVdrAdapter
    └── SvdrpVdrAdapter (future)
```

## Design Goals

- Backend independence
- Testability
- Future extensibility
- Frontend neutrality

## MockVdrAdapter

Purpose:

- Testing
- Development
- CI

No external VDR required.

## ExternalVdrAdapter

Purpose:

- Generic external VDR integration

Acts as the foundation for future communication mechanisms.

## RestfulApiVdrAdapter

Implemented backend.

Responsibilities:

- Communicate with vdr-plugin-restfulapi through IHttpClient
- Convert RESTfulAPI responses into backend-neutral VDR domain objects
- Hide HTTP implementation details
- Keep RESTfulAPI JSON outside service and frontend layers

Implemented mappings:

- /info.json -> VdrStatus
- /events.json -> VdrEvent
- /channels.json -> VdrChannel
- /recordings.json -> VdrRecording
- /timers.json -> VdrTimer

Implemented mappers:

- RestfulApiStatusMapper
- RestfulApiEventMapper
- RestfulApiChannelMapper
- RestfulApiRecordingMapper
- RestfulApiTimerMapper

The rest of the application must not know whether RESTfulAPI is used.

## SvdrpVdrAdapter

Future backend.

Responsibilities:

- Communicate via SVDRP
- Convert protocol responses into VdrStatus objects

## Future Expansion

Additional adapters may be added without changes to:

- Dashboard
- Daemon
- Recording services
- Metadata services
- Frontend applications

Only the factory and adapter implementation need to change.
