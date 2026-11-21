# Phase 66.6 — Recently Watched / History Closeout

Status: **COMPLETED / ACCEPTED. Real-system acceptance passed on the runtime-sensitive candidate `6747682fd84f70c437937eb5311e72048593c73b`. PR #237 intentionally remains Draft; Ready-for-review and merge are not authorized.**

PR: **#237 — Phase 66.6: Recently Watched / History**

This document records the bounded Slice-66.6 implementation and acceptance evidence. It does not authorize Slice 66.7 or any later Phase-66 work.

## Scope

Slice 66.6 adds durable, actor-scoped Recently Watched state for existing VDR Recording identity and projects that state as a separate `Zuletzt angesehen` Home rail.

The accepted semantic split is:

- **Continue Watching** remains truthful resumable Recording state.
- **Recently Watched** remains truthful recent Recording viewing activity, including completed playback when completion is known.

A Recording can therefore disappear from Continue Watching after completion while remaining in Recently Watched.

## Ownership and evidence

The slice does not create a second Recording identity, content catalog, player, MediaSession owner or recommendation authority.

History evidence is sourced only from the canonical Recording playback owner and is persisted under the server-assigned evidence value:

```text
canonical-recording-playback-owner
```

The browser observes the existing owner lifecycle, position/resume capability and actual media `ended` evidence. Home card clicks, DOM content, localStorage and browser wall-clock time are not History truth sources.

The accepted correction after the first real-browser test also preserves this ownership rule: returning programmatically to the canonical Home module now triggers a History-list refresh by observing the shell-owned active module-tab state. The DOM observation is only a refresh trigger; server-side actor-scoped History remains the data authority.

## Persistence and API

History is stored as one latest activity row per:

```text
(actor_id, backend_id, recording_id)
```

The bounded store retains at most 100 Recording identities per actor/backend. Server-generated activity timestamps own ordering, duplicate operation IDs are idempotent, unknown completion/resume facts remain unknown, and missing canonical Recordings are not exposed.

`POST /api/media/recently-watched` supports bounded `list` and `activity` operations and reuses the accepted Recording MediaSession authorization/CSRF boundary. Continue Watching and History mutations use independent queues so one failure cannot poison the other semantic.

## Home projection

The accepted Home projection:

- appears below the Recording discovery rails as `Zuletzt angesehen`;
- uses canonical Recording title, subtitle and preferred artwork;
- displays resumable state such as `Fortsetzbar · 1:34` when known;
- displays `Angesehen` after explicit completion evidence;
- opens the existing Recordings2 surface without inventing History-specific autoplay or playback ownership;
- refreshes after canonical programmatic return to Home;
- fails closed if History cannot be loaded.

## Candidate and CI evidence

The immutable runtime-sensitive accepted product candidate is:

```text
accepted_product_candidate=6747682fd84f70c437937eb5311e72048593c73b
source_ci_workflow=VDR-Suite CI
source_ci_run_number=8412
source_ci_run_id=33334217608
source_ci_result=PASS
real_system_acceptance=PASS
```

Hosted CI #8412 passed on the exact candidate, including frontend regression, packaging regression, fast regression plus daemon build, Make/test audit, architecture and documentation jobs.

The candidate includes the real-browser correction discovered during acceptance: the first implementation loaded History initially and on explicit Home clicks, but failed to refresh after programmatic return from Recordings2. The fix observes the canonical shell module-state transition solely as a refresh trigger and is covered by a dedicated regression test that proves a programmatic transition to `overview` issues the Recently Watched list request.

## Real-system acceptance

Real yaVDR/Android-browser acceptance passed after the Slice-66.6 assets were installed from the accepted branch state.

Observed acceptance evidence:

1. after Recording playback, a separate `Zuletzt angesehen` rail appeared below `Aufnahmeordner`;
2. Recording `1917` rendered with canonical poster artwork, title `1917`, subtitle `Zeit ist der Feind` and resumable label `Fortsetzbar · 1:34`;
3. after the Recording was allowed to reach its natural end, `1917` was absent from `Weiterschauen` while remaining present in `Zuletzt angesehen`;
4. the retained History item displayed the completed-state label `Angesehen`;
5. the programmatic return-to-Home refresh defect seen in the first browser attempt no longer reproduced after the correction.

This proves the required semantic separation between resumable state and durable recent viewing history on the real browser/runtime path.

## Acceptance gate

Slice 66.6 is accepted because:

1. exact-head hosted CI passed on the runtime-sensitive candidate;
2. the History persistence/API/security and production frontend composition are covered by ordinary repository tests;
3. the real browser demonstrated Recently Watched with canonical Recording metadata/artwork and persisted resume information;
4. the real browser demonstrated completed content disappearing from Continue Watching while remaining in Recently Watched as `Angesehen`;
5. the acceptance-discovered Home-return refresh defect was fixed and permanently regression-tested;
6. existing Recording identity, MediaSession/playback owner, metadata/artwork, actor and CSRF authorities remain intact;
7. Slice 66.7 and later Phase-66 semantics were not introduced.

PR #237 remains Draft. This closeout records Slice-66.6 product acceptance only; it does not authorize Ready-for-review, merge, Slice 66.7 or any later work.
