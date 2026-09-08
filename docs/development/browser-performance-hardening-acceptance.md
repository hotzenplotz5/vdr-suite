# Browser performance hardening acceptance

Status: candidate under validation; real yaVDR/browser acceptance pending.

## Baseline and reproduced failures

Baseline: `8bdf508908454a43c3e00466d93037bb95f7dc17` on both main and the
`work/post-editing-browser-performance-hardening` branch, verified remotely
on 2026-09-08. Node.js used locally: 24.19.0 on Windows.

New assertions failed against the baseline:

- `test_phase66_series_return_scroll.js`: an unchanged detail projection
  detached the actual episode rail.
- `test_post_phase66_recording_discovery_performance.js`: same-backend
  revalidation detached the valid Series rail while metadata was pending.

Those regressions pass with the candidate correction. Existing progressive
tests use DOM doubles; their missing native `insertBefore` operation was added
to model the in-place reconciliation. No production fallback is added for a
test double. The unrelated inline-discovery timer test failed once locally and
passed on a focused rerun; hosted frontend CI must still be inspected.

## Automated gates

Run the existing `test-phase66-recording-discovery-frontend` surface. It includes
production composition, pagination, native metadata, navigation, Home retention,
warm reuse, interrupted work and backend/generation fencing. New assertions cover
retention while revalidating and stable season/episode rails during metadata updates.
Check syntax and diff whitespace. Hosted `frontend-regression-test` is required
before a targeted frontend installation; full CI is required before Ready/merge.

## Real-browser acceptance procedure

Record exact source commit, installed file SHA-256, browser/version, backend,
inventory size and viewport. Recreate the Home/Discovery owner after installing
through `install-phase66-recording-discovery-assets PREFIX=/usr`; no daemon
replacement, recording mutation or broad installation is required for this slice.

1. Load Home cold. Initial Series population must still be progressive. Scroll
   the rail well right, focus a card and retain its visible identity.
2. Allow an actual unsettled metadata retry (at least 65 seconds); record that
   the same rail, focus and horizontal position survive. Unchanged projections
   must produce zero subtree mutations; enrichment affects only relevant cards.
3. Open a Series and a season. Repeat metadata completion with those selections
   active. Keep the actual season/episode rail, horizontal position and focus.
   Open an episode and return through the canonical Recording detail callback.
4. Return to the Series list and verify its previous position. Exercise lower
   Home navigation and remote-control Home after leaving Home for over 65 seconds.
5. Trigger the existing explicit refresh. Observe retained valid UI while reads
   are pending, no loading replacement, and fresh authoritative results afterward.
   A transient failure retains UI but never marks it warm. A confirmed empty
   result withdraws old data. Backend changes must never display old-backend data.
6. Measure cold/warm cover requests, cache hits and transfer sizes, image decode,
   DOM changes, long tasks and input latency. Compare equivalent actions on the
   same inventory. Use these results to choose slices 2–4 in the plan.

Backend switch may be marked unavailable only with an explicit environment
reason; automated fencing coverage remains required. No synthetic result counts
as successful installed-runtime acceptance.

## Current environment gate

On 2026-09-08 the existing Edge tab at `https://yavdr/vdr-suite/frontend/`
showed `Nicht angemeldet`, backend loading `Failed to fetch` and no selected
backend. A read-only SSH connection to the known host `192.168.178.38` timed out.
No runtime installation was attempted. The next real-system gate requires an
accessible yaVDR host and authenticated working browser session, followed by
verified candidate installation and the observations above.

CI run identifiers and final candidate fingerprints are recorded after remote
publication and terminal CI results; none are invented here.
