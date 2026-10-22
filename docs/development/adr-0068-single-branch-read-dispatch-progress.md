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
