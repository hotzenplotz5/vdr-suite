# PWA — Local-first, cache-backed Webfrontend

## Decision and scope (2026-11-19)

Maintainer direction: prioritize the PWA/local-cache modernization of the
existing VDR-Suite Webfrontend as the next frontend development workstream.
Ordinary browser use is the primary beneficiary; installation as a PWA is
optional. This is a non-numbered cross-cutting workstream, not Phase 70,
a separate Android application, a framework migration, or permission to
change unrelated MLD, HbbTV, playback or Media Tools behavior.

IPTVnator is the motivating comparison: a locally available web UI and
persistent client data are useful patterns. This is not a claim of identical
transport, backend architecture, workload or measured performance. Do not
copy its code or switch VDR-Suite to Angular. Recheck its current upstream
architecture if a concrete design choice depends on it.

## Evidence and existing architecture

The existing Home-performance work remains unaccepted in the real browser.
On 2026-11-19 the operator installed HOME.P7.1
(`7086e3e04e218a9676dea6a09325d2cde194e050`), passed
`test_phase66_home_inline_discovery.js` and the installed-file comparison,
but reported visible Series artwork missing even after minutes.

The subsequent initial-home diagnostic (75,526.9 ms observation interval,
mixed/warm cache) recorded 255 total observed Resource Timing entries,
15 Recording metadata reads, 178 image elements, 135 complete image elements,
123 lazy image elements, 4,374,719 image transfer bytes, three metadata-image
HTTP 404 entries and zero observed long tasks. The previous capture observed
8,774,871 image transfer bytes and 250 total resource entries; the earlier
baseline observed 640 entries including 384 metadata reads. These are
different samples, not controlled timing benchmarks. img.complete alone is
not proof of successful decode or visibility; lazily deferred offscreen
images need not load. The latest preview rows all request variant=home;
that parameter alone does not prove resized dimensions. Neither the browser
nor a server bottleneck has been isolated by these aggregate data.
A persistent cache cannot fix missing artwork, 404s or slow first delivery.

Reuse the existing server-owned SQLite/read models, canonical Recording and
backend identities, Home projection/render owners, server artwork preview
cache (core/media/ArtworkPreviewCache), existing Client API and shared SSE
change feed. Preserve [ADR-0019](../adr/ADR-0019-sse-event-stream-transport-strategy.md)
and [ADR-0067](../adr/ADR-0067-human-account-backend-access-administration.md).
The inspected Home branch has no manifest/service-worker registration in
index.html and no IndexedDB-based Home persistence in the inspected app.js
and Home paths; confirm the current tree before implementation. Never
introduce another domain authority or a parallel SSE stream.

## Target client architecture

- One existing UI for normal browsers and optional installed PWA; preserve
  layout, navigation, focus, prefix-aware URLs, localization and playback.
- Versioned app shell: manifest and a narrowly scoped Service Worker;
  existing deferred assets and server-composed bundles must remain coherent
  across updates. Authenticated HTML/credentials and arbitrary API responses
  must not be cached as public static assets. Verify actual serving, MIME,
  scope, HTTPS, proxy/subpath and package installation paths.
- IndexedDB: bounded, versioned, asynchronous, disposable client read-model
  snapshots (initially Home). Store only necessary successful projections,
  stable IDs, scope, schema and server revision/cursor. The server remains
  authoritative. Show valid cached data promptly while revalidating; never
  block normal online behavior on unavailable storage, migration or quota.
- Artwork: reuse existing HTTP caching and revision semantics; explicitly
  manage bounded Cache Storage or an equivalent browser-local image cache
  only for eligible successful authorized preview responses. Keep Home
  previews separate from detail originals. Do not persist errors, arbitrary
  URLs, streaming segments or unversioned images indefinitely; handle changed
  images/removals and eviction without destroying unrelated site storage.
- Loading priority: immediately render shell and known visible cards, prefer
  visible posters over offscreen work, keep incremental rendering and bounded
  requests. Missing images use genuine fallback; diagnose source/404/latency
  independently of cached-start performance.
- SSE: reuse canonical change events and cursor/reconnect semantics.
  Invalidate/re-fetch only affected domain/backend/IDs where the existing
  contracts permit; reconcile gaps once using authoritative snapshots.
  No new polling, duplicate stream, blind full reload, or blind write replay.

## Scope and account isolation from PWA.1

The current authenticated identity, authorized backend scope and grants are
authoritative. Design scope/cache boundaries before persisting private data.
Separate origin/installation, account, backend and where appropriate
permission scope; prevent cross-account leakage from Cache Storage, IndexedDB,
service-worker responses, concurrent tabs, login/logout, backend switching
and changed or revoked grants. Do not expose private cached content before
current authorization is established. Do not assume an offline device can
verify revocation; baseline offline behavior is a public shell and a clear
offline indicator, not unverified private data access. A private offline
policy requires an explicit separate decision. No cached login tokens,
passwords, CSRF credentials or generic mutation responses; no new
server-side grants, gates or permissions. Live operations, timer changes,
recording edits and playback retain their existing online contracts; no
offline write queue. Unavailable persistent storage must fall back to the
existing authenticated network path.

## Ordered implementation slices

| Slice | Scope and completion evidence |
| --- | --- |
| PWA.1 | Implement manifest and minimal versioned app-shell Service Worker with normal-browser progressive enhancement, scope/auth separation, update lifecycle, supported deployment wiring, and safe fallback. Test installed asset routing, subpath, stale bundle updates, no protected-response leakage, and absence of SW support. |
| PWA.2 | Introduce one small IndexedDB client persistence owner for scoped Home read models; test migration, storage failure, corrupt/partial snapshots, logout, account/backend changes, and concurrent tabs from the start. |
| PWA.3 | Bounded persistent Artwork cache and visible-first image delivery; reuse canonical preview URLs/revisions and server cache. Diagnose 404/missing posters and cold-cache delivery separately; test image version changes, failures, quota/eviction, logout and isolation. |
| PWA.4 | Wire cached Home projections into existing production lifecycles: Newly Recorded, Series, Genres, folders, Recent Movies, Continue Watching, then suitable EPG/Live Home data. Preserve current UI and live playback; stale EPG/current programme must be identifiable or refreshed. |
| PWA.5 | Wire existing SSE events, scoped invalidation and reconnect/cursor gap reconciliation into persistent snapshots. Test rapid events, duplicates, gaps, stale in-flight fetches, and changes across sessions without full Home reload. |
| PWA.6 | Offline/reconnect behavior: visible connection state, public shell when authorization unavailable, verified recovery, no offline write replay. Do not broaden private offline access without approval. |
| PWA.7 | End-to-end account/backend/grant isolation audit and cleanup regressions (not a deferral of security from earlier slices). |
| PWA.8 | Normal browser and optional installed PWA parity on Android and desktop; validate proxy/base-path, app updates and installability where supported. No separate native APK in this workstream. |
| PWA.9 | Controlled real-browser acceptance and regression closure; verify cold/warm/return, visibility, correctness, storage degradation, account switching, package/install and existing playback/SSE behavior. |

Account isolation and regression evidence apply throughout. Deliver coherent
incremental slices, not nine independent cache frameworks. No new branch
without the maintainer's explicit permission. New standalone work normally
starts from verified main under the existing branch lifecycle; the active
Home branch contains the unmerged optimization dependency, so explicitly
resolve integration rather than rebase, force-push or discard it. The
maintainer's kickoff is for PWA.1; no speculative feature expansion.

## PWA.1 — implementation checkpoint (2026-11-19)

The maintainer explicitly selected the existing `work/home-performance`
branch. The verified starting head was
`f54febbcba8419148e09ea216d2a76a8587c18f7`, 12 commits ahead of and
zero behind then-current main. Keep all Home fixes; no rebase or new branch.

- Manifest start URL and worker scope resolve relative to the existing
  public `/frontend/` route, including proxy prefixes. Registration
  is optional, asynchronous, secure-context only and uses the existing
  public URL owner. The legacy root URL remains online-compatible;
  PWA control is limited to the canonical frontend directory.
- A versioned Cache Storage entry contains only a generated account-neutral
  offline document. No fetched HTML, scripts, styles, session, API, SSE,
  artwork or media responses are persisted in this slice. The normal app
  shell and composed/deferred bundles keep the existing HTTP revalidation,
  preventing mixed cached bundle versions. This is the minimal public
  fallback shell, not a warm-start implementation or private offline access.
- Only a GET navigation to the canonical frontend or index can receive the
  fallback, and only on network rejection; HTTP errors and redirects stay
  server-owned. The worker does not claim running tabs or force updates.
  Old versions of this installation's shell cache are removed on normal
  activation. Bump the shell cache version when changing the fallback body.
- Existing HTTP static allowlist and install-runtime are extended; no new
  auth rule or SSE connection. Direct daemon delivery needs a new binary
  because the allowlist is compiled. The existing Nginx proxy path is unchanged.
- UNIT/CONTRACT: Node syntax and focused VM worker/registration test passed
  for root and prefixed deployments, rejected registration, storage failure,
  unauthorized response passthrough, isolated cache cleanup and exclusion of
  private routes. This is not a real browser. HTTP route regression and
  staging assertions are added; production C++/installation, hosted CI,
  production browser scope/update/offline/installability remain NOT VERIFIED.
- IPTVnator remains a motivating pattern, not verified feature or timing
  parity. PWA.2 IndexedDB and PWA.3 persistent artwork are intentionally not
  included; cold artwork errors and visible-first performance remain open.

## PWA.2 — IndexedDB owner checkpoint (2026-11-19)

Based on PWA.1 commit `c8d0d11b37f3d5af44f93aeee499147e99fa1c9e`.
VDR-Suite CI [#12883](https://github.com/hotzenplotz5/vdr-suite/actions/runs/44503089731)
passed all six jobs for that PWA.1 source, not real-browser acceptance.

- One asynchronous frontend platform owner (`platform/home-persistence.js`)
  with version 1 IndexedDB schema and disposable card projections. Origin
  isolation is native to IndexedDB; database name includes the public URL
  installation prefix, key includes account, backend, permission revision and
  rail. Store only complete snapshots with opaque revision, stable unique IDs,
  title and subtitle. No arbitrary response objects, passwords, CSRF, media
  URLs or artwork. Limits: 500 items, 256 KiB estimated serialized UTF-16 per
  snapshot, 32 snapshots per installation; oldest saved entries evicted.
  These are cache eligibility limits, not UI/API restrictions.
- API: create({authorize}), read(scope, rail), write(scope, rail, snapshot),
  invalidate(), clear(). Read/write expect a fresh current server-authorized
  exact-scope proof from the caller; local authentication boolean alone is
  insufficient. No proof means cache miss; no new server grant or access gate.
  A local generation fences account/backend transitions. Explicit clear
  purges this installation's snapshots and advances a shared transaction
  epoch, preventing a delayed operation admitted before clear from restoring
  old data. Clear failure is reported as false; no claim of physical deletion
  when storage is unavailable. Caller must still respect current server scope.
- The existing browser session snapshot exposes authentication status but
  not an account identity or permission revision; no guessed identity or
  new API assumed. The owner is loaded/registered but not yet instantiated or
  connected to private data. PWA.4 must bind existing authoritative session,
  backend and rail authorization/read paths and logout/invalidation before
  enabling persistence or cached presentation. PWA.5 owns SSE revision/gap
  reconciliation; opaque revisions are not ordered here. No polling.
- Node syntax and asynchronous test-double contract PASS locally: unavailable
  storage, denied opening, complete vs partial, credential field exclusion,
  scope separation, logout epoch fence across two owners, corrupt schema,
  eviction and newer database version fallback. Native Chromium navigation
  to the local test fixture was blocked by the execution environment
  (ERR_BLOCKED_BY_ADMINISTRATOR). Native quota/transaction/upgrade and two
  real tabs, production authorization and installed route remain unverified.
  Source integrates the regression into the existing PWA/frontend target and
  adds static routing and install assertions. No live deployment or rebuild.

## PWA.3 — persistent artwork checkpoint (2026-11-19)

PWA.2 base `5b508cd116bec78097e6c10477b415d18bbf5854` passed all six
[VDR-Suite CI #12884](https://github.com/hotzenplotz5/vdr-suite/actions/runs/44510409988)
jobs; this does not close real-browser IndexedDB acceptance.

- Reuse the canonical Home preview URLs, existing authenticated HTTP route
  and ArtworkPreviewCache. Successful browser-authorized canonical
  Recording/EPG metadata previews are tagged with a non-credential partition
  derived from the actual actor, backend and sorted current permission tuples.
  The decorator computes a validator from that partition, exact request path,
  content type and final served bytes (including preview fallback). Conditions
  are evaluated only after the existing authorized read; no authorization or
  new endpoint is introduced. Unsupported legacy artwork paths remain online.
- The existing worker owns a Cache Storage namespace per installation and
  partitioned entries per canonical URL and server-proved actor/backend/grants.
  It conditionally requests the current server on every reuse; a matching 304
  with the exact tag and partition can use persistent bytes. No offline private
  image fallback, no arbitrary URLs, tokens, detail originals, live media,
  failed or oversized responses. A 404/403/410 invalidates that URL; 401,
  observed logout and successful login/logout clear the artwork namespace.
  Serialized writes and a local generation fence prevent delayed persistence
  across a clear. Storage failure keeps the existing online response usable.
  Limits: 192 entries, 256 KiB each (48 MiB payload upper bound), FIFO by save.
- The existing discovery owner promotes up to 12 initially visible connected
  posters per animation-frame batch; offscreen posters retain native lazy
  loading. Inline genre/folder rendering reuses this owner. No polling.
- Node syntax and actual Worker fetch-handler contract with simulated Cache
  Storage/network passed locally for root/prefix, conditional reuse, version
  replacement, two partitions, 401/403/404, logout, storage failure, offline
  denial, unknown URL exclusion and eviction. Added focused visible-first and
  C++ decorator regressions. Production build, actual browser Cache Storage,
  native HTTP validators, grants/parallel tabs and cold/warm timing are not
  yet verified. The local Git checkout/network is unavailable in this session;
  hosted CI must establish integration. The HTTP changes require a single
  necessary incremental daemon build for eventual runtime acceptance.
- The three earlier 404s and slow first poster delivery remain open; their
  exact request-level causes cannot be determined from the prior aggregate
  samples. The worker never substitutes a stale image for a failed response.
  304 conserves transferred image bytes, not the server lookup/preview
  computation or RTT. No measured speed improvement is claimed. PWA.4 still
  needs the production Home snapshot/authorization binding.

## PWA.4 — scoped Home previews and authoritative readback (2026-11-19)

Base `cf18b04058528bd02f93408e4b3c0d6411736877` (PWA.3).
PWA.3 frontend regression, architecture, docs and inventory passed at
the initial PWA.4 checkpoint; full C++/packaging CI remains separate.

- Bind the existing Newly Recorded, Series, Genres, Folders, Recent Movies
  and Continue Watching owners to one runtime on top of PWA.2 IndexedDB.
  Each owner keeps its existing full online loading and rendering path.
  The cached first window (up to 24 cards) is a visibly labelled,
  noninteractive preview only. Fresh results win; no cached actions,
  recording details, playback positions, timer data or write responses.
  EPG/Live remain online-owned because current programme data is volatile.
- No new network request, endpoint, polling or authorization gate.
  The existing successful browser-authenticated backend snapshot and exact
  recording/history read routes return non-credential cache eligibility
  metadata derived from the actual actor, exact backend and current sorted
  grants. The existing AuthorizationService evaluates cache eligibility
  for recordings.view and media.recording.play; it cannot change the
  original request admission, status or body. Cache proof headers are
  private/no-store. Continue Watching only accepts its canonical list POST.
- The runtime accepts server proofs only for current generation and selected
  backend. Every page of a scan must carry matching proof before persisting
  its complete ordered preview window. Read proof expires after five
  seconds; transient previews expire after fifteen seconds at most and
  disappear when current results arrive, on error, scope mismatch, logout,
  backend change, offline or page hide. Browser-session changes purge local
  caches and notify sibling tabs with BroadcastChannel where supported.
  Persistent data never authorizes access, and known-offline presentation
  of private cached data remains disabled. Older servers/disabled storage
  simply use the online UI. Actual cross-tab/revocation latency still
  needs native-browser verification.
- Snapshot fields remain bounded: stable ID/title/subtitle and optional
  allowlisted canonical backend artwork URL, excluding unknown/external/
  credential-bearing URLs. The legacy list APIs do not provide a shared
  revision: the marker unversioned-preview-v1 denotes the preview contract,
  not a server revision. Rich late metadata and additional offscreen cards
  are not guaranteed to be in the persisted first window. PWA.5 owns
  incremental SSE invalidation/revision and gap reconciliation.
- Locally executed Node contract with IndexedDB double and exact source
  hashes: reload after fresh proof, live result wins, account isolation,
  mixed/missing proof, backend transition, logout, proof expiry and
  URL/credential exclusion; PWA.2 regression passed. Added HTTP scope
  contract and asset/staging assertions to existing CI. Production C++
  build, full rail regression, browser IndexedDB/worker and timings remain
  unverified until corresponding CI/real VDR evidence. No deployment yet.

## Measurable acceptance (provisional targets, not guarantees)

Compare baseline and candidate with the same authenticated account, backend,
inventory, browser/version, viewport and network. Record if authentication
time is included; use navigation time zero and separately measure shell
interactive, first meaningful rail, first visible successful poster and all
eligible visible posters (excluding genuinely missing images). Record network
TTFB/download, request count/bytes, cache source/hits, decode success, input
latency and errors. Compare cold cache, warm reload/new tab/browser restart,
warm Home return, direct normal browser vs diagnostic frame, Android
Chrome/Edge and desktop, home LAN vs remote if feasible. Do not force the
entire offscreen inventory to load as a performance target.

Warm authenticated target: shell usable in about 0.5–1 s; meaningful Home
with cached visible posters about 1–2 s (aim at no more than 2–3 s for eligible
visible images under agreed test conditions). Cold start target: progressive
usable UI without global artwork/metadata barriers and materially faster
visible poster completion than baseline; establish numerical cold targets
from a controlled baseline. No additional new polling. No reported
speedup until measured; no merge or accepted status based on docs or
synthetic tests alone.

Record UNIT/CONTRACT, production integration, relevant CI and real-browser
evidence separately. Preserve existing AGENTS.md rules; no CI wait after
each tiny commit, no main merge/Ready transition without explicit approval,
no unnecessary daemon rebuild or runtime restart for frontend-only code.
