# ADR-0068 increment 3 — Backend GET dispatch and callback lifecycle

Baseline: `main` `608b9dc74955311e7235fdefe45ea09ecd5aa588` (PR #510 merged with all four post-merge workflows passing). This is a **partial, behavior-preserving Backend-domain separation**, not full ADR-0068 completion.

- `PublicApiRuntime::tryHandleGet` delegates the unchanged `/api/v1/backends` read branch to a private `tryHandleBackendCollectionGet` on the **same runtime instance**. The entire original authentication, duplicate-scope, query, cursor, authorization, monotonic ordering, paging, callback and response code is transplanted verbatim into the new private method; an unrecognized backend status retains its original fallthrough.
- `PublicBackendLookupRuntime.cpp` contains the original four `PublicApiRuntime` Backend lookup registration, reset, configured and callback invocation methods, unchanged, still referencing the original mutex and callback slot. No second collection service, cache, backend owner or security admission exists.
- `REST_PUBLIC_API_SRC` compiles and links the new translation unit in all production and standalone unit builds. The established Phase-69F architecture guard now reads the registration marker from its new file and keeps all existing Backend parsing, cursor, security and response assertions on the original runtime/Discovery sources.
- Existing Backend tests cover public scope-filtering, keyset cursor and changed-scope 409, wildcard, invalid query 400, unavailable 503, POST and other method 405/Allow, response exclusions and no-store/ETag. This increment adds anonymous 401 and duplicated-scope 400 negative response tests. `SecurityHttpGate` and other Public-v1/legacy routes remain untouched.

Backend parsing and response assembly currently still depend on shared anonymous-namespace helpers in `PublicApiRuntime.cpp`. Moving those by duplicating serializers or security/provenance logic is expressly deferred until a separate exact-response parity extraction can be shown to remain behavior-preserving. Do not count this as the entire Backend route decomposition.

No yaVDR installation, productive Recording mutation, schema change, permission change or R2/R3 hardware replay.
