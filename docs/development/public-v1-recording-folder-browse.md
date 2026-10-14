# Public-v1 R2: Home-compatible recording-folder browse (VIDAA)

Status: implementation candidate in a separate branch; the running yaVDR daemon has **not** been replaced or tested with this change.

## Reuse rather than duplication

Web Home's existing "Aufnahmeordner" hierarchy remains the reference. VdrRecordingCacheRepository indexes those folders and recordings. The Public-v1 API projects this canonical hierarchy into bounded device-safe items without leaking private native paths or provider URLs. VIDAA owns its own remote-focused TV presentation and does **not** call unversioned legacy Web APIs.

The feature branch work/r2-recording-folder-browse is derived from the installed server-R2 commit **1822ff0181add4620a3bd35a36d6b574c252003b**; server main is divergent and must not be substituted.

## Endpoint and capability

- GET /api/v1/capabilities now advertises public-api.recordings-browse version 1, available with Recording collection lookup; unchanged public-api.recordings-read version 1 remains.
- Legacy GET /api/v1/recordings?backendId=... retains its existing recordingId/cursor contract.
- New additive GET /api/v1/recordings?backendId=home&view=folders&limit=30&offset=0; optional folderId=fld1_ followed by 32 lowercase hex characters.
- Items have kind=folder, folderId, name and recordingCount; or kind=recording, canonical recordingId, backendId, title, recordedAt, durationSeconds, durationKnown. Page has limit, offset, totalCount and hasMore. No native path, backendNativeId, private media transport or secret is exposed.
- Folder IDs are stable backend-scoped references, not credentials. Only existing authenticated SecurityHttpGate recordings.view for the specified backend authorizes access. Unknown IDs / illegal parameters fail closed; there is no automatic legacy fallback.
- Page combines sorted immediate subfolders and direct recordings from the existing Web Home browse snapshot. A VDR title folder marked `singleRecordingLeaf` is emitted as `kind=recording` with its authorized canonical `recordingId`, preserving the original position within the page; it must not appear as a navigable folder. Only the requested bounded page is serialized. VIDAA uses 30 objects/page; no 1033-item frontend DOM.

## Required regression and production acceptance

Focus tests with the actual selected branch head:

- make test-public-recording-collection-api
- make test-public-recording-collection-security
- make test-public-recording-collection-projection
- make test-vdr-recording-cache-repository

Regression includes folder IDs derived from Home hierarchy, backend isolation, strict legacy cursor compatibility, invalid folder inputs, authenticated gate and response without provider paths. The feature has not yet passed compilation on yaVDR.

Only after successful tests and a resource preflight should the server be deployed by the **canonical** stage-install-runtime / deploy-install-runtime / check-install-runtime-deployment pipeline with RUNTIME_DEPLOYMENT_MATCH=YES. The existing server build and TV installation should remain intact until that passes. The separately version-pinned VIDAA checkout must then be deployed to /var/www/html/app with --chmod=D755,F644, complete backup, byte-for-byte HTTPS content checks and rollback.

Real Hisense testing of directory traversal, D-pad/Back, page navigation with >1000 recordings, display stability, denied permissions, and preservation of pairing is still required. Playback belongs to a separate MediaSession integration.
## Public-v1 bounded title search (VIDAA integration candidate)

Browser Global Search at /api/search remains unchanged. The TV client must use device-authenticated /api/v1, not Web session credentials. A new additive recording collection view uses GET /api/v1/recordings?backendId=<authorized>&view=search&query=<percent-encoded-text>&limit=30&offset=0. The *existing* device recordings.view and exact backend scope authorize it; no independent search permission or private-provider endpoint is introduced.

Only recording **title** matching is supported in this scoped first slice. The server uses the Web Global Search text-folding utility on the projected, backend-authorized Recording snapshot (without creating a second search index). This is not yet full Web search parity for people, description, EPG, metadata or provider attributes. Input is strictly URL-decoded UTF-8, 2–80 Unicode points, at most 120 decoded bytes, and control characters / malformed encoding are rejected. Pagination returns at most 30 items, canonical rec_ identities, totalCount and hasMore; no native VDR paths, credentials, or raw media URLs. The TV must never enumerate all recordings to filter locally.

Security/API regression: test-public-recording-collection-api and test-public-recording-collection-security, plus full architecture/integration CI. Real Hisense acceptance of remote input, keyboard focus, search results (including nested folders), paging, no-match/denial, return-to-folder and playback transition remains **PENDING**. Installed yaVDR daemon remains at the separately verified HLS fast-start code PIN until a separately authorized deployment.

## 2026-10-14 correction: Web Global Search parity and device-safe results

The previous TV title-only matching failed parity with the Webfrontend. Web uses GlobalSearchRepository indexed SQL: normalized substring matching across recording titles, original titles, metadata, native short texts, associated people, plus EPG events, not a user-regex. Case and German umlauts are normalized by the shared foldText implementation. Search order prefers exact titles, then prefixes, other title hits, then person hits.

The current Public recording view now calls the same GlobalSearchApiRuntime service that powers Web, then intersects its results with the current VDR snapshot and PublicRecordingIdentityRepository. It serializes only authorized opaque rec_ IDs and PublicRecordingCollectionProjection fields; raw native paths, metadata URLs and person portraits cannot cross into Device API. Unmatched stale index/snapshot references are rejected. The browser-session /api/search route remains unavailable to TV credentials.

An independent Public-v1 EPG route GET /api/v1/epg/search?backendId=home&view=search&query=Hobbit&limit=30&offset=0 returns the same indexed Web EPG results in the Web default six-hour-history / fourteen-day-future window. Its security gate uses epg.view on exactly one authorized backend, *not* recordings.view or channels.view. Events are additionally scoped against that backend's current channel snapshot and exposed with safe channel/title/timing/match-reason facts only. Public-v1 input rejects malformed/unsafe queries and paginates at most 30 results. Results are separate so a recordings-only TV grant cannot view EPG and an EPG-only grant cannot view recordings.

CI and Hisense actual search acceptance remain separate; no production deployment is implied by these draft commits. The currently installed daemon still has product-code SHA 5167ae5f020049892be1af77bee77554352c0f2a and the TV app b20261014.4 until deliberately updated.
