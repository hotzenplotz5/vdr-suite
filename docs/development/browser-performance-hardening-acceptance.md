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

The user explicitly requested continuing without yaVDR access. Runtime
installation and real-inventory measurements remain pending under that scope.

## Controlled real-browser regression evidence

On 2026-09-08 Edge 152 on Windows, viewport 2552 x 1274, ran the production
renderer in `web/frontend/tests/browser-performance.html`. Source candidate:
`a0771064acd723fa9d7caaf21dd5d9097e63cebc`. The same fixture against baseline
`8bdf508908454a43c3e00466d93037bb95f7dc17` failed ten of 24 assertions; the
candidate passed all 24. Baseline failures covered insertion focus, list identity
and scroll after detail return, unchanged-detail mutations and summary-update
episode rail/focus/scroll retention. The existing top-list no-op optimization
already passed on main and is not claimed as a new optimization.

One 20-iteration sample per inventory size, synthetic poster-free cards, includes
forced browser layout (milliseconds; timings are descriptive, not a speedup claim):

| Series | Baseline cold | Candidate cold | Baseline no-op median / P95 | Candidate no-op median / P95 | No-op mutations, both |
| --- | --- | --- | --- | --- | --- |
| 100 | 4.1 | 7.0 | 0.1 / 0.2 | 0.1 / 0.2 | 0 |
| 1000 | 29.6 | 40.6 | 1.2 / 1.6 | 0.7 / 2.2 | 0 |

Single-card enrichment generated eight mutation records, all within that card.
The actual browser confirmed focus and scroll retention on insertion and detail
return, and zero mutations for an unchanged open detail. The sample does not
measure production cover transfer, decoding, HTTP cache behavior or input latency
and does not justify virtualization by itself.

Reproduce from the checkout with `node tools/serve_browser_performance.js`, open
`http://127.0.0.1:18765/web/frontend/tests/browser-performance.html`, and click
`Run browser regressions`. Append `?baseline=1` for the exact baseline. The server
only exposes the fixture and renderer on loopback, performs no backend requests
and stops with Ctrl+C. No runtime installation is involved.

The canonical metadata retry regression additionally exercises user-style
Series-card click, season-button click and the real retry timer, then verifies
preserved rails, scroll and selection without restarting Discovery queries.

## Completion coalescing regression

The focused lifecycle fixture leaves one non-representative native metadata
read pending after the first visible Home refresh. A second Home refresh on the
same generation previously produced two Series scans. The bounded correction
keeps one scan and one pending native read by returning the existing completion
promise. The projection becomes warm only after the tail settles. Explicit
refresh, Home-exit invalidation and backend change each still start fresh work;
the old completion cannot replace the new card or certify another backend warm.

These deterministic tests pass locally. They establish request coalescing for
that exact lifecycle gap, not a measured production latency improvement.

## Publication evidence

First product commit: `a0771064acd723fa9d7caaf21dd5d9097e63cebc`,
[Draft PR #270](https://github.com/hotzenplotz5/vdr-suite/pull/270).
Hosted `frontend-regression-test` passed on that commit in
[VDR-Suite CI #8856, run 34193007136](https://github.com/hotzenplotz5/vdr-suite/actions/runs/34193007136).
This is a completed frontend-job result, not a claim that the entire workflow
or the real runtime was accepted. Final-head CI is checked separately.
