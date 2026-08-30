# Phase 66.5 — Recording Discovery Rails Closeout

Status: **COMPLETED. Real-system acceptance passed on the runtime-sensitive candidate `cd8133a814ba5325638fef0407915e294f8d125c`. PR #236 intentionally remains Draft; Ready-for-review and merge are not authorized.**

PR: **#236 — Phase 66.5: Recording Discovery Rails**

This document records the bounded Slice-66.5 implementation and acceptance evidence. It does not authorize Slice 66.6 or any later Phase-66 work.

## Scope

Slice 66.5 adds lazy, below-the-fold Recording discovery rails to Media Home by projecting already-existing Recording, Genre and hierarchy truth.

The accepted Home rails are:

- `Neu aufgenommen` from the canonical Recording query path;
- `Genres` from the existing Recording Genre domain;
- `Serien` only when the existing canonical Genre/Recording metadata truth proves series membership;
- `Aufnahmeordner` from the existing Recording folder hierarchy.

The slice does not create a Home-only content catalog, Recording identity, Genre authority, metadata database, playback owner or navigation owner.

## Ownership and navigation

Recording cards retain explicit backend and Recording identity and open through the established `VdrSuiteRecordings2` owner. Folder cards enter the existing Recordings2 hierarchy. Genre cards enter the existing Genre browser. Home Live Preview is relinquished through its public owner surface before handing control to a Recording/Genre destination.

The discovery runtime is deferred below the initial Home viewport. Rail requests are isolated so one failing discovery source does not block the other rails or the primary Home experience.

## Canonical membership

`Neu aufgenommen` is bounded and ordered through the existing client Recording query contract using explicit backend scope and descending Recording start time.

`Genres` consumes the existing Recording-Genre projection and keeps only canonical non-empty genre entries.

`Serien` is fail-closed. It is attempted only when the canonical Recording Genre result contains the existing `series` genre. Returned Recording rows are then required to retain explicit matching backend/Recording identity and canonical provider metadata proving `contentKind=series-episode` plus a series identity/title. If that canonical evidence is absent, the Series rail is omitted rather than inferred from titles, folders or heuristics.

`Aufnahmeordner` consumes the existing Recording folder endpoint and preserves its hierarchy/path identity.

## Production composition and packaging

The Home discovery bootstrap is served through the established deferred-runtime mechanism. The production test HTTP server asset composition includes the bootstrap/runtime relationship, and the normal runtime install path stages both Slice-66.5 frontend assets.

Focused regression coverage is integrated into the ordinary frontend and packaging CI surfaces. It covers canonical membership/backend scope, lazy loading, failure isolation, established navigation owners and install staging.

## Candidate and acceptance evidence

The immutable runtime-sensitive accepted product candidate is:

```text
accepted_product_candidate=cd8133a814ba5325638fef0407915e294f8d125c
source_ci_workflow=VDR-Suite CI
source_ci_run_number=8405
source_ci_run_id=33307208279
source_ci_result=PASS
real_system_acceptance=PASS
```

Hosted CI #8405 passed on the exact candidate, including frontend regression, packaging regression, fast regression, Make/test audit, architecture and documentation jobs.

Real yaVDR/browser acceptance was performed after exact checkout/install identity verification. The Android browser acceptance confirmed the below-the-fold discovery composition with real Recording cards, canonical Genre cards and Recording-folder cards. The accepted behavior keeps Series conditional: absence of a Series rail is valid when the canonical series evidence required by the slice is not present. No acceptance defect was reported after the final candidate was installed.

## Acceptance gate

Slice 66.5 is accepted because:

1. exact-head hosted CI passed on the runtime-sensitive candidate;
2. the repository-derived runtime installation and deployed discovery assets were verified before browser testing;
3. real yaVDR/browser acceptance passed for the bounded Recording discovery composition;
4. Recording/Genre/folder membership remains owned by existing domain contracts;
5. Series membership fails closed instead of using Home-side inference;
6. existing Recordings2, Genre and Live Preview owners remain authoritative;
7. Slice 66.6 and later Phase-66 semantics were not introduced.

PR #236 remains Draft. This closeout records Slice-66.5 product acceptance only; it does not authorize Ready-for-review, merge, Slice 66.6 or any later work.
