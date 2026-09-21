# Post-Phase-66 Recording Discovery Performance Acceptance

Status: **The bounded post-Phase-66 Recording Discovery Home-return performance hardening is accepted on the real yaVDR runtime candidate. Series, Newly Recorded, Genre rails and Recording-folder discovery have been investigated in the authorized order. Phase 67 remains closed.**

## Accepted runtime candidate

```text
branch=work/post-phase66-recording-discovery-performance
runtime_candidate=1b4f2a9d7a528652cc46f8eb68dfbe93b406d80b
source_ci_workflow=VDR-Suite CI
source_ci_run_number=8610
source_ci_run_id=33769027825
source_ci_result=PASS
```

The exact runtime candidate passed the complete hosted VDR-Suite CI workflow. The frontend regression covered the existing Phase-66 Recording Discovery contracts, the post-Phase-66 Series reuse lifecycle contract and the canonical Home navigation propagation fence.

## Real yaVDR browser acceptance

The user reported the requested browser acceptance as **PASS** after installing the exact runtime candidate.

Accepted observations:

1. After Home had fully loaded, leaving Home and returning through the lower Home navigation restored the already-rendered Home state without visibly rebuilding or re-reading the Home Recording Discovery content.
2. Leaving Home and returning through the VDR remote-control VDR-SUITE/Home launcher restored the same existing Home state without triggering the independent Home data-refresh listeners.
3. Repeated lower-Home and remote-control-Home navigation kept the Home content stable without visible loading states or rebuild flicker.
4. The explicit refresh path remained available as the intentional way to request fresh data.
5. Backend-switch acceptance is **N/A on the current yaVDR host** because there is currently no implemented/configured "Backend hinzufügen" path and therefore no second backend available for a truthful real-system switch test. Automated backend-fencing coverage remains authoritative for that unavailable scenario.

## Separate rail investigations

### Series

Series was investigated first. The Series path retains the accepted progressive first-visible behavior while completed same-backend metadata/projection work can be reused. Interrupted, failed, stale-generation and backend-mismatched work is not certified warm. The real yaVDR acceptance above explicitly proved stable Series state across both Home navigation launchers.

### Newly Recorded

`loadNewly()` remains owned by the existing Recording Discovery `refresh()` path and continues to use the canonical Recording source. The production Home navigation fence prevents ordinary same-backend Home return from reaching the bubble listener that schedules another Discovery refresh. Therefore Home return does not re-run the Newly Recorded query; explicit refresh remains the fresh-data path.

A production-composition regression exercises the actual Discovery install listener together with the canonical Home capture fence and verifies that neither lower nor upper Home return schedules a Discovery refresh or invokes the Newly Recorded fetch owner.

### Genre rails

`loadGenres()` and the associated genre-recording path remain owned by the same existing Discovery refresh lifecycle. No parallel genre cache, metadata owner or refresh authority was introduced. The same production-composition regression independently counts genre and genre-recording requests and verifies that normal same-backend Home return invokes neither one.

### Recording-folder discovery

`loadFolders()` remains owned by the existing Recording Discovery refresh lifecycle and canonical Recording-folder API. No parallel folder owner or projection source was introduced. The same production-composition regression independently counts Recording-folder requests and verifies that normal same-backend Home return does not invoke the folder fetch owner.

## Evidence reuse after the runtime PASS

All commits after runtime candidate `1b4f2a9d7a528652cc46f8eb68dfbe93b406d80b` in this closeout step are documentation, regression-test or Make test-inventory changes. No installed runtime asset was changed. Therefore the accepted real yaVDR observations remain valid and are not repeated merely because their regression evidence was strengthened afterward.

The common root cause was Home navigation being allowed to propagate into independent Home data-refresh listeners. Fixing that production navigation boundary removed the unnecessary same-backend Home-return reload for the affected Recording Discovery rails together. Adding separate cache owners for Newly Recorded, Genres or Recording folders would duplicate ownership and is intentionally not part of this hardening.

## Scope boundary

This evidence closes the bounded post-Phase-66 Recording Discovery Home-return performance hardening work. It does not reopen Phase 66 and does not authorize or begin Phase 67. "Backend hinzufügen" is not implemented by this workstream and remains outside this acceptance scope.
