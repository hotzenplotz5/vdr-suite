# Web Client API Contract Snapshot

## Navigation

- [Development Index](index.md)
- [Client API and Frontend Module Boundary Plan](client-api-frontend-module-boundary-plan.md)
- [Current Project Status](current-status.md)
- [Parity Audit and Frontend Gap Roadmap](../planning/parity-audit-and-frontend-gap-roadmap.md)

---

## Status

Phase 59.09f documents and guards the current Web Client API seam before UI module extraction.

This document is intentionally a snapshot, not a design wishlist. Every exported helper listed here must exist in `web/frontend/api/client-api.js` and must be exported through `window.VdrSuiteClientApi`.

No direct `fetch()` calls in `web/frontend/app.js`.

---

## Contract Owner

Runtime API ownership:

- `web/frontend/api/client-api.js` owns HTTP access from the web frontend.
- `web/frontend/app.js` owns UI orchestration and must use `window.VdrSuiteClientApi`.
- feature modules must not introduce direct backend fetches.

---

## Exported Client API Functions

Timer:

- `fetchClientTimers`
- `fetchClientTimerConflicts`
- `fetchClientTimerCreateAction`
- `fetchClientTimerUpdateAction`
- `fetchClientTimerDeleteAction`

Channels:

- `fetchClientChannels`
- `fetchClientChannelMoveAction`

Capabilities and runtime state:

- `fetchClientCapabilities`
- `fetchClientVdrOverview`
- `fetchClientVdrStatus`
- `fetchClientVdrHealth`
- `fetchClientVdrSnapshotSummary`
- `fetchClientVdrSnapshots`

Backend selection:

- `fetchClientBackends`
- `fetchClientDefaultBackend`
- `fetchClientBackendSnapshot`

EPG:

- `fetchClientEpgWindow`
- `fetchClientEpgSearch`
- `fetchClientEpgCacheStatus`
- `fetchClientEpgCacheWindow`
- `fetchClientEpgNowNext`
- `fetchClientEpgTimeWindow`
- `fetchClientEpgChannelWindow`

Metadata and persons:

- `fetchClientMetadata`
- `fetchClientPersons`
- `fetchClientRecordingPersons`

Recordings:

- `fetchClientRecordings`
- `fetchClientRecordingActionValidation`
- `fetchClientRecordingActionExecution`

SearchTimer:

- `fetchClientSearchTimers`
- `fetchClientSearchTimerDiscovery`
- `fetchClientSearchTimerPreview`
- `fetchClientSearchTimerPlan`
- `fetchClientSearchTimerValidate`
- `fetchClientSearchTimerExecute`
- `fetchClientSearchTimerRealTest`
- `fetchClientSearchTimerCreateAction`
- `fetchClientSearchTimerUpdateAction`
- `fetchClientSearchTimerDeleteAction`

---

## Current Direct Fetch Inventory

Remaining known direct API fetch inventory in `web/frontend/app.js`:

- none

The ownership guard must fail if `web/frontend/app.js` calls `fetch()` directly.

---

## Missing Backend Route Gaps

The following helpers must not be added until matching backend routes exist and are verified:

- `fetchClientPermissionReport`
- `fetchClientEventDetail`
- `fetchClientEventArtwork`
- `fetchClientEventMedia`
- `fetchClientRecordingMarks`
- `fetchClientRecordingResume`
- `fetchClientRecordingCut`
- `fetchClientRecordingPlayback`

Known missing route areas:

- permission report
- event detail
- event artwork
- event media
- recording marks
- recording resume
- recording cut
- recording playback

---

## Next Use

This snapshot is the handoff point for the next frontend work:

- extract UI modules without moving HTTP ownership back into `app.js`
- keep `client-api.js` DOM-free
- keep new backend route wrappers explicit and guarded
- update this snapshot whenever a real new `fetchClient*` wrapper is added

---

## Back

- [Development Index](index.md)
- [Documentation Index](../index.md)
