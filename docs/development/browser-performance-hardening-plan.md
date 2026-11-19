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

One bounded gap is reproduced without a runtime dependency: after the first
visible refresh resolves, tail metadata can still be running in the existing
`seriesCompletionInFlight` entry. Another same-generation `refreshForHome()`
started a second Series scan (two requests instead of one) and invalidated that
completion. Reuse the existing completion promise for coalescible Home scheduling.
Never reuse it for explicit refresh, backend changes or an invalidated Home-exit
generation. Focused tests cover all four paths, including late stale completion.

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

## Current measurement decisions

The real-browser fixture proves stable DOM, focus and scroll with 100/1000
synthetic Series and 100 episodes. Unchanged renders have zero mutations.
Single-sample cold rendering was 7.0/40.6 ms for those Series sizes; this neither
establishes real inventory sizes nor demonstrates a production virtualization need.
Keep virtualization gated on real inventory and interaction measurements.

Artwork audit: native Recording images reach
`VdrRecordingFolderController::getMetadataImage()` and
`EpgArtworkController::serveValidatedPath()` through the canonical metadata route.
The latter reads a validated local image and returns bytes without setting
ETag/Last-Modified/Cache-Control itself. This does not prove the headers delivered
by the full server/proxy chain. The Recording image URL is identity/kind/index
based, not content-versioned; blanket long-lived caching could preserve a changed
cover incorrectly. Measure the real delivery path and design invalidation in this
owner before changing cache policy. Do not add a browser artwork cache.

`createPosterArtwork()` already uses native `loading='lazy'`. The synthetic fixture
uses poster-free cards, so it cannot choose image priority or decode policy.
Real image/transfer traces are required for those changes. Newly Recorded, Genres,
folders and inline expansion have separate projection/render paths in the existing
Discovery and bootstrap modules. PR #266 already protects ordinary Home returns;
avoid duplicating its cache/navigation fix. Their remaining refresh costs require
separate production measurements and tests before changing those paths.

## Delivery gates

Small coherent GitHub commits, immediate fast-forward-only branch updates,
focused local regressions and relevant final-head CI. No rebase or force push.
Frontend installation requires passing focused tests and hosted
`frontend-regression-test`; use the existing repository install target only.
No production mutation is part of this workstream. Full CI and explicit user
approval remain required for Ready/merge. Finish runtime acceptance before
claiming that a browser-performance slice is accepted.

## HOME.P follow-up — 2026-11-19

The real initial-Home Resource Timing capture collected 640 observed resource
entries, including 384 Recording metadata reads, 47 script entries and zero
observed long tasks. The approximately 59.6-second capture interval is not a
measured time-to-complete-Home; exact request-to-rail attribution and startup
latencies remain unverified. The large metadata count motivates eliminating
avoidable HTTP reads before tuning DOM or adding infrastructure.

- HOME.P1 (code/test head `cd588c5301615f0c44bc164e0c133f1b7a51aa56`): reuse a
  cached canonical Series poster from another member of the same Series group;
  avoid an extra initial representative read while retaining detail enrichment.
- HOME.P2 (code head `389163f895966ab9d24a3242f704913043e4f1f0`): extend the existing
  page-scoped Genre index native summary with compact `nativeCardMetadata` for
  found non-manually-locked recordings, including movies. Reuse the existing
  portrait-first/preferred image rules and canonical image URL boundary. No
  filesystem paths, cast, full gallery or detailed metadata payload are exposed.
  Random Genre and inline Genre cards use this projection; only records without
  it use existing bounded metadata reads. The compact object does not populate
  the full-detail or Series metadata cache. Locked manual assignments, older
  schemas and missing native metadata retain the existing lookup path. No new
  endpoint, polling, artwork authority, or HbbTV change is introduced.

Regression coverage: compact native film/Series cards, backend scope, manual
precedence and older schema fallback in the Genre controller; mixed embedded and
non-embedded card lists in both Home Genre rendering paths. CI and real yaVDR
acceptance must be recorded separately. Newly Recorded and missing Series
metadata still have independent read paths. No claim is made that this slice
alone removes all 384 observed requests or meets the overall Home latency target.

### HOME.P5 — bounded inventory and foreground metadata priority

Retain immediate startup of the existing recording rails and progressive first
Series page. Fetch the remaining known-size inventory in ordered batches of at
most two parallel pages rather than issuing an unbounded Promise.all. Preserve
canonical order, complete-inventory publication and backend/generation fences;
unknown totals keep sequential pagination. The existing four-read metadata queue
prioritizes Newly Recorded over ordinary queued episode enrichment and explicit
Series detail interaction over queued representative work. Promotion reuses the
existing native-ID promise; active reads are not cancelled. Native lazy loading
and asynchronous decoding are set before assigning the recording poster URL.
No added timer, polling, viewport gate, API, cache or navigation owner.

Regression coverage: a 501-recording fixture observes only two concurrent tail
pages, out-of-order replies preserve canonical order, and a queued detail read
preempts queued background and representative reads without duplicate requests.
Actual transport and Home latency improvements still require browser acceptance.

### HOME.P6 — scoped SSE invalidation work

The existing backend-scoped recording feed remains authoritative. Coalesce exact
native IDs within the already scheduled microtask and share the fresh metadata
read promise between Newly Recorded and Series within that batch only. Subsequent
SSE batches never reuse the previous batch's fresh result. Skip duplicate
representative reads for those explicit IDs; retain full canonical membership
revalidation, because changed metadata may change the Series classification.
The Series pagination revision fence now prevents superseded inventory scans
from starting additional pages. Layout, keyed cards, sequence/resync behavior,
backend scope and broad inventory reconciliation are preserved. No additional
SSE connection, polling, timer or persistent cache. The shared reads still use
the existing direct exact-ID lookup; this slice does not eliminate the canonical
Series inventory scan or establish production latency gains.

Tests cover shared exact-ID metadata correction across both rails, same-tick
bursts, and cancellation of future pages after a newer revision.
