# Post-Phase-66 Series Metadata Artwork Acceptance

Status: **REAL-SYSTEM ACCEPTED ON DRAFT PR #267.** This is a bounded, non-numbered correctness completion after Phase 66 and before any Phase-67 authorization. It does not reopen Phase 66 and does not start Phase 67.

## Scope

This record captures the real yaVDR/browser acceptance for the bounded Series metadata/artwork completion in Recording Discovery. The change keeps the existing metadata, artwork, cache, Home and backend ownership boundaries intact. It introduces no new provider, metadata/artwork authority, cache owner or Phase-67 feature.

The public recording-native-metadata response now distinguishes retryable enrichment from an authoritative terminal result through `settled:false/true`. Home retries only explicitly unsettled Series metadata, with a bounded batch of at most eight recording IDs per 60 seconds. Existing responses without `settled` retain their compatibility behavior.

The retry reprojects the existing Series rail only. It does not refetch unrelated Home rails or perform a full Home rebuild. Backend and generation fencing remain authoritative. Leaving Home preserves a pending Series metadata retry without issuing metadata GETs while Home is inactive; returning to Home permits the bounded retry to continue for the same backend/generation.

## Exact runtime candidate

```text
pr=267
branch=work/post-phase66-series-metadata-artwork
runtime_candidate=3d9e35654594ad0a8b1d4a27b6056706824f77df
source_ci_workflow=VDR-Suite CI
source_ci_run_number=8621
source_ci_run_id=33802675434
source_ci_result=PASS
source_ci_jobs=6/6 SUCCESS
```

The exact runtime candidate was installed coherently on the real yaVDR host through the repository-owned installation path with `make install PREFIX=/usr`.

Installed identity evidence:

```text
vdr-suite-daemon=/usr/sbin/vdr-suite-daemon
frontend_root=/usr/share/vdr-suite/web/frontend
build_daemon_sha256=9c202c163e0eae4331b0c2c7554515b2e14deaa52390fd24a4a8135c436de89a
live_daemon_sha256=9c202c163e0eae4331b0c2c7554515b2e14deaa52390fd24a4a8135c436de89a
source_home_recording_discovery_sha256=8534f661adf001b36c8842acfc3c9f5784df1f1be022dd72ffb4be0a7bb0b8ee
live_home_recording_discovery_sha256=8534f661adf001b36c8842acfc3c9f5784df1f1be022dd72ffb4be0a7bb0b8ee
INSTALLED_RUNTIME_IDENTITY=PASS
```

The daemon remained active and listened on TCP port 18080. A direct anonymous request to `/api/vdr/health` reached the daemon and returned `401 authentication_required` with curl exit status 0. That response is the expected security boundary for an unauthenticated ordinary API request; browser acceptance therefore used the normal authenticated VDR-Suite browser path rather than treating anonymous HTTP 401 as a readiness failure.

## Real browser acceptance result

Observed on 2026-09-04:

```text
POST_PHASE66_SERIES_METADATA_ARTWORK_REAL_SYSTEM=PASS
```

The accepted observations cover:

1. Home loaded normally with the Series rail present while Newly Recorded, Genres and recording folders remained usable; the Series projection did not remain stuck in a grouping/loading state.
2. Existing Series metadata/artwork remained usable, missing or still-enriching metadata did not block Home, and completion did not require a visible full Home rebuild.
3. The production lifecycle path `Home -> another module -> wait at least 65 seconds -> Home` preserved the Series rail and allowed pending metadata completion after return without a manual browser reload.
4. Series navigation through series, season and episode views and back-navigation remained functional without a frozen or stale projection.
5. A repeated Home/module/Home lifecycle transition remained stable with no broken or empty Series rail and no manual reload requirement.

The daemon remained active after acceptance. No runtime defect was exposed by the acceptance gate.

## Hosted CI evidence

VDR-Suite CI #8621 passed the complete six-job graph on the exact accepted runtime candidate `3d9e35654594ad0a8b1d4a27b6056706824f77df`:

- documentation check;
- architecture check;
- packaging/install regression;
- Make/test audit;
- frontend regression;
- fast regression including daemon build.

The frontend regression includes the production Recording Discovery Home-exit owner transition and proves that a pending unsettled Series metadata retry survives Home exit, performs no metadata read while Home is inactive, remains fenced by backend/generation, and can complete after Home becomes active again.

## Boundary after acceptance

This acceptance proves the bounded post-Phase-66 Series metadata/artwork behavior for runtime candidate `3d9e35654594ad0a8b1d4a27b6056706824f77df` together with hosted CI run #8621.

The documentation commit containing this record changes no runtime code and therefore does not invalidate the real-system observations for that runtime candidate. Hosted CI still has to be green on the documentation-updated PR head before any later review-state decision.

PR #267 must remain Draft unless the user explicitly authorizes a review-state change. No merge is authorized. Phase 67 remains not started and not authorized by this bounded post-Phase-66 completion.
