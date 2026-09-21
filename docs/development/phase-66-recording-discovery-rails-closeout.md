# Phase 66.5 — Recording Discovery Rails Closeout

Status: **COMPLETED. Original Slice-66.5 acceptance passed on `cd8133a814ba5325638fef0407915e294f8d125c`; the bounded canonical-Series membership regression follow-up passed on `53ee180e00f8ca5521468585acac2fb65a4f92d5`. PR #236 is merged; regression-fix PR #238 is the follow-up delivery vehicle.**

PRs:

- **#236 — Phase 66.5: Recording Discovery Rails**
- **#238 — Phase 66 follow-up: canonical Series rail membership**

This document records the bounded Slice-66.5 implementation, its original acceptance evidence, and the later bounded Series-membership regression correction. The regression correction is not Slice 66.7 and does not authorize any later Phase-66 slice.

## Scope

Slice 66.5 adds lazy, below-the-fold Recording discovery rails to Media Home by projecting already-existing Recording, Genre and hierarchy truth.

The accepted Home rails are:

- `Neu aufgenommen` from the canonical Recording query path;
- `Genres` from the existing Recording Genre domain;
- `Serien` only when the existing canonical Recording Genre domain exposes the `series` genre and returns same-backend Recording members for it;
- `Aufnahmeordner` from the existing Recording folder hierarchy.

The slice does not create a Home-only content catalog, Recording identity, Genre authority, metadata database, playback owner or navigation owner.

## Ownership and navigation

Recording cards retain explicit backend and Recording identity and open through the established `VdrSuiteRecordings2` owner. Folder cards enter the existing Recordings2 hierarchy. Genre cards enter the existing Genre browser. Home Live Preview is relinquished through its public owner surface before handing control to a Recording/Genre destination.

The discovery runtime is deferred below the initial Home viewport. Rail requests are isolated so one failing discovery source does not block the other rails or the primary Home experience.

## Canonical membership

`Neu aufgenommen` is bounded and ordered through the existing client Recording query contract using explicit backend scope and descending Recording start time.

`Genres` consumes the existing Recording-Genre projection and keeps only canonical non-empty genre entries.

`Serien` is fail-closed. It is attempted only when the canonical Recording Genre result contains the existing `series` genre. Membership is then defined by the canonical Recording-genre endpoint for `series`, with explicit matching backend and Recording identity preserved by Home. Optional provider fields such as `contentKind`, `seriesId` or `seriesTitle` are presentation enrichment only and are not a second Home-side membership authority. If the canonical `series` genre or canonical same-backend Recording members are absent, the Series rail is omitted rather than inferred from titles, folders or heuristics.

The bounded PR #238 regression fix removed the former Home-only second classifier that could discard valid canonical `series` members when the production genre-recordings response legitimately carried an empty `metadata.provider` object.

`Aufnahmeordner` consumes the existing Recording folder endpoint and preserves its hierarchy/path identity.

## Production composition and packaging

The Home discovery bootstrap is served through the established deferred-runtime mechanism. The production test HTTP server asset composition includes the bootstrap/runtime relationship, and the normal runtime install path stages both Slice-66.5 frontend assets.

Focused regression coverage is integrated into the ordinary frontend and packaging CI surfaces. It covers canonical membership/backend scope, lazy loading, failure isolation, established navigation owners and install staging. The PR #238 regression additionally executes the real Home `refresh()` composition and proves that canonical Series members render even without provider-series fields, while non-canonical heuristic-looking items and foreign-backend items do not gain membership.

## Candidate and acceptance evidence

The original immutable Slice-66.5 runtime-sensitive accepted product candidate is:

```text
accepted_product_candidate=cd8133a814ba5325638fef0407915e294f8d125c
source_ci_workflow=VDR-Suite CI
source_ci_run_number=8405
source_ci_run_id=33307208279
source_ci_result=PASS
real_system_acceptance=PASS
```

The bounded canonical-Series membership regression candidate is:

```text
regression_product_candidate=53ee180e00f8ca5521468585acac2fb65a4f92d5
source_ci_workflow=VDR-Suite CI
source_ci_run_number=8418
source_ci_run_id=33353771446
source_ci_result=PASS
real_system_acceptance=PASS
```

Hosted CI #8405 passed on the original exact candidate, including frontend regression, packaging regression, fast regression, Make/test audit, architecture and documentation jobs.

Hosted CI #8418 passed completely on the exact regression candidate `53ee180e00f8ca5521468585acac2fb65a4f92d5`.

Real yaVDR/browser acceptance of the original slice confirmed the below-the-fold discovery composition with real Recording cards, canonical Genre cards and Recording-folder cards. Real Android-browser acceptance of the bounded regression candidate then confirmed that the Home `Serien` rail appears with canonical The Walking Dead episode recordings after the canonical-membership correction, while the surrounding Phase-66 Home rails remain operational after loading. No title/folder/provider heuristic was introduced as an alternative Series-membership authority.

The follow-up UX idea to group episode recordings into selectable series cards with series artwork and then list the contained episodes is explicitly separate from this regression correction. It requires a truthful canonical series-level projection rather than deriving series identity from folder/title heuristics.

## Acceptance gate

Slice 66.5 and the bounded Series-membership regression correction are accepted because:

1. exact-head hosted CI passed on both runtime-sensitive product candidates;
2. the repository-derived runtime installation and deployed discovery assets were verified before browser testing;
3. real yaVDR/browser acceptance passed for the bounded Recording discovery composition and the later Series-membership correction;
4. Recording/Genre/folder membership remains owned by existing domain contracts;
5. Series membership fails closed and trusts canonical `series` membership instead of using Home-side inference;
6. existing Recordings2, Genre and Live Preview owners remain authoritative;
7. the correction does not introduce Slice 66.7 or later Phase-66 semantics.

PR #236 is merged. PR #238 carries only the bounded regression correction and its regression coverage. Any series-level grouping/artwork UX remains a separate bounded Phase-66 follow-up and must preserve the canonical membership and navigation ownership established here.
