# ADR-0068 increment 4 — Channel GET and lookup lifecycle boundary

Base `main`: `d3abf54f15175f1569ec59a49c4666f067a9f601`. Post-R3 source-only refactor; no productive host action.

## Exact extraction
- `PublicApiRuntime::tryHandleGet` keeps the audited `if (path == PublicChannelCollectionPath)` dispatcher and delegates to its new private `tryHandleChannelCollectionGet` method, **on the same runtime instance**.
- The complete original Channel GET branch is transplanted byte-for-byte into the helper, retaining parser, authorized multi-backend scope, duplicate rejection, `ch1_` cursor provenance, neighbor resolution inputs, source-state validation, service/Problem Details outcomes, and the original 200 response builder.
- `PublicChannelLookupRuntime.cpp` now defines the original four callback lifecycle methods (`registerChannelCollectionLookup`, `resetChannelCollectionLookup`, `channelCollectionLookupConfigured`, `lookupChannelCollection`) byte-for-byte with the same mutex and callback slot; daemon startup/shutdown registration remains unchanged.
- Shared response and cursor helpers remain in `PublicApiRuntime.cpp` to avoid duplicating serializers or creating a new security/identity owner. All standalone C++ tests and the daemon compile the new translation unit through `REST_PUBLIC_API_SRC`.

## Preserved contract matrix
| Path/method | Contract |
| --- | --- |
| GET `/api/v1/channels` | Explicit authorized `backendId` scopes, `backendId,channelId` ordering, `ch1_` cursor bound to scope, optional real VDR neighbor semantics, partial source metadata, no collection ETag, standard no-store/context headers |
| GET with invalid/missing/duplicate scopes | Existing 400 Problem Details; unauthenticated 401; cross-scope cursor 409 `cursor_expired`; all sources unavailable 503 `backend_unavailable` |
| POST and unsupported methods | Existing 405 with `Allow: GET`; unknown path dispatch and legacy paths unchanged |

The existing Channel runtime test covers pagination, scope-mismatch 409, invalid query 400, partial/unavailable 503, neighbor wrap, negative parsing and POST 405. This increment adds anonymous 401, missing/duplicate authorized-scope 400, and request/correlation-header assertions. Existing `SecurityHttpGate` source and tests retain all authorization and CSRF ownership.

**Not full ADR-0068 completion:** cursor/serializer extraction, Recording/EPG/Timer/SearchTimer, Accounts, Device and Media route responsibility boundaries still require audit and tests before E1. No R2/R3 hardware action or installation is repeated.
