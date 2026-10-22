# ADR-0068 — Single-branch Public GET responsibility extraction

This is the review log of the ongoing checkpoint, **not a claim of completed ADR-0068 or hardware acceptance**.

- Single branch: `work/adr0068-router-modularization-20261022` (created directly from `main` `c4fa7285384da8f0bac6d922993914bba296cfd3`).
- All changes remain on that branch in sequential fast-forward commits. **No branch may be created from this branch. No PR stack.** One draft PR is used for batched CI and later reviewed/merged only after the entire approved checkpoint is ready.
- Before these changes, `PublicApiRuntime::tryHandleGet` was approximately 2,181 lines. After domain GET delegation, the central method is approximately 347 lines.
- Byte-for-byte transplanted GET blocks: NativeTimer collection/item; SearchTimer draft preview, item preview and collection plus TimerAssignment collection/item; Recording collection, Recording/EPG search and Genre; Device Pairing collection, admin item and pairing-token polling; Account credential/session, Account/Device lifecycle/grants, Account item and Account collection.
- Byte-for-byte transplanted callback lifecycle: NativeTimer collection/detail and SearchTimer/TimerAssignment reads in `PublicTimerReadLookupRuntime.cpp`; Recording/Search/Genre reads in `PublicBrowseReadLookupRuntime.cpp`. The rest of the callback lifecycle remains on the existing runtime and is **not** silently marked extracted.
- Earlier merged ADR-0068 boundaries remain unchanged: Discovery, Backend, Channel, EPG read and the `ApiRouter` Recording action boundary.
- Actual branch dispatchers still check every original route-path expression in the original order. Helpers run on `*this` and retain the original parsing, authorization inputs, mutexes, lookup functions, cursor ordering, API status/error/headers, ETag/If-None-Match and fallback behavior. The established `SecurityHttpGate`, legacy API, timer/recording mutation executors, identities, databases and daemon startup/shutdown wiring have **not** changed.
- The two new helper source files are compiled by `REST_PUBLIC_API_SRC` for both daemon and standalone tests. Dedicated native Timer 200/304 headers and TimerAssignment/SearchTimer 200 headers are regression-asserted; the existing security, HTTP and contract tests cover the other GET branches.
- The exact original bodies were source-compared after the sixth implementation commit and all six inspected blocks matched, byte-for-byte. This is static source parity evidence, **not** a substitute for C++ compilation and CI.

**Remaining checkpoint scope:** POST/DELETE domain-dispatch segmentation where safe; callback/admission lifecycle ownership of Account and Device; final Public-v1/legacy route, method, security and contract matrices; relevant source/CI validation at the final single-branch head. E1 is not started. No productive yaVDR install or R2/R3 hardware replay.

## Subsequent POST and callback ownership checkpoint (2026-10-22)

- Existing branch/PR continued without branch stacking; sequential fast-forward commits after `f62d691`.
- Domain POST delegation introduced in `9b4ebf8`: six private POST methods in original match order, central dispatcher reduced from 2,666 to approximately 81 lines. No route body change; no extra mutation owner.
- OpenAPI route source guard strengthened for delegated method bodies and first-match order in `d032d5f`, with missing/reordered delegation negative tests.
- Callback lifecycle extracted to `PublicIdentityAdmissionLifecycle.cpp` and included by `REST_PUBLIC_API_SRC` in `768f9e2`, preserving byte-exact callback bodies. NativeTimer DELETE remains the only Public-v1 DELETE handled here.
- Older OpenAPI and P2 architecture path guards re-pointed/fixed in `21d6ac5`; the older CI snapshots from moving source lines are not release evidence. Full CI at the **final** head is still required.
- [Audited Public-v1 method/Legacy isolation matrix](adr-0068-public-legacy-contract-reconciliation.md) documents the OpenAPI partial inventory and the unmodified `ApiRouter` legacy POST families.

**Outstanding:** full final-head CI, source/test/security review of remaining guard failures, metadata/status closeout, main divergence inspection and careful merge. E1 remains blocked until complete. No yaVDR modifications.
