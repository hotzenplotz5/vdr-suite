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
- Page combines sorted immediate subfolders and direct recordings from the existing Web Home browse snapshot. Only the requested bounded page is serialized. VIDAA uses 30 objects/page; no 1033-item frontend DOM.

## Required regression and production acceptance

Focus tests with the actual selected branch head:

- make test-public-recording-collection-api
- make test-public-recording-collection-security
- make test-public-recording-collection-projection
- make test-vdr-recording-cache-repository

Regression includes folder IDs derived from Home hierarchy, backend isolation, strict legacy cursor compatibility, invalid folder inputs, authenticated gate and response without provider paths. The feature has not yet passed compilation on yaVDR.

Only after successful tests and a resource preflight should the server be deployed by the **canonical** stage-install-runtime / deploy-install-runtime / check-install-runtime-deployment pipeline with RUNTIME_DEPLOYMENT_MATCH=YES. The existing server build and TV installation should remain intact until that passes. The separately version-pinned VIDAA checkout must then be deployed to /var/www/html/app with --chmod=D755,F644, complete backup, byte-for-byte HTTPS content checks and rollback.

Real Hisense testing of directory traversal, D-pad/Back, page navigation with >1000 recordings, display stability, denied permissions, and preservation of pairing is still required. Playback belongs to a separate MediaSession integration.