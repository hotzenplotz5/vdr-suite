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
    ├── RestfulApiVdrAdapter (future)
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

Future backend.

Responsibilities:

- Communicate with vdr-plugin-restfulapi
- Convert HTTP responses into VdrStatus objects
- Hide HTTP implementation details

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
