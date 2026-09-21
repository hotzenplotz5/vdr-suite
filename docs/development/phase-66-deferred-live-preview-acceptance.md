# Phase 66.3 — Deferred Live Preview Acceptance

Status: **Real-system acceptance passed for the exact runtime candidate. PR #234 remains Draft and is not Ready or merged. Slice 66.4+ remains out of scope.**

## Scope

This record captures the mandatory real yaVDR browser acceptance for Phase 66 Slice 66.3 — Deferred Live Preview. It does not authorize Slice 66.4 or Phase 67+, and it does not change PR review/merge state.

## Exact runtime candidate

```text
branch=work/phase66-deferred-live-preview
runtime_candidate=8fe7d332b37dd8114209cd9dad15d79fde66d298
source_ci_workflow=VDR-Suite CI
source_ci_run_number=8353
source_ci_run_id=33223366613
source_ci_result=PASS
source_ci_jobs=6/6 SUCCESS
```

The exact runtime candidate was deployed on the real yaVDR host through the repository-owned installation path. Because Slice 66.3 changes the daemon static-asset registry, the daemon and frontend assets were installed coherently with `make install PREFIX=/usr` rather than by copying frontend files independently.

Installed identity evidence:

```text
vdr-suite-daemon=/usr/sbin/vdr-suite-daemon
frontend_root=/usr/share/vdr-suite/web/frontend
build_daemon_sha256=062402c9b3b313ee9e3d5bb46220a792d540cea8b273a9eafb73a4642599250a
live_daemon_sha256=062402c9b3b313ee9e3d5bb46220a792d540cea8b273a9eafb73a4642599250a
home_live_preview_sha256=0493e84889e860b6214fe66f4176939ecab4e0082040077d9f75e551219a443f
home_live_hero_sha256=dd8005d56020f962ec8e39e069605b1c2922acff0a4c05444c49946e33ac9ab9
channel_day_program_compat_sha256=d168ea1be17f125189c3fc08f49e56ed880f4e4ac7ea09e1912f2751ac885960
PHASE66_SLICE3_DEPLOYED_CANDIDATE=PASS
```

The daemon remained active after installation and after the browser acceptance. No warning-or-higher daemon journal entries were observed in the acceptance post-check.

## Real browser acceptance result

Final result:

```text
PASS_COUNT=14
FAIL_COUNT=0
NA_COUNT=1
PHASE66_SLICE3_REAL_SYSTEM=PASS
```

Accepted observations:

1. Rapid channel focus changes faster than the settle interval did not create visible intermediate previews and browsing remained responsive.
2. A settled channel focus started the preview for the currently focused channel.
3. A stale in-flight preview did not attach after focus moved to another channel.
4. An active preview was deterministically relinquished when browsing moved away.
5. Leaving Home relinquished the Home preview without leaving a hidden or floating stream behind.
6. Existing explicit full Live playback retained priority; Home preview did not displace the full-playback owner.
7. Preview to `Watch Live` promotion reused the canonical playback path without double picture, double audio or a competing duplicate session; full playback was audible and normal controls were restored.
8. Repeated keyboard navigation remained functional after preview starts, rerenders and relinquish cycles; Hero focus did not regress.
9. Desktop preview remained visually secondary to the browse Hero.
10. Mobile portrait integrated preview into the Hero without a persistent floating mini-player; browsing and `Watch Live` remained usable.
11. Mobile channel changes relinquished the obsolete preview and only the newly settled channel became current.
12. Real backend switching was not executable on this host because only one backend was available. This case is recorded as N/A rather than fabricated real-system evidence; automated Slice-66.3 regression coverage still exercises the backend fence.
13. With the Live preview stream deliberately blocked in Chromium DevTools, preview failure remained non-blocking: channel browsing, Hero focus and Home/EPG/navigation remained usable.
14. After removing the deliberate transport block, a new preview started normally again.

## Clarification of the deliberate failure test

The initial interactive acceptance run recorded item 13 as `FAIL` only because the confirmation question was answered `n`; it did not establish a product defect. The test was then repeated with the intended semantics understood: the preview stream was deliberately broken, and the acceptance criterion was that browsing remained usable despite that failure. The repeated test returned:

```text
PHASE66_SLICE3_TEST13=PASS
```

Recovery after removal of the deliberate block had already passed in the original run, so the final real-system result is 14 PASS, 0 FAIL, 1 N/A.

## Boundary after acceptance

This acceptance proves the Slice-66.3 runtime behavior for candidate `8fe7d332b37dd8114209cd9dad15d79fde66d298` together with hosted CI run #8353.

The documentation commit containing this record does not change runtime code and therefore does not invalidate the real-system observations for that runtime candidate. Hosted CI still has to be green on the documentation-updated PR head before any later review-state decision.

PR #234 must remain Draft unless the user explicitly authorizes a review-state change. No merge is authorized. Slice 66.4 Continue Watching and every later Phase-66 or Phase-67 runtime block remain outside the current scope.
