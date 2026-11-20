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

## PWA.5 — persistent SSE synchronization (2026-11-19)

Base `3b3f083568eba82e330ea9b407d555df1f748074` (PWA.4).
PWA.3 frontend/packaging/docs/architecture/inventory passed; its fast regression
was cancelled, not a PASS. PWA.4 same five jobs passed; fast regression was
still running when checked. Browser timing acceptance remains open.

- One observer in the established shared /api/vdr/live EventSource, before
  the existing per-rail handlers. No extra transport, endpoint, polling,
  authorization rule, server change or new HTTP read. Parse the existing
  daemon epoch/sequence `lastEventId` and validate payload sequence.
  Ignore duplicate/older updates in the cache; an unexpected gap or changed
  epoch on an update reuses the owners' existing resync path. Explicit
  resync invalidates the affected baseline; ordinary transport errors dismiss
  cached previews and do not schedule polling. The browser's EventSource
  reconnect/replay remains the transport owner.
- Recording changes invalidate the six recording-dependent previews for
  that backend (including Continue Watching); Continue Watching-only
  changes invalidate only its rail, unrelated domains do not invalidate
  Home snapshots. A gap without a proven scope discards older snapshots
  across backends. Preserve newer same-epoch rows. Retire up to eight old
  daemon epochs in the bounded metadata store. One shared write floor and
  transaction epoch reject stale writes even from another owner; this
  conservatively fences in-flight writes for unrelated rails too.
- Canonical newly-recorded reconciliation now republishes its preview from
  the very same existing list read; Series and Continue Watching keep their
  PWA.4 readback bindings. A specific metadata-only event that does not
  cause a complete list read invalidates the old preview, rather than
  inventing membership or writing a partial snapshot. Genres, Folders and
  Recent Movies are invalidated and repopulate on the next normal owner read,
  not a new full Home refresh. Existing live tile updates remain unchanged.
- The stored revision is the observed SSE cursor, a causal cache fence,
  not a server resource revision. Initial resync requires a compatible
  cursor before a persistent preview may paint. A cold document uses its
  initial cursor as a conservative lower bound; unrelated missed events
  can therefore produce a cache miss. Requests sent before the first
  valid cursor are not persisted, and older unversioned PWA.4 previews
  are not reused. Network rendering never waits for this optional cache.
- Node contracts actually executed locally: PWA.2 persistence, PWA.4
  preview compatibility mode, and PWA.5 production cursor mode with an
  asynchronous IndexedDB test double. New cases: equal-cursor warm reload,
  backend and domain isolation, rapid/duplicate events, stale read/write,
  gap, disconnect/reopen, epoch change, retired epoch and logout.
  Added shared physical SSE dispatch regression to existing test.
  Exact locally tested blobs verified. Native IndexedDB transaction
  scheduling, real multi-tab delivery/reconnect and production composition
  are still to be verified; new hosted CI pending. No daemon rebuild or
  deployment performed for this frontend-only slice.

## PWA.6 — offline and verified recovery (2026-11-19)

Base `84124c9536707c1ac2a8a56fc2d079c34d9d0c78`.
PWA.4 hosted CI passed all six jobs; PWA.5 frontend/docs/architecture/inventory
passed at the initial check, fast/packaging still running.

- One connection-state owner in the existing pwa-register asset, available
  before app bootstrap also in ordinary browsers without a Service Worker.
  Bilingual, accessible connection status and manual retry. Real browser
  offline or failed current-session/bootstrap transport selects the public
  outage presentation and dismisses private cached preview work without
  granting offline access. Main content is concealed during that known
  outage; optional cache operations are suspended, not online API admission.
  Existing security rules, media and mutation contracts remain unchanged.
- A shared SSE error alone only indicates live-transport interruption; no
  private offline cache is thereby authorized, no extra probe, no reload.
  Existing EventSource reconnects as before. Online event, a recovered SSE
  open during a known outage, or explicit retry coalesce one fresh GET of
  the existing browser-session current endpoint (no-store, same-origin).
  Valid current response or canonical authentication-required/revoked
  response confirms server reachability, not access to private data.
  A failed/invalid response stays in outage presentation; stale completion
  after another offline event is discarded. One normal document bootstrap
  after verified genuine outage restores current account/backend/grants.
  This deliberately trades preservation of the selected view for a clean
  authoritative boot only after such an outage, not ordinary SSE churn.
  With no native online event/active SSE, manual retry remains available.
- Public fallback Shell v2: existing server-first canonical navigation,
  account-neutral message/link, one automatic navigation attempt on the
  native online event. Non-GET/live/media responses and HTTP errors retain
  the existing network contracts. No polling, background sync, queued
  writes, new server endpoint or credential persistence.
- Focused Node connection-state contract passed locally: no probe/reload on
  normal SSE churn, known offline, rejected verification, coalesced retry,
  loss during an in-flight check and one verified navigation. Added actual
  shared SSE/session API and worker shell assertions to the existing test
  targets. Native browser UI, modal/focus lifecycle, actual server outage/
  recovery, multi-tab and installed worker activation remain unverified;
  PWA.7 includes the end-to-end isolation audit. No daemon rebuild or deploy.

## PWA.7 — multiuser/cache isolation audit (2026-11-19)

Base `52bfe950633403a5bbfd9c037f77b6b1081e07b5`. The PWA.6 CI
[#12888](https://github.com/hotzenplotz5/vdr-suite/actions/runs/44522007568)
frontend, architecture, packaging, docs and make-test-audit jobs passed.
The fast job failed during Ubuntu dependency download (apt exit 100),
not in a VDR-Suite test. Do not call the entire run green.

- Audit found a late-response window: the previous IndexedDB fence was
  captured at storage access, not at the start of the HTTP request. An
  independent tab without the broadcast notification could acquire the
  post-clear epoch while still holding old authorization evidence.
  Capture a separate durable session epoch at request dispatch and carry
  it through the actual response proof, every accepted page, read and
  write transaction. Logout/grant invalidation advances that epoch in
  the same transaction as deleting snapshots. SSE-only invalidation still
  owns its separate epoch and cursor. Late or failed local storage work
  remains optional. Broadcast invalidation also dismisses sibling cached
  previews; a preview expires at most at its current proof's expiry.
- Server eligibility metadata for both Home and artwork now hashes the
  actual authenticated user, current browser session identity, selected
  backend and sorted resolved grants into an opaque cache partition.
  A prior login's partition is no longer reusable even for the same user
  and same grants. Ambiguous backend query identity stays outside the
  optional cache. The existing read grant is checked only for cache
  eligibility; the original route response/admission is unchanged. No
  credential or raw session identifier is exposed by the new partition.
- Worker artwork namespace v2 discards old v1 entries at normal activation,
  scoped to this installation. Suspend optional artwork reuse/writes
  around both ends of an existing login/logout/first-admin claim,
  including a failed request; an in-flight old 304 cannot serve the old
  persistent bytes after that transition. Require matching size/type
  metadata in addition to scoped server tag; unscoped Home previews
  still load normally but are no-store. Existing non-Home paths unchanged.
  Ineligible old IndexedDB partitions are inaccessible through the
  new scope, bounded and evicted normally or cleared on session cleanup;
  no claim of forensic erasure of browser storage.
- Focused actual-source JavaScript test harness (asynchronous repository
  IndexedDB double, simulated browser HTTP/Cache Storage APIs) passed
  new isolation and extended artwork contracts, including cross-owner
  late request after logout, new scope, backend/grant mismatch, expired
  proof, delayed 304 during logout, first-admin cleanup and failed
  mutation. PWA.4/PWA.5 focused source contracts also passed in that
  harness. Extended existing C++ scope/decorator and worker activation
  regressions; hosted Node and C++ integration still need CI evidence.
- Limits of this audit: no native browser transaction/worker scheduling
  or actual parallel browser session/revocation run yet. Broadcast
  arrival, already-rendered online content and a server grant change
  not yet observed by a client remain governed by the existing session
  and read lifecycle; instantaneous remote erasure is not promised.
  No additional permissions, polling, API route, merge or deployment.
  The server cache metadata changes will need a normal incremental
  build at eventual runtime acceptance, not a routine daemon rebuild now.

## PWA.8 — optional installed Android and desktop frontend (2026-11-19)

Base `21d2d78657acc97253a1a3eb52241959b03a0520`. PWA.7
[VDR-Suite CI #12889](https://github.com/hotzenplotz5/vdr-suite/actions/runs/44538762520)
passed all six hosted jobs. That does not constitute native browser acceptance.

- Same Webfrontend and server, not a separate native APK, Electron app,
  Play Store package or second UI. Manifest retains the relative installation
  identity/start URL/scope and standalone display mode. Add checked-in 192
  and 512 PNG icons, opaque and padded inside the maskable safe zone, raster
  adaptations of the existing icon-vdr-suite.svg (light background, no new
  brand). Preserve the regular browser favicon/logo. Serve exact PNG assets
  on the existing public frontend static asset owner; packaging installs
  and compares the exact files. No new API route or authorization rule.
- Optional localized install button below the header, only after the
  browser's beforeinstallprompt event in a secure context. A real user
  click invokes the browser's own prompt synchronously, at most once for
  that offer. Dismissed/rejected offers are not replayed. appinstalled and
  display-mode detection hide the control appropriately. Unsupported
  browsers retain normal operation and their own browser-menu options.
  No user-agent requirements, polling, confirmation gates, new storage
  or permission handling. Installation grants no access and does not
  alter account/session or cache-isolation logic.
- Retain the established network-first shell, scoped worker registration
  and normal browser worker activation/update schedule. Do not force
  skipWaiting, claim, reload or interrupt a playing recording when a
  deployment is detected. No additional worker or cached private shell.
- Actual-source focused JavaScript contract passed in a simulated DOM/
  install-event harness (button visibility, user activation, dismissal,
  failure, appinstalled, standalone, insecure/unsupported context and
  double click). Checked manifest identity and exact icon Git blobs/
  static paths; locally rendered and inspected PNGs. New Node manifest
  URL/PNG signature/dimension assertions and C++ HTTP asset regressions
  join existing test targets; full hosted CI/integration still pending.
  No deployment performed. Existing static asset route additions require
  an incremental daemon build for runtime acceptance, not a fresh rebuild
  now.

### Installation and device acceptance

Use the actual VDR-Suite HTTPS URL with a trusted certificate, either
direct or the configured reverse-proxy path (e.g. /vdr-suite/frontend/).
The same-origin prefix is part of installation identity. Do not move
an installed app to another origin or proxy prefix by changing id;
install there separately if desired.

On Android (Chrome) open that URL, sign in normally, select the available
"VDR-Suite installieren" control or the browser menu's install/add-to-home-
screen option. On desktop Chromium/Edge use the same control or the
browser's native install item; browser/platform support varies. Launch
from OS launcher, check standalone display and fresh normal authorization.
A regular bookmark/shortcut on an unsupported browser is not evidence
of installed standalone PWA acceptance. Direct untrusted HTTP normally
does not support this install flow; ordinary HTTP browser use remains
available subject to the existing transport contract.

Before PWA acceptance verify both browser and installed mode: identical
Home rails, Live TV/recording playback, search/EPG/timer navigation,
login/logout and backend switch; offline public fallback/reconnect.
Check manifest and both PNGs return 200 with correct types using the
actual deployment path; no root leakage through the reverse proxy.
Install direct and prefixed identities as appropriate, reopen warm,
deploy a changed frontend, close/reopen installed app and check the
normal worker update lifecycle without disrupting active playback.
Record device/browser/version and actual results in PWA.9. Neither
native installability nor performance has yet been proven on a device.

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

## PWA.9 — controlled acceptance run sheet (2026-11-19)

Product candidate: `d4ce4f7011da4995d40da9990180acbae8a50661` (PWA.8).
[VDR-Suite CI #12890](https://github.com/hotzenplotz5/vdr-suite/actions/runs/44550156488)
completed successfully for that candidate: frontend regression, fast
regression, packaging regression, architecture, docs and make-test-audit.
This is hosted regression evidence, not live installation or native browser
acceptance. The last product-code SHA remains the PWA.8 candidate for this
documentation-only update.

Use the existing `test-pwa-shell` target for focused reproduction when needed.
No new test runner, polling, or install prerequisite. Before runtime acceptance,
establish the actual installed daemon/source and reusable artifacts; the
candidate includes compiled HTTP route and cache-scope changes, so it is
not a frontend-only deployment. See the canonical runtime deployment contract.
Preserve the current installation, browser data and recordings.

| Case | Procedure and observable result | Live evidence |
| --- | --- | --- |
| Baseline | If the old installation is still available, record its source and Home timings before upgrading; otherwise mark the controlled baseline unavailable. | OPEN |
| Cold | Use a separate browser profile per candidate; authenticate normally, record shell usable, first meaningful rail, first visible decoded poster and last eligible visible poster. Record whether login is included. | OPEN |
| Warm | Same account, backend, viewport, network and profile; repeat normal reload, new tab, browser restart and Home return. Note first-window cache eligibility, cache misses and fresh-result replacement. | OPEN |
| Artwork | Separate visible successful decoding from lazy offscreen images, true missing artwork, HTTP errors, 304 revalidation and cache reads. | OPEN |
| Connection/SSE | Verify ordinary reconnect without document reload, public outage presentation, verified recovery and scoped updates; no new polling or write replay. | OPEN |
| Storage | With optional local storage unavailable, normal online Home still loads and remains usable. | OPEN |
| Isolation | With authorized test accounts/backends only, check logout, re-login, backend switch and two tabs; no foreign/private cached preview or artwork. | OPEN |
| Device and parity | Direct browser, Android and desktop installed mode where supported; check trusted HTTPS, actual proxy path, launch, update, search/EPG/timers, recordings and Live TV playback. | OPEN |

Repeat cold and warm observations three times where practical; report individual
runs and median, and preserve the original samples. Do not label a reload
as a cold cache. The existing optional `browser-performance-diagnostics.html`
can export supplemental JSON from its embedded diagnostic frame (Home neu
laden & messen; Messung stoppen; JSON exportieren). Its reload does not clear
storage, and its iframe/diagnostic shell is not equivalent to the normal
top-level or installed frontend. Verify those separately; zero reported
transfer bytes alone do not prove a Cache Storage hit. Do not clear the
operator's regular browser profile merely to obtain a cold sample.

Keep evidence by product SHA, device/browser version, viewport, network,
account/backend scope (non-identifying), scenario, timing, errors and result.
Do not record tokens or private content. Report unavailable cases explicitly.
PWA.9 remains OPEN until the actual runtime/device results are available;
no measured speedup, merge, or phase acceptance is claimed here.

## PWA.9 — operator runtime evidence and remaining performance deficit

On 2026-11-19 the operator built and installed PWA.8 product source
`d4ce4f7011da4995d40da9990180acbae8a50661` on yaVDR.
`DAEMON_INSTALL=PASS`, `FRONTEND_DEPLOYMENT_MATCH=YES`,
`RUNTIME_DEPLOYMENT_MATCH=YES`, and active daemon were reported.
Running binary SHA-256:
`84b8dd45ff1b2aac0c9f1de3931273592ff2c5c1c82b6f10fa419c9a84dbd0a7`.
The operator reported that warm reload and browser restart were not noticeably
faster, and some covers took many seconds to appear. PWA.9 performance
acceptance FAILED / OPEN; the video is an observed sequence, not a
controlled navigation-timing benchmark. Native Android/desktop installation
and the remaining real-browser cases are still open.

## PWA.10 — versioned, cache-first public app shell (2026-11-19)

Maintainer approved PWA.10 only. Existing branch and public static route
owner retained. The runtime installer generates the worker from the source
worker, its static-shell helper and SHA-256 of the actual staged public
index, CSS and eligible JavaScript routes, including the daemon's exact
two-file composition (separator and addon bytes). Any staged shell change
changes the generated worker identity. Source/uncomposed use retains the
existing network-first worker. No new HTTP path, daemon code or API contract.

The worker anonymously prefetches the exact static release with four
concurrent fetchers. Only successful non-redirected responses with expected
MIME, exact byte digest and non-private cache policy are admitted. The release
is usable only after complete prefetch; partial, rejected or unavailable
storage falls back to the ordinary network path. This is optional cache
eligibility, not an authorization gate. A canonical controlled navigation
and its static script/style/worker requests use the completed release locally;
diagnostic and legacy paths stay network-owned. Browser registration,
waiting and activation remain normal (no claim, forced reload or skipWaiting).
An update becomes usable after the normal worker lifecycle; prior active
clients retain their release. A failed prefetch remains optional/network-first
until a new worker installation; loss/eviction mid-page can require network
fallback. No private HTML, API response, cookie, artwork policy change,
polling, stream or queued mutation. Current session/bootstrap and SSE
remain authoritative; no private offline entitlement.

Local focused Node harness passed generator identity/bundle bytes, exact
public prefetch policy, zero warm network fetches for fixture shell resources,
direct and prefixed scope, unrelated cache retention, diagnostic isolation,
and storage/401/private/digest-mismatch fallback. The new helper, generator
and focused test blobs are byte-identical to the locally tested files.
The real production composition uses the existing install-runtime hook and
packaging compare is replaced by deterministic generated-worker verification.
Hosted integration, actual installed worker activation, security-header
behavior through the reverse proxy and browser timings remain NOT VERIFIED.
PWA.11 and beyond are not part of this implementation; artwork still uses
the existing PWA.3 revalidation path. No measured speedup claimed.

## PWA.10.1 — Random Folder Rail artwork correction (2026-11-19)

The operator identified the automatically selected Aufnahmeordner rail, not
the canonical Series overview: a Doku expansion displayed large text-only
child-folder cards. The existing inline folder renderer never created an
image; this is independent of PWA cache effectiveness.

The Home inline renderer now uses the established Recordings2 representative
traversal, the existing authorized folder API and the existing Home preview
URL/metadata projection. Embedded representative data is preferred; otherwise
visible folder cards request at most two one-recording folder pages each,
with three active lookups per expansion and no extra native metadata calls.
No recursive enumeration of an entire folder tree, no new service or polling.
A missing poster or failed lookup leaves a stable navigable fallback.
Folder labels are presentation-only; raw paths and click/back behavior remain
unchanged. Old expansion results are fenced by the current request/backend
and expansion identity. This is a frontend-only fix; the generated PWA
release must be rebuilt through the existing staging composition.

Extended the existing inline discovery contract to verify a representative
poster, the bounded folder read, raw path preservation and zero new metadata
reads. Execution of this exact candidate and real-browser validation remain
open at commit time; no measured performance improvement claimed.

## PWA.11 — locally served recording artwork after existing Home authorization

PWA.10 and PWA.10.1 were installed on yaVDR with the complete focused
PWA suite, inline discovery contract, generated worker release and runtime
deployment checks passing (operator output for `ca576eb`). Screenshot
shows recording posters in the random Oskar folder rail; the remaining
Klassiker child folder is not yet verified to have representative artwork.

PWA.11 reuses the existing successful, network-owned Home recording-scope
header in the service worker. The worker observes normal snapshot and
canonical recording-list/folder/genre GET responses, without a new API or
extra authorization request. A proof is in memory only, scoped to the
origin/installation, controlled client, backend, actual browser session and
grant-derived opaque partition. Its existing five-second Home proof window
is measured from request start. No EPG or Series settings artwork is admitted
to this fast path. No server or permission contract is changed.

After a matching fresh proof, a valid recording preview can be looked up
by the exact scope key in Cache Storage. A preview validated within five
minutes paints without another image HTTP request. Older validated bytes
(up to 24 hours) can paint immediately and refresh in the background
(maximum four deduplicated concurrent refreshes; saturated refreshes are
left for a subsequent use). Older/unstamped entries use the original online
conditional path; a successful 304 refresh records the validation time.
Revisionless artwork can temporarily show a previous picture during this
freshness interval. This is a bounded online presentation optimization,
not authorization for private offline access, and does not promise
instant server-side revocation detection between server checks. Logout/login,
grant/partition drift, denied scope reads, explicit offline and observed
network failure invalidate proofs; existing session-mutation cache clearing
remains. API bodies, streams and actions are never persisted. Missing proof,
storage, unsupported routes or outdated bytes retain the normal image path.

The existing worker test now exercises the production fetch handler for
fresh proof, exact scope and tab, expired proof, partition changes, background
refresh including 304, denial, offline reset and logout. Exact candidate test
execution, installed activation and real performance acceptance remain open
at commit time. PWA.12–14 are not implemented here.

## PWA.11.1 — artwork capacity and observable cache behavior (2026-11-19)

Operator installed PWA.11 at `662810c`: focused PWA contracts including
the extended artwork test, generated shell and runtime check all PASS.
The subsequent video and operator report still show slow poster population.
The live performance gate remains FAILED / OPEN.

The persistent preview cache now retains up to 1024 scoped entries instead
of 192 (at most 256 KiB each, i.e. 256 MiB of image payload at the configured
ceiling; actual browser quota and overhead vary). No cache version bump or
deliberate loss of existing entries. Insertion-order eviction uses a
worker-local key inventory reconstructed on first write, clear or storage
write failure, rather than enumerating all keys after every poster. The
existing serialized mutation queue, session clearing, scoped cache keys and
per-image eligibility remain. This is FIFO by successful storage/refresh,
not a new persistent LRU index. External browser eviction can reduce actual
occupancy until the worker reconstructs its inventory. No new server query,
permission change or cache-proof lifetime change.

The opt-in browser performance diagnostic now appends actual Service Worker
counts to its existing JSON capture: local hits, conditional hits, image HTTP
requests, proof reads/accepts, missing/expired proof, other artwork routes,
cache misses, old entries, server results, background refreshes, writes,
evictions and write failures. Counts are cumulative for that controlled
document since worker start, not exact timed deltas; a worker restart can
reset them. The diagnostic requests its own counters via a one-shot message
port, without identifiers, URLs, scopes or credentials. A missing worker or
old/unavailable diagnostics channel reports unavailable instead of blocking
normal operation. The embedded diagnostic does not measure top-level static
shell caching. No periodic polling or ordinary production UI change.

Tests extended for strict cache capacity, avoided full key scans, eviction
and the real worker/diagnostic message flow. Actual candidate execution and
real browser performance evidence remain open at commit time.

## PWA.11.2 — measured artwork proof expiry (2026-11-19)

PWA.11.1 installed at `5f00d76`; operator reports focused contracts and
runtime deployment PASS. Two diagnostic captures show 128/60 worker
requests, 26/6 local hits, 64/38 expired proofs, 106/55 image HTTP requests
and zero evictions or storage write errors. Capture duration is not
time-to-render; different image sets and cumulative counts cannot establish
a speedup.

The artwork-only optimization window is now 30 seconds from receipt of the
existing successful server proof, absolutely bounded to 60 seconds from
request start. The Home read-model proof remains unchanged. No additional
server query, polling, UI gate or permission. Existing account/session,
backend, grant and client scope, sequence/epoch fences, logout, denial and
offline behavior remain. Longer revocation observation latency between
confirmed reads is a presentation tradeoff, not instant revocation.
Expiry restores network delivery. Focused tests extended for delay and
expiry; execution and real browser acceptance remain open.

## PWA.11.3 — direct recording artwork preview cache (2026-11-19)

Both PWA.11.1 captures reported 45 direct recording-artwork resources and
1,885,683 transfer bytes on that route outside the old worker allowlist.
PWA.11.3 extends existing worker and router/decorator ownership to canonical
`/recording-artwork/<backend>/<32-hex-id>?variant=home` previews. Existing
admission and fallback stay intact: the router conveys the same opaque
session/backend/grants partition on its fallback only for an authenticated
browser with existing recordings.view on the exact backend. The artwork
owner resolves the asset and generates the scoped final-byte ETag and size
as for metadata previews. The request URL never supplies authorization.
Missing assets, unscoped or noncanonical previews, and full-size artwork
retain their original delivery. No additional permission, API, cache
version, provider, service or polling.

Product commits: PWA.11.2 `0072de3a1d24a7523670617cf8cb70badc547786`;
PWA.11.3 `62e37b8f7f4a9366916b837c928c2443f9e4ce30`.
The existing worker, HTTP cache-scope and artwork-decorator tests cover
delayed proof, exact canonical path, backend isolation, retained admission,
conditional responses, missing assets and unauthenticated cases.
These tests were extended, not yet executed on the exact candidate.
The C++ change requires a matching incremental daemon candidate and the
canonical generated frontend stage. Real browser performance still open.
PWA.12–14 remain separate.

## PWA.12 — persistent selected TMDB Series covers (2026-11-19)

PWA.11.2/11.3 were built and installed on yaVDR at
`62e37b8f7f4a9366916b837c928c2443f9e4ce30`. Operator reported
`test_test_http_server passed`, `BUILD=PASS`, `DAEMON_INSTALL=PASS`,
`FRONTEND_DEPLOYMENT_MATCH=YES`, `RUNTIME_DEPLOYMENT_MATCH=YES` and
`PWA113_INSTALLATION=PASS`. Subsequent subjective report: substantially
faster, still not comparable to IPTVnator. The second diagnostic capture
reported approximately 1.03 MB vs 2.95 MB previously, for different image
sets and capture durations; this is not a controlled loading-time result.
Real warm-start acceptance remains open.

The existing Home preview helper now opts selected TMDB series covers into
`variant=home`. Only the canonical selected cover image route, with
`seriesKey`, its positive persisted `revision` and the Home variant,
is eligible. The existing series cover owner reads its actual revision
with the selected local image path (no schema change). A successful
authorized browser response with matching revision can receive the existing
recordings.view session/backend/grant partition. Internal revision evidence
is consumed at the HTTP boundary; a stale or ambiguous revision retains
existing online behavior without cache admission. The shared preview
decorator resizes and computes the final-byte scoped ETag, and the existing
worker uses its scoped cache and current recording authorization proof.
Candidate search images, original/detail images and series identity,
season/episode hierarchy and provider selection remain unchanged.

The existing series settings POST is passed unchanged to its owner. The
worker fences old writes/proofs and selectively invalidates selected series
cover cache entries for that backend before and after that mutation, even
on ambiguous or failed outcomes. Ordinary artwork bytes are retained.
Remote/unobserved changes remain subject to the existing bounded cache
freshness and revalidation window; local cache is not an instant revocation
or invalidation channel. No new permission, API, polling, database, service
or cache version. Diagnostic resource details now classify selected series
artwork separately without exporting series keys, URLs or revision values.

Product candidate comprises the backend and frontend commits plus
diagnostic/documentation changes. Existing worker, Home rail, API owner,
HTTP scope, preview decorator and diagnostic regressions are extended.
Exact test execution, incremental daemon compilation and installed browser
acceptance are not yet verified. PWA.13/14 remain separate.

## PWA.13 — authorized Local-first startup (2026-11-19)

PWA.12 installed at `79d4dcaa32ebbd8b5db5f29ea36c02d7e4d0fae3`.
Operator reported focused test block, daemon build, installation and
runtime verification PASS (`PWA12_INSTALLATION=PASS`). Browser performance
and selected-cover behavior are still pending real acceptance.

During initial document startup only, the optional compact persisted Home
projection may paint immediately after the existing authenticated, scoped
response headers arrive, without waiting for JSON parsing or the first
SSE cursor. Backend selection and online authorization remain mandatory.
The preview uses the current account/session/grant partition, durable
session fence and a saved cursor, is at most 24 hours old when read before
the initial cursor, and is explicitly marked as last-loaded/updating.
It is non-interactive, never a playback or mutation authority.
The existing five-second proof/preview expiry is retained. An incompatible
first SSE cursor, connection error, logout, scope drift or owner
replacement removes it; no offline display and no speculative replay
after connection error. Normal authoritative network and SSE owners
continue in parallel. Durable replacement writes still require the
existing connected cursor/complete provenance; no new polling or API.
Read transactions are readonly instead of readwrite. Evidence arriving
during a pending read can trigger a bounded event-driven retry, not a
timer. Selected, revisioned PWA.12 series-cover addresses are eligible
for the bounded persisted card projection; candidate/foreign/unapproved
URLs are rejected.

Tests extend the actual cache runtime, IndexedDB test double and JSON client
for early authorized paint, pending body, scope rejection, initial cursor/
epoch mismatch, error, expiry, and selected-cover filtering. The startup
preview is still temporary and not a replacement for the live interactive
rail. No C++ or schema change. Candidate tests and real native browser
acceptance remain open until executed. PWA.14 remains separate.

## PWA.13.1 — persistent public logo resources (2026-11-19)

PWA.13 installed at `6187ae7b39a38c07cddeaaf641f8dc5332307bd7`;
operator reports scoped Home, SSE, isolation, artwork, frontend tests and
runtime deployment PASS (`PWA13_INSTALLATION=PASS`). Two subsequent
diagnostic captures reported 1,044,345/411,590 image transfer bytes;
the latter had 230 image elements rather than 159. Both observed 25
successful logo images (393,590 transfer bytes), a logo-route link
resource (392,424 bytes), and 32 normal missing-logo 204 replies
(9,600 bytes). Both also observed a 413,542-byte CSS-initiated EPG
resource; that is PWA.13.2, not this slice. Resource Timing aggregates
are not a network trace or a time-to-interactive measurement.

The existing HTTP logo route already serves anonymous public PNG/SVG
files before the security gate. Its unchanged successful lookup now
advertises public freshness for one hour, and its existing missing
204 result for five minutes (not 404/error responses). Existing root
priority, filename validation, image formats, alias lookup/fallback and
status remain unchanged. The worker caches these explicitly public
responses in a separate per-installation Cache Storage namespace, max
512 entries and 1 MiB per positive entry. It serves fresh entries,
including empty 204 status, locally without revalidation. It honors
remaining freshness/Age, bypasses explicit reload/range/auth/query
requests and refuses private, unapproved, redirected or other-status
responses. Expiry causes the original network lookup, so newly added
logos appear after the short negative window; normal browser hard
reload can bypass the worker policy. Public brand/genre PNG/SVG paths
share the same policy. No change to private artwork/session purge,
authorization, API, database schema, polling or logo sync workflow.
Older server and storage failure retain ordinary online delivery.

Extended existing shell tests for positive and negative cache reuse,
worker restart, both public base paths, expiry, sprite fetch, old server,
foreign paths, unsafe requests and storage failure; HTTP tests verify
the actual public headers and unchanged lookup. Exact test execution
and incremental daemon installation remain unverified. PWA.13.2
(EPG artwork), PWA.13.3 (critical path and timing), and PWA.14 (real
performance acceptance) remain open.

## PWA.13.2 — persistent scoped EPG artwork (2026-11-19)

Operator reports PWA.13.1 installation PASS at `c125d6cdfa9047e750879bd71a2d528025ee1b48`.
The observed 413,542-byte CSS-initiated EPG image exceeded the existing
variant=home / 256 KiB worker policy. The two existing EPG image routes now
admit canonical original URLs as well as explicit Home previews, using the
same final-byte validator and optional persistent artwork cache. Original
Hero/detail bytes are unchanged; explicit Home variants use the existing
preview owner. The 1 MiB EPG allowance covers the observed resource;
larger images retain ordinary online delivery. Recording limits stay
unchanged. The shared 1024-entry FIFO and cache version are retained; the
theoretical payload ceiling is 1 GiB if all entries are maximum-size EPG
images (actual browser quota may be lower).

Existing successful browser snapshot and EPG now-next/artwork-manifest,
window and metadata reads convey independent epg.view eligibility proof
for the authenticated account/session/backend/grant partition. No recording
proof authorizes EPG and no EPG proof authorizes recordings. Reuse the
existing 30-second receipt/60-second round-trip proof bound, five-minute
image freshness, 24-hour maximum stale age, deduplicated background
validation, online fallback, generation fences and logout/denial cleanup.
Existing route admission is unchanged. No new request, polling, API,
permission rule, storage namespace or cache purge. The existing authorized
404 compatibility fallback may convey the opaque eligibility marker only
to the actual successful EPG image owner; the 404 itself is not cached.

Extended existing worker and HTTP/decorator regressions for original Hero
and metadata images, independent grants, cross-tab, expiry, manifest proof,
background refresh, oversized/ambiguous requests and fallback delivery.
JavaScript syntax and a simulated request-grammar test are separate from
full Node, C++, native browser and yaVDR acceptance, which remain open.
IPTVnator remains the perceived-speed benchmark, not a verified comparison
of equivalent EPG workloads. PWA.13.3 and controlled PWA.14 timing remain open.

## PWA.13.3 — critical startup path and measured landmarks (2026-11-19)

PWA.13.2 source candidate `14b2f64e256374b18f2fb0ef23ad195801c14b6e`;
last operator-confirmed installation PWA.13.1. No new daemon changes here.
The existing Client API starts the canonical backend snapshot and matching
Recording-cache status in parallel, but previously awaited both before the
App could render and enable controls. Preserve both reads, count authority
and fallback; add an optional early callback for the authenticated snapshot.
The App renders once, shows a pending count rather than a misleading zero,
and patches only that metric after status settles. Backend/session changes
fence delayed results; a late count does not remount another module or Home.
The lightweight snapshot, Home cache, SSE and API routes remain unchanged.
No extra request, cache purge, new permission or poll. This removes one
response from the render barrier, not from total request volume.

Navigation-relative User Timing marks record session readiness, backend
selection, snapshot readiness, Home controls readiness, a two-frame render
opportunity in a visible document and count settlement. The existing opt-in
buffered diagnostic exports these and browser first paint/FCP where supported;
missing evidence is null. Controls-ready does not mean all rails/images are
loaded or Lighthouse TTI. The frame marker is not a compositor timestamp.
Compare fresh navigations with the same authenticated starting state; do not
mix session transitions or substitute capture duration for startup timing.
No identifiers are exported. Existing App/Client API and diagnostics tests
are extended for pending/failed count, module/backend changes, logout and
reported landmarks. Real browser performance and native test execution
remain open. Further metadata/request reductions require attributable traces;
no speculative request deletion. PWA.14 compares cold/warm browser/PWA
startup and functional parity against the perceived-speed goal (IPTVnator).

## PWA.13.4 — independent startup navigation and snapshot timing (2026-11-19)

Operator confirmed PWA.13.3 native JS tests and combined PWA.13.2/.13.3
installation `26149a16859fec25fd12c41ce889a7ee6cf59d67`: daemon build,
frontend and complete runtime verification PASS. First warm capture: first
contentful paint 292 ms, session ready 280 ms, backend selected 320.1 ms,
snapshot ready 5786 ms, controls ready 5786.3 ms; 39/40 worker-local artwork
hits and zero reported image transfer bytes. Capture duration is not startup
time; the precise HTTP/server source of the remaining wait is unverified.

The App selects a backend from the authenticated collection, dispatches the
existing snapshot/count reads before Home-resume discovery, and mounts Home
and enables ordinary module navigation without waiting for a metrics response.
Pending numeric metrics show ellipses, not invented zeros. Snapshot completion
updates just the snapshot metrics (or the snapshot fields of Settings);
a late or failed snapshot does not remount or clear an independent module.
Existing backend/session generation fences, server route authorization,
Home cache proof and SSE contracts remain unchanged. No extra read, new cache
or polling. Startup timing adds navigation-ready and snapshot-dispatch marks,
and exports a same-origin unambiguous Resource Timing breakdown where
available; request-to-first-byte includes network/server wait, not pure server
execution. Previous snapshot/controls timing retains its metrics-ready meaning.

Operator also observed that Recent Movies is slower. The existing owner in
home-recently-watched.js serially reads 100-item pages, filters and sorts the
entire result before rendering twelve cards. PWA.13.5 targets ordered server
pagination while preserving exact movie-year and release-date semantics;
PWA.13.6 targets progressive independent rails. Neither is implemented here.
No daemon rebuild for PWA.13.4. Native browser timings and PWA.14 remain open.

## PWA.13.5 — ordered Recent Movies pages (2026-11-19)

PWA.13.4 installed at `60c92fe13066d850dd3455fd1f2490ab5fcd75d2`;
operator-confirmed focused native Node tests, frontend and runtime checks PASS.
The existing Recent Movies owner used serial 100-item queries and only rendered
after the last response. The canonical recording query now accepts the optional
sort=movieReleaseDate order=desc on the existing authorized route. Its owner
retains the existing native year filter, applies locked manual metadata to the
same final eligibility and ordering before slicing, and serializes only the
requested page. Date and recording-start descending are preserved; exact ties
use deterministic bytewise title/id rather than browser-dependent German collation.
The frontend requests twelve items first and appends the next ordered page on
near-end scroll or a More Films button (also for non-overflowing wide rails),
with deduplication, same-rail DOM/scroll preservation, stale
response fencing and retry on subsequent scroll. Retained visible results are
not emptied on failed refresh. The existing authorized cache ticket persists
only the first complete requested projection (up to 24 cards), not arbitrary
API data. No new route, schema, polling or cache namespace. Server still reads
its existing catalog; no measured server CPU or latency reduction is claimed.
Install matching daemon before the new frontend; native C++/Node and real Home
acceptance remain open. PWA.13.6 and PWA.14 remain separate.

PWA.13.5 operator frontend run reached the ownership guard after the recent
movies runtime test passed. The guard still required the pre-PWA.13.3
two-argument snapshot signature. Updated the actual wrapper assertion to
recognize the optional callback and check the canonical request inside that
wrapper; no product code, route, ownership or permission changes. Native
full guard/installation still require confirmation.

Operator confirmed the corrected ownership and platform guards passed; the next
Home shell test still required the old synchronous count expression. Replaced
those string assertions with execution of the actual metric rendering owner
(pending snapshot, pending count, confirmed zero), and exercised canonical
module navigation before a snapshot and after logout. Product code unchanged.

## PWA.13.6 — intelligent rail loading (2026-11-19)

Operator confirmed PWA135_INSTALLATION=PASS at
`6ca56e8bdbb0524d8689ca5d4339a1baf9f5fb0c`: native remaining Home tests,
daemon build/install, staged frontend and runtime match, active service.
Frontend-only. Newly-recorded, history, genres and Series already render
independently; Series has bounded continuation after its first page. Keep
those semantics. The existing shared rail bootstrap schedules one optional
speculative read after 500 ms and a browser idle opportunity where supported
(not a network-idle guarantee). Skip hidden, offline, data-saving/2G and
unauthenticated contexts. Movies retain 12 initial cards and prefetch the
next 24 into a 30-second memory buffer, not extra DOM or poster reads. A scroll
or click coalesces with an in-flight prefetch or consumes the buffered response;
failed, expired or invalidated buffers fall back to authorized demand reads.
Use the existing Home-cache generation, scope and recording-domain fence;
no new durable data or SSE. Subsequent demand schedules one bounded lookahead.
Movie and EPG paging trigger up to one viewport before the edge (max 1200 px);
time-sensitive EPG does not speculatively load at startup. Folder categories
render from the first root page before remaining root recordings settle; only
full completion exposes the root group/count and persists a complete cache.
Keep valid categories if the tail fails. Artwork lazy loading remains intact.
No daemon rebuild. Actual browser performance and PWA.14 remain open.

The speculative fence also retires buffered reads permanently across an SSE
transport error/reconnect. Data Saver disables prefetch, not demand reads.

Operator PWA.13.6 run passed preceding recording discovery, Series, SSE,
metadata and navigation tests. The new early-folder assertion sampled after
one host microtask, before Node vm's cross-realm Promise adoption settled.
Wait for the event-loop turn with the second response still held pending;
assert the second request has started and only category tiles exist before
releasing it. No runtime or permission changes. Native rerun remains open.

## PWA.13.7 — random folder performance (2026-11-19)

Operator confirmed PWA136_INSTALLATION=PASS at `b3925f80a058e8f2b2cdf28e6f1ad6962f833f96` and reported responsive Home except the automatically expanded random recording folder. The legacy folder query limits direct recordings only; every child folder, including embedded single-recording leaves, was serialized and enriched, then the browser awaited all direct-recording pages. The optional existing route query `homeCards=1` slices child folders followed by direct recordings in the same catalog snapshot before embedding leaf metadata/serialization; total folder and recording counts retain their meaning. Home receives `homeCardPage`, `cardCount`, `returnedCards`, and `hasMore`; legacy requests remain unchanged. The random auto-open starts from the first authoritative root category page, requests 12 combined cards first, then optionally buffers 24 using the existing 30-second scope/SSE fence and shared one-in-flight speculative scheduler (two keyed pending owners). Scroll/click joins pending prefetch or uses a normal demand page; no speculative poster DOM, no new polling or durable cache. Existing manual non-auto folder navigation retains the legacy contract, nested browsing within the auto-opened rail uses the same combined pagination. Page-local display groups genuine folders before recording cards. Same rail stays mounted during append. No schema migration. Matching daemon and frontend required; runtime and actual performance acceptance remain open.

The paged expansion also keeps its loading/error notice separate from the
already mounted rail, uses the existing single bounded artwork coordinator
across appends, and drops speculative responses after collapse or session
logout. Automatic selection never displaces an already expanded folder.

## PWA.14.1–14.3 — inline folder artwork and layout (2026-11-19)

Operator confirmed PWA137_INSTALLATION=PASS at
`2528ef3e5fece361a7c17c7f4729b31c103af4ac`. The 29.6-second
screen recording shows existing Home rails already painted, followed by
recording-card artwork arriving visibly over several seconds after folder
changes and horizontal scrolling. The exact cold-image HTTP, preview
generation and decode timings are unverified. IPTVnator/Plex/Netflix are
perceived-speed targets, not equivalent measured workloads.

Reuse the actual Home artwork owner: one animation-frame batch prioritizes
visible inline-folder images (up to twelve promotions), accounting for
the scroll rail's visible clipping rectangle, before other newly created
posters. Up to four already-rendered nearby images per pass, within one
visible rail width (max 1200 px), become eager with low fetch priority;
previously speculative images are promoted when visible. Native browser
request concurrency remains authoritative. Data Saver, 2G, offline and
hidden documents suppress speculative promotion, not demand. No extra
metadata calls, detached poster requests, custom cache, authorization
change or changed 12/24 card paging. Other manual folder queries retain
their contract. Folder switching reserves the preceding expansion's
height until success or error; closing intentionally removes it.
Existing focused tests cover priority, rail clipping, scroll promotion,
bounded lookahead, constrained connections, detached owners, actual
folder-scroll wiring and loading-height release. Actual full native
frontend regression and real browser performance remain open; no
daemon rebuild is required by these changes. No measured speedup or
final PWA.14 acceptance claimed before the operator's repeat test.

### PWA.14.4 — bounded server-side recording artwork preparation

Operator confirmed PWA14_INSTALLATION=PASS for `aee493c7cf63c0e714f15418603ce8cb21b85671`
including the native frontend regression and PWA shell verification. The repeat
smartphone test reports very fast warm browsing but several seconds for
some cold covers; browser vs server cache, original image reads and decoder
latency remain distinct and are not yet measured independently.

The inspected request path previously ran FFmpeg on a cold preview miss
(up to three seconds wall time); concurrent misses fall back to original
bytes instead of queuing. The existing private preview cache now keys new
derivatives by exact source content and MIME, independent of HTTP URL.
Previous route-bound entries remain readable. Only the existing authorized
image read can supply original bytes; no HTTP access, account partition,
validator, grant, source-path validation or browser cache semantics change.
Different original content must miss even when a URL is unchanged.

The existing Recording cache warmup worker schedules up to 64 newest
recordings per backend on startup and newly discovered recording identities
after its existing refresh events, plus committed native metadata identities
from each successful refresh and the existing metadata cycle. Scheduling
uses the already committed inventory in that path, not another catalog read;
optional scheduling failures do not invalidate the inventory. It owns a
memory-only deduplicated
128-candidate queue and attempts one item after ordinary refresh work per
existing iteration, outside the shared refresh lease. Use the canonical
validated metadata poster reader, with the existing legacy artwork reader
for compatibility when no poster response exists; no external acquisition,
new HTTP route, polling loop, worker thread, persistent queue or schema.
Background FFmpeg runs at a best-effort lower CPU priority; the same
nonblocking conversion lock, byte/pixel/time limits and 32 MiB / 256-entry
disk capacity remain. Cached/small/missing images do not require conversion.
This is bounded preparation, not a promise to pre-render the entire library.
Content is re-read/validated at demand, so the original read cost remains.
Server-Timing separates authorized read from preview work for Home responses;
the read interval includes routing and authorization, not solely file I/O.
The cache/HTTP regressions exercise prewarmed bytes, aliases, changed inputs,
legacy entry compatibility, separate foreground/background cache instances
and existing authorization. Native C++ tests, build and actual yaVDR cold
browser timings are open. No new frontend installation is required.

### PWA.14.5 — all Home folder opens use combined pages and scoped previews

Operator confirmed PWA144_INSTALLATION=PASS on `a13c3a2cdf961dd935c5dc389e6eae18e50c0db7`.
A repeat video/operator report identified several seconds before manual folder
cards appear, plus staggered series subfolder covers. The manual branch still
awaited all legacy folder pages; now automatic, manual, nested and Back use
the existing 12-card initial / 24-card lookahead path. Older non-combined
responses fall back to the established legacy reader. The existing persistent
Home owner can store a sanitized first-page presentation under a bounded
folder-specific key (same database, scope, 32-row cap, SSE generation and
account/grant fences). It is deliberately a noninteractive preview, not a
cached canonical Playback/Recording object. Current server proof is required;
authoritative demand fetch replaces it and enables actions. No offline private
access, new endpoint, polling, schema migration or generic response storage.
A recording-domain invalidation discards dependent folder previews. Real
browser latency and native test execution remain to be verified.

### PWA.14.6 — Home subfolder cover projection

The inline folder owner already had a visible-only bounded fallback, but each
real subfolder could still require up to two additional folder HTTP reads.
Reuse the existing per-backend browse snapshot: compute a representative's
owning folder bottom-up once per inventory replacement, preserving direct
recordings first and alphabetical child order. Only non-leaf cards in the
requested non-root combined Home page get a representative; legacy and
Public-v1 folder responses, leaves, counts and paging are unchanged.
The existing controller resolves the current manual/native/cached cover
precedence and returns only its canonical image URL, not episode payloads or
private filesystem paths. No added metadata HTTP calls for these subfolders;
the existing validated/authenticated artwork routes still deliver bytes.
The browser renders the supplied poster at card creation and only runs the
old bounded traversal for older responses without the field. Missing artwork
uses the existing fallback. The first page's scoped persistent projection
also includes these URLs. No new schema, endpoint, polling or permission.
Actual server/current-browser latency and native C++ validation are pending.
IPTVnator/Plex remain perceived-speed targets, not measured equivalents.

The focused preview integration test now uses the actual category-click path,
holds the demand response, verifies the visual-only preview, then verifies
its replacement by the canonical recording card and teardown on collapse.

### PWA.14.7 — reuse the canonical Series presentation in recording folders

Operator confirmed PWA1456_INSTALLATION=PASS and supplied three actual UI
photos: upper Home Series covers, selected Battlestar Galactica with six
season/special groups, and selected season with episode cards. The Series
recording folder used different representative images and slower generic
navigation. The same canonical Series model, cover selection, hierarchy,
season buttons and episode card renderer now support a separate mounted
folder surface with its own selection/scroll state. Scope by real recording
path and backend; compare unique member count to the authoritative folder
entry before replacing generic folder browsing. Unmatched or incomplete
catalogs retain the existing combined-page path; individually matched
non-leaf series subfolders can use the shared detail even with incomplete
root coverage. No new API, storage, grant or series model. Cover/hierarchy
writes stay in their existing owner. Both episode rails use the existing
visible-first/adjacent-artwork priority pass on render and scroll. No image
substitution or speculative offscreen requests detached from rendered cards.
Actual first cold folder HTTP latency and cover latency have not been
measured separately; those remain real-browser acceptance questions.
Native frontend regression and real-browser acceptance pending.

CI #12918 confirmed the native Series return/scroll, native grouping, SSE,
progressive projection and hierarchy tests; the older simple-UI source-string
contract still expected the single-surface call signature. Adjust that contract
and the matching production-composition signature, retaining mutation/UI checks.

### PWA.14.7 correction — do not discard the shared Series rail on count mismatch

Operator confirmed PWA147_INSTALLATION=PASS, but the supplied screenshot
showed the lower physical Series folder still had generic folder tiles
(different covers, recording counts, missing Battlestar cover). The previous
all-or-nothing exact-count guard and early auto-open could select the generic
path permanently. The Series root now uses the scoped canonical groups even
when genre membership differs from physical inventory; the original full
folder navigation is explicitly reachable with “Alle Aufnahmen im Ordner”.
A root opened before the existing Series query finishes stays mounted and
receives its projection; if the settled query yields no matching members
the existing generic browser remains available. Direct subfolder replacement
still requires complete membership. Reuse the actual canonical series title
and cover (including overrides) even when the episode list is filtered by
physical location. No cross-backend membership or new query/API/cache owner.
Regression covers a 464-recording folder with partial canonical membership,
early opening, identical cover/title/card class, scoped seasons and original
folder access. The exact live count/path mismatch is not measured from the
screenshot alone. Browser verification still required.

## PWA.15.1 — Cold-start artwork diagnosis (2026-11-20)

Baseline: merged `main` `574ebebaef8fffff88b841d737aac8d68bfea226`.
Post-merge [CI #12921](https://github.com/hotzenplotz5/vdr-suite/actions/runs/44679155261)
passed all six jobs. Existing opt-in browser diagnostics already measure
buffered Resource Timing, startup marks and per-client Service Worker
counters for local hits, misses, revalidations, network artwork and evictions.

This slice extends **only** the existing browser diagnostics probe, adding
identity-free grouped artwork Resource Timing phases: total duration,
request to first byte, response body and available `Server-Timing`
`artwork-read` / `artwork-preview` durations. Visible same-origin artwork
load events record first and last observed load times from measurement
start; the stop report adds visible, loaded and pending image counts. The
load listener is removed on stop. No production request, forced image
decoding, cache, polling, rendering or server behavior changes.

**Limits:** A `load` event is not decoded-image paint, and the viewport
check may include horizontally clipped rail images. Resource Timing alone
does not prove a Cache Storage hit; inspect the existing worker counters.
Server phases may be absent on intermediary/cache responses, and
`artwork-read` includes authorization and routing. No individual URL,
recording, backend, user or credential is exported. A diagnostics iframe
reload does not simulate a power cycle.

**Real-browser acceptance still open:** After an actual PC restart, use
`/frontend/browser-performance-diagnostics.html` while authenticated.
Stop/export the initial Home, then compare a warm reopen, Series/season
changes, recording folders and horizontal scroll. Compare startup marks,
first/last visible artwork loads, pending covers, worker cache outcomes,
image HTTP counts, response and server timing. Only then decide whether
PWA.15.2 cache sizing, PWA.15.3 prewarming or PWA.15.4 persistent
presentation needs modification. Keep the existing 32 MiB / 256-preview /
1024-artwork limits until a measured defect is established.


## PWA.15.4.1 — concurrent exact-artwork reuse (2026-11-20)

A real operator capture with an existing browser cache observed 183 worker
artwork requests, 52 immediate scoped local hits, 16 revalidated hits, 128
current-scope misses, 150 artwork HTTP requests and 141 successful writes;
Home frame was ready at 324.4 ms but observed image loads continued through
55.2 seconds. The 22 repeated resource entries in the same capture are
**not** proof that all repetitions were artwork or concurrent, nor do 128
misses prove that those exact images existed in the persistent cache.

The existing visible-first Home artwork coordinator and canonical persistent
Cache Storage ownership remain in place. For an actual concurrent duplicate
image URL in the **same controlled client, current recording/EPG authorization
proof sequence, partition, generation and proof epoch**, the existing worker
now shares one pending online validation response while producing independent
Response bodies. It never shares responses between clients, permissions or
different URLs; absent/expired proofs and non-success responses follow the
original online path. Session, series-artwork mutation and grant drift retain
all existing generation/epoch fences. No cache-key rewrite, grant expansion,
new polling, prewarm burst or server processing change is introduced.

The additive identity-free `coalescedRequests` worker counter distinguishes
actual coalescing from ordinary cache hits. Regression covers simultaneous
requests and separately readable bodies, alongside existing cross-client,
logout, 403, proof-expiry, grant-isolation and installation-path contracts.
The exact browser improvement and whether remaining misses are previously
unseen covers still require a repeated controlled warm/cold capture.


## PWA.15.4.2 — no unrelated validator scan on a scoped miss (2026-11-20)

The current-scope exact-match lookup in the existing worker runs before
network validation. If this lookup finds **no validated artwork in the
current authorization partition**, old cached candidates from other
partitions cannot satisfy the existing 304 validator matching rules.
Previously every such known miss still invoked `cache.keys(base,
{ignoreSearch:true})` and inspected up to four other entries before
making an image HTTP request. The operator captured 128 such current-scope
misses in a 61.7-second Home session with a nonempty browser cache.

For that exact known-miss path, skip the obsolete candidate enumeration and
make the same original unconditional authorized image request. Valid
current-partition local hits and stale/revalidated entries retain all
previous behavior. The no-proof, expired-proof, offline, permission,
logout, mutation and other legacy network paths remain unchanged; no
new cache namespace or changed cached-image authority is introduced.
The deterministic worker regression asserts zero old-partition scans on
known misses and no cross-partition ETag forwarding.

This removes redundant local Cache Storage work; it cannot turn previously
unseen covers into immediate hits. Browser wall-clock improvement remains
unverified until the operator repeats the actual capture.


## PWA.15.4.3 — stable Recent Movies rail DOM during Home refresh (2026-11-20)

The operator verified the installed browser diagnostics, bridge, probe and
generated Home HTML match repository sources at
`66304478761a55e97bf2139df3c6545363434cea`: old diagnostic assets
are not responsible for repeated poster loads. `Home neu laden & messen`
intentionally creates a fresh embedded Home document, which necessarily
reconstructs its DOM; this is separate from in-document refresh behavior.

The `Filme der letzten 5 Jahre` rail previously removed all artwork via
`renderState()` before each same-backend fetch and `target.replaceChildren()`
on each successful render. The same-backend refresh now retains the already
painted rail until authoritative data arrives and reconciles cards by
Recording ID and backend. Unchanged image URLs preserve their original IMG
nodes; changed poster URLs replace only the affected artwork. Existing
card clicks follow the newest authoritative Recording object, while
pagination, scroll position, session/backend change clearing, first-page
query and bounded prefetch remain unchanged. No extra request, cache purge,
permission change, polling or server build is introduced.

The production-backed Recent Movies regression covers identical image
identity, a changed title, updated Recording click behavior and a changed
poster URL while neighboring card nodes remain stable. A complete iframe
reload still reloads the UI; browser-visible speedup remains unmeasured.
