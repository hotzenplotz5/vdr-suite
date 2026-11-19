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
