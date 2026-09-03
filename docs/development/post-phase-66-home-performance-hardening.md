# Post-Phase-66 Home Performance Hardening

Status: **REAL-SYSTEM ACCEPTED ON DRAFT PR #265.** This is a bounded, non-numbered hardening block after Phase 66 and before any Phase-67 authorization. It does not reopen Phase 66 and does not start Phase 67.

## Boundary

The work keeps the existing Home Live-Hero, EPG, playback, artwork, metadata and Recording-discovery ownership unchanged. No parallel cache authority, metadata/artwork source, playback path or Phase-67 feature is introduced.

Recording Discovery was inspected as a further performance candidate. Its current refresh path can perform broad progressive Recording/Genre/Series/folder work on Home re-entry, but that owner also discards generation-bound progressive metadata work when Home is left. A blanket warm-cache shortcut was therefore deliberately not introduced without a separate correctness-preserving design.

## Accepted candidate

```text
pr=265
branch=work/post-phase66-home-performance
accepted_runtime_head=00d7245126c89b79216ece3004eca7ad59849dff
workflow=VDR-Suite CI
run_number=8577
run_id=33735740131
ci_result=PASS
```

VDR-Suite CI #8577 passed the complete six-job graph on the exact accepted runtime head: frontend regression, packaging/install staging, fast regression plus daemon build, architecture, documentation and Make/test audit.

## Implemented hot-path hardening

The accepted runtime head contains three bounded Home changes:

1. Pure Live-Hero left/right/touch browsing rerenders only the Hero projection instead of rebuilding both complete programme rails.
2. Canonical EPG events are indexed once per data change by channel id instead of repeatedly filtering and sorting the complete event list during projection lookup.
3. A same-backend Home return within 60 seconds reuses the already loaded canonical programme projection. The existing `Was läuft jetzt` / `Was läuft danach` rails remain mounted, their horizontal position is retained and no duplicate Channel/EPG request is issued for that warm return. Backend changes, explicit refreshes and real data resets retain their existing refresh semantics.

The warm-return lifecycle is protected by the production-style Home module-visibility regression: initial Home load performs one Channel and one EPG request; `Home -> another module -> Home` within the warm window remains at one Channel and one EPG request while preserving the selected programme projection.

## Real-system acceptance

The user executed the branch/head-specific yaVDR/browser acceptance blocks and reported:

```text
HOME_PROGRAMME_PAGING_SCROLL=PASS
HOME_PERFORMANCE_HERO=PASS
HOME_PERFORMANCE_WARM_RETURN=PASS
```

The observed acceptance therefore covers:

- programme-rail paging without a visible jump back to the start;
- rapid Hero browsing without programme-rail rebuild churn;
- Home return within the warm interval without visible EPG-rail teardown/reload;
- preservation of selected Hero state and horizontal rail position across the warm return;
- continued Hero browsing and programme paging after repeated Home/module transitions.

No new runtime defect was exposed by these acceptance gates.

## Remaining gate

PR #265 remains Draft. Ready-for-review and merge remain explicit user-approval gates under `AGENTS.md`. Phase 67 remains not started and not authorized by this hardening work.
