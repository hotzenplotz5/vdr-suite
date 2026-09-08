# Browser performance hardening plan

Status: implementation and measurement in progress. No real-runtime acceptance
is claimed. This post-editing workstream does not start Phase 67 or HbbTV.

## Verified baseline and contracts

Remote `main` and `work/post-editing-browser-performance-hardening` were both
verified at `8bdf508908454a43c3e00466d93037bb95f7dc17` on 2026-09-08 before edits.
PR #268 is merged at that commit. The full main `AGENTS.md` governs execution.

Read alongside this plan:

- [PR #265 Home contract](post-phase-66-home-performance-hardening.md)
- [PR #266 Discovery acceptance](post-phase66-recording-discovery-performance-acceptance.md)
- [PR #267 metadata acceptance](post-phase-66-series-metadata-artwork-acceptance.md)
- [Current acceptance gates](browser-performance-hardening-acceptance.md)

These older acceptances apply to their recorded candidates, not to this change.

## Ownership and correctness

Recording Discovery remains the Series projection, metadata queue and lifecycle
owner in `web/frontend/home-recording-discovery.js`. The canonical Client API,
metadata read model, artwork helper and Home navigation owner remain authoritative.
No additional cache, provider, request, artwork or navigation owner is introduced.
Browser responsiveness is a product requirement independent of a future native app.

Keep generation/backend/Home-active fences, four concurrent metadata reads,
settled-negative compatibility, the eight-ID/60-second unsettled retry and the
60-second fully completed warm projection contract. Interrupted, failed or partial
work must never be certified warm. Explicit refresh still fetches current data.

## Ordered slices

### 1. Series DOM and revalidation regression

The existing rail already uses keyed cards and presentation signatures. Extend
that implementation rather than replacing it with an older full renderer.

Reproduced defects: `loadSeries()` removes valid same-backend UI before a new
scan; `renderSeriesDetail()` replaces the entire section on every projection.
The existing list reconciliation also detaches every card for a membership or
order change, potentially dropping keyboard focus despite restoring scroll.

Preserve a valid same-backend projection during requests and transient failure.
Do not publish partial scans over it. Publish the completed authoritative result;
empty results still clear withdrawn data. Initial loading remains progressive.
Keep detail containers, keyed season/episode buttons, current navigation data
and unaffected artwork nodes. Reconcile actual membership changes in place.
Remember the list DOM/scroll while opening a Series and restore after mounting.
Do not reuse this presentation snapshot across backends.

### 2. Measure cover delivery and rendering

Measure the accepted Series slice in a real browser before changing scheduling.
Capture rail sizes, DOM mutations, render time, long tasks, request counts,
duplicate cover URLs, response cache headers, transferred bytes and image decode
cost for cold Home, warm return, offscreen rails and metadata completion.
The existing poster helper already uses native lazy loading. Review the existing
artwork HTTP/cache and Client API owners before tuning them. A synthetic DOM
benchmark is useful regression evidence but cannot establish real cover-cache
behavior, request priority or production inventory costs.

### 3. Warm state and coalescing

Use the measurements to isolate avoidable same-backend work in Newly Recorded,
Genres, folder discovery and inline discovery. Preserve PR #265 Hero/EPG/Recent
Movies and PR #266 navigation fences. Extend existing owner state only; no global
Home cache. Reuse only complete projections and coalesce only current generations.
Background revalidation must retain valid UI and focus. Validate each affected
surface independently before broadening the slice.

### 4. Large rails, only on demonstrated need

Compare real inventory rendering with input latency and layout/paint costs.
Introduce bounded incremental rendering or virtualization only if measured
costs still justify it after no-op and targeted-update fixes. Preserve keyboard
navigation, accessible order, selected Series/season, horizontal position and
canonical request boundaries. Do not infer a production need from synthetic size.

## Delivery gates

Small coherent GitHub commits, immediate fast-forward-only branch updates,
focused local regressions and relevant final-head CI. No rebase or force push.
Frontend installation requires passing focused tests and hosted
`frontend-regression-test`; use the existing repository install target only.
No production mutation is part of this workstream. Full CI and explicit user
approval remain required for Ready/merge. Finish runtime acceptance before
claiming that a browser-performance slice is accepted.
