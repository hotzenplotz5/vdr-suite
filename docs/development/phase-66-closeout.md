# Phase 66 — Media Home and Browse Experience Closeout

Status: **SLICE 66.8 ACTIVE / GOLDEN JOURNEY ACCEPTANCE PENDING.** Slices 66.1 through 66.7 are accepted and merged. No Phase-67 runtime work is authorized by this closeout.

This document is the durable evidence matrix for Slice 66.8 — Golden User Journey and Real-System Acceptance. Slice 66.8 is an integration, acceptance and closeout slice. It does not introduce a new media, playback, artwork, metadata, history, Recording or navigation owner.

## Closeout baseline

The verified `main` baseline at Slice-66.8 kickoff is:

```text
main_head=850d3dadf3163a86a71bf1246da2846824884f32
phase66_7_product_head=a5bf909d6b6580a61fb20d4e5058201da45d345f
phase66_7_merge_pr=262
phase66_7_merge_commit=850d3dadf3163a86a71bf1246da2846824884f32
```

The accepted Slice-66.7 product head has the complete hosted CI graph:

```text
workflow=VDR-Suite CI
run_number=8563
run_id=33722591860
head=a5bf909d6b6580a61fb20d4e5058201da45d345f
result=PASS
```

All six jobs passed: `packaging-regression-test`, `make-test-audit`, `architecture-check`, `fast-regression-test`, `frontend-regression-test` and `docs-check`.

Real yaVDR browser acceptance for Slice 66.7 passed on that exact product head. The comparison from `a5bf909d6b6580a61fb20d4e5058201da45d345f` to merged `main` `850d3dadf3163a86a71bf1246da2846824884f32` changes only `AGENTS.md`; no product runtime, frontend, packaging or media input changed. The accepted runtime evidence therefore remains valid under the repository evidence-reuse rule.

## Reused Phase-66 evidence

Accepted evidence is reused unless a directly relevant input changed. Slice 66.8 does not require repeating real-system tests already passed by an unchanged owner/runtime path.

| Area | Accepted evidence | Result |
| --- | --- | --- |
| 66.1 Home shell / responsive IA | PR #231; product head `a00c33c96cb650ce4de9aed69042b4449ffb7c3e`; VDR-Suite CI #8326 / run `33200350463`; real yaVDR marker `PHASE66_SLICE1_REAL_SYSTEM=PASS` | PASS |
| 66.2 Live-TV hero | PR #232; candidate `ce4ab17d58ecc8676b93430b3e5cfcdb6b56f5d3`; VDR-Suite CI #8337 / run `33204833908`; real-browser focus defect then corrected by PR #233 | PASS after correction |
| 66.2 keyboard-focus correction | PR #233; head `0663326baf6e1630516661d661d127d28c2c40cb`; VDR-Suite CI #8339 / run `33207700335`; correction models persistent real-browser focus ownership | PASS |
| 66.3 Deferred Live Preview | PR #234; final head `316bcf9fa6e754c3165132f0fef451fc40cfe2cc`; VDR-Suite CI #8354 / run `33225955077`; final commit records real-system acceptance | PASS |
| 66.4 Continue Watching | PR #235; accepted runtime candidate `d382928f84f72da3ac132d4da1e78039d6b7887f`; CI #8401 / run `33301330762`; real yaVDR/browser acceptance; closeout head `417a1ce98b947a63a1229b9b09158a98103f9059`; CI #8403 / run `33302847586` | PASS |
| 66.5 Recording Discovery Rails | PR #236; accepted product candidate `cd8133a814ba5325638fef0407915e294f8d125c`; CI #8405 / run `33307208279`; real yaVDR/browser acceptance | PASS |
| 66.5 canonical Series membership | PR #238; accepted candidate `53ee180e00f8ca5521468585acac2fb65a4f92d5`; CI #8418 / run `33353771446`; real yaVDR/Android acceptance | PASS |
| Series hierarchy follow-up | PR #239; accepted head `7660bc77da832940b174e42a9607e39bbb48ddc0`; CI #8425 / run `33386492769`; real yaVDR/Android acceptance | PASS |
| 66.6 Recently Watched / History | PR #237; accepted runtime candidate `6747682fd84f70c437937eb5311e72048593c73b`; CI #8412 / run `33334217608`; real yaVDR/Android acceptance including resumable-to-completed semantic split | PASS |
| 66.7 Visual Polish / Accessibility | PR #262; product head `a5bf909d6b6580a61fb20d4e5058201da45d345f`; CI #8563 / run `33722591860`; real yaVDR/Xiaomi Edge acceptance | PASS |

## Phase-66 completion matrix

| Requirement | Existing evidence | Remaining evidence | Required Slice-66.8 action |
| --- | --- | --- | --- |
| 1. Media Home is the accepted first-party landing experience | 66.1 production composition and real yaVDR acceptance; existing `overview` shell/navigation owner retained | One integrated Journey-6/7 observation that Home is useful before preview settles | Observe in final Golden journeys |
| 2. Desktop/tablet/mobile are responsive recompositions | 66.1 CSS/production regression covers distinct desktop, tablet, mobile and landscape breakpoints; real mobile evidence exists in later slices | Integrated supported-layout check at 1080p, 4K-class/equivalent, phone portrait and the materially different landscape composition | Perform only these viewport checks during final Golden acceptance |
| 3. Live-TV browsing does not wait for preview startup | 66.2 browse-only owner separation plus 66.3 deferred-preview fencing and real acceptance | One integrated rapid-browse -> settle -> resume-browsing observation | Observe in final Golden journeys |
| 4. Deferred preview uses canonical Phase-65 ownership and deterministic cleanup | 66.3 production ownership tests, exact-head CI and real yaVDR acceptance | No owner/lifecycle retest required; only observe start/relinquish as part of the journey | Reuse 66.3 evidence plus integrated observation |
| 5. Continue Watching is truthful and resumes canonical absolute position | 66.4 durable actor/backend/Recording truth, real exact-position acceptance and final CI | No deep semantic replay required; one ordinary Continue action inside Journey 6/7 is sufficient | Reuse 66.4 evidence plus integrated action |
| 6. Recording discovery rails use existing domain truth | 66.5 closeout, PR #238 canonical Series fix and PR #239 bounded hierarchy follow-up | One ordinary Newly Recorded/Genre/folder browse path in desktop Golden journey | Observe integrated navigation only |
| 7. Recently Watched uses explicit actor/history semantics | 66.6 closeout and real resumable/completed evidence | None for Journey 6/7; semantics unchanged | Reuse 66.6 evidence |
| 8. Keyboard/touch/focus/accessibility gates pass | PR #233 real-browser focus correction, 66.7 reduced-motion runtime coverage, full CI and real acceptance | Integrated desktop keyboard sanity and phone swipe/touch/nav sanity; no repetition of reduced-motion internals | Observe final Golden journeys |
| 9. Golden desktop and mobile journeys pass in supported environments | Component and slice evidence covers all owners and actions separately | **Missing: one coherent Slice-66.8 desktop journey and one coherent phone journey on the final accepted product runtime** | Mandatory real-system acceptance |
| 10. Full final-head CI including packaging/install regression passes | 66.7 product head has full six-job graph | Closeout/documentation final head must receive its own final CI before Phase 66 is declared closed | Run/record final-head GitHub CI after acceptance evidence is committed |
| 11. Rollback and provider/session/privacy boundaries remain intact | ADR-0058, accepted slice closeouts and CI ownership/security guards; this slice changes no runtime owner | Record closeout rollback and ensure no later-phase work enters the PR | Documentation only |

## Minimal remaining real-system acceptance

The missing evidence is intentionally integration-level, not a replay of every earlier slice.

### Desktop Chromium-family on real yaVDR

Use the accepted installed Phase-66 product runtime and prove the coherent desktop path:

```text
Home useful immediately
  -> rapid Live hero browse
  -> stable focus produces delayed preview
  -> browse again and observe obsolete preview relinquish/non-blocking focus
  -> explicit Watch Live
  -> return Home
  -> Continue one resumable Recording
  -> return Home
  -> browse Newly Recorded / one Genre
  -> open one Recording folder
```

The same desktop session must visually sanity-check a 1080p viewport and a 4K-class/equivalent viewport. This is a layout/interaction check only; it does not require replaying media acceptance twice.

### Real phone / actually supported mobile browser

In portrait, prove:

```text
one dominant Live hero + neighbor peeks
  -> swipe between channels
  -> Now/Next follows focus
  -> settled preview stays inside hero
  -> Watch Live and EPG touch actions are reachable/usable
  -> Continue Watching rail scrolls horizontally
  -> bottom navigation reaches Home / Live / Recordings / Search / More
  -> secondary navigation still reaches Timer / EPG / Settings
```

Because the production CSS has a distinct landscape composition, rotate once to landscape and verify that the Home hero/navigation remain usable without page-wide overflow or loss of the primary/secondary routes. The complete media journey does not need to be repeated in landscape.

### Acceptance result

Record the exact tested source/product relationship and a single result:

```text
PHASE66_GOLDEN_DESKTOP=PASS|FAIL
PHASE66_GOLDEN_MOBILE=PASS|FAIL
PHASE66_GOLDEN_ACCEPTANCE=PASS|FAIL
```

If a failure is found, Phase 66 remains open. Reproduce the concrete failing path, establish its production owner, and repair only the smallest root cause before repeating the affected portion.

## Deferred boundary

The known issue that visible Series metadata/artwork is not always projected as expected remains a post-Phase-66 follow-up unless it demonstrably blocks one of the Golden journey gates above. Its root cause is not established. Slice 66.8 must not invent a parallel artwork or metadata source.

## Rollback

Slice 66.8 introduces no runtime behavior. Before final acceptance is recorded, rollback is the documentation-only revert of the Slice-66.8 closeout/status commits. Accepted Phase-66 product runtime and all provider/session/history/privacy ownership boundaries remain unchanged.

If a later root-cause repair becomes necessary because Golden acceptance exposes a real defect, that repair requires its own explicit rollback note tied to the affected runtime input.

## Closeout rule

Phase 66 is **not completed yet** while the Golden desktop/mobile acceptance or the final closeout-head CI is missing.

After both Golden journeys pass:

1. record the exact real-system acceptance result here;
2. advance volatile/current status to Phase 66 completed;
3. run and record the complete final-head `VDR-Suite CI` graph including packaging/install regression;
4. keep the PR Draft until explicit user authorization to mark Ready-for-review;
5. do not start Phase 67 merely because Phase 66 becomes eligible to close.
