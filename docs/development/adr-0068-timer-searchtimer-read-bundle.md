# ADR-0068 — Bundled Timer/SearchTimer GET boundary

Baseline: `main` `c4fa7285384da8f0bac6d922993914bba296cfd3`. **Single non-stacked work branch:** `work/adr0068-router-modularization-20261022`. All changes belong to this branch and one final PR; no branches are created from it.

## Extracted boundaries, committed together

1. NativeTimer detail and collection GET remain on the original `PublicApiRuntime` instance, delegated to private `tryHandleNativeTimerReadGet`. The original two GET branches and original NativeTimer collection/detail lookup callbacks were moved byte-for-byte, without changing the `If-None-Match`, opaque revision/strong `ETag`, binding state, unbound timer, 200/304/400/401/404/503, paging, status or header mapping.
2. SearchTimer draft preview, resource preview, keyset collection, TimerAssignment collection and TimerAssignment item GET remain on that same instance, delegated to private `tryHandleSearchTimerAssignmentReadGet`. The original five GET branches and their existing Lookup/Preview callback lifecycle methods were moved byte-for-byte. `SearchTimerCreate`, native Timer modify/delete admissions, EPG Timer intent POST and all state mutation paths remain unchanged.
3. `PublicTimerReadLookupRuntime.cpp` contains the original read-callback lifecycle bodies (same mutexes, callback slots, registration/reset and shutdown wiring), linked through `REST_PUBLIC_API_SRC` in both production and standalone builds. `PublicApiRuntime.cpp` retains the existing parser, cursor and response helpers and the real source-dispatch markers for OpenAPI and Phase-69 architecture checks.

The execution path does not allocate a second timer/epgsearch repository, persistent search identity, backend adapter, security gate or idempotency executor. Source authority remains with the established services, including the SearchTimer retirement fix from PR #512 already merged into the baseline.

## Validation and non-changes

- Existing C++ tests: `test_public_native_timer_detail`, `test_public_epg_now_next` native collection portion, `test_public_timer_assignment_collection` (SearchTimer collection, previews, draft preview, TimerAssignment pagination), `test_public_timer_assignment_resource` (opaque ETag and If-None-Match) and related Public-v1 security/HTTP/daemon suites.
- Extended exact Cache-Control, X-Request-ID and X-Correlation-ID assertions for native Timer 200/304, TimerAssignment collection 200 and SearchTimer draft preview 200.
- All Public-v1 paths, method matching, JSON response order, Problem Details, malformed-query rejection, cursor scope, backend grants, If-Match/CSRF, legacy REST, registration lifecycle and failure fallback must compare equal.
- The final full CI gate is once on the **final branch HEAD**, then a single PR merge and post-merge confirmation. Earlier intermediate commits need not wait for separate CI completions. No productive yaVDR installation or destructive Recording/Timer test.

ADR-0068 as a whole remains open for Account, Device, Recording, Media and the closing route/legacy parity audit; E1 is still not started.
