# ADR-0068 increment 5 — EPG read GET and callback lifecycle

Base `main`: `687f99209781754f4b88bc32f94ea13da6bb242b` (PR #513 / Channel read merged). This is an exact-behavior extraction, not new functionality.

- Three existing GET branches for `/api/v1/epg/event`, `/api/v1/epg/window` and `/api/v1/epg/now-next` are moved byte-for-byte into the private `PublicApiRuntime::tryHandleEpgReadGet` on the original runtime instance. The central GET dispatcher retains its original three audited route markers, their order and their fallthrough when a future unrecognized callback status is encountered.
- The original EPG Now/Next, Window and Event Detail registration/configuration/reset/lookup methods are moved byte-for-byte to `PublicEpgReadLookupRuntime.cpp`. The instance's existing locks, callback fields, daemon registration and shutdown reset remain the sole authorities. The new C++ translation unit is included by `REST_PUBLIC_API_SRC` in all standalone tests and daemon builds.
- The existing strict single-backend grant provenance check, request parsing, optional native event ID projection, title/source validation, `If-None-Match`/strong `ETag`, HTTP 304, error statuses, response body ordering, request/correlation headers, `Cache-Control` and Problem Details remain unchanged. No new cache/identity authority, parser, backend fallback or serializer is introduced.
- Public Event Detail regression tests now assert exact 200 and 304 headers, ETag continuity, and unsupported HEAD 405/Allow; established Now/Next and Window tests already cover authorization, source-scope rejection, bounded queries and failing callbacks. OpenAPI and Phase-69 inventory guards continue to inspect the actual central dispatcher.
- POST EPG timer intentions, native Timer mutations, Recording workflows and legacy routes are outside this increment and remain untouched.

No productive yaVDR installation or hardware Recording/Timer acceptance is performed. ADR-0068 remains open until the remaining route responsibilities and complete security/legacy matrices have been reviewed.
