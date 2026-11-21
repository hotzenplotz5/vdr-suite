# Public-v1 Recording R2/R3 — real yaVDR hardware closeout (2026-10-21)

## Verdict and provenance

**`API_V1_RECORDING_HW_ACCEPTANCE=PASS`** for the *single explicitly approved* Brisant fixture on the user's yaVDR host.

- Accepted source and deployed daemon: `13e63d277deb5c9e0230f757a8c5733f6e7b0cbb` ([commit](https://github.com/hotzenplotz5/vdr-suite/commit/13e63d277deb5c9e0230f757a8c5733f6e7b0cbb)); installed/running SHA-256 `aca41b516e59375cca0cfb8d3e0c63043d1cb71d90df0282cf4b73d3b48a82f5`, daemon active.
- All exact-source workflows passed: [R2 Recording + Genre #165](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39656027588), [Recording R2 Candidate #124](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39656027625), [VDR-Suite CI #10597](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39656027816), [VIDAA focused #146](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39656027394).
- Human Account browser session / CSRF accepted: `HUMAN_AUTH=PASS`.

## Flat Public-v1 Recording collection restoration

ADR-0035 omits Recordings from the lightweight VDR startup snapshot. The flat Public-v1 list incorrectly projected that sparse snapshot, yielding HTTP 200 and an empty collection while real Recordings existed. Commit `13e63d2` reads the independent, backend-scoped `VdrRecordingCacheRepository` with readiness check, preserving the existing projection, folder browse and lazy startup contract. No schema, second index or new VDR fetch was introduced. With the corrected daemon, the authenticated flat Public-v1 endpoint returned **1,037** entries before fixture mutation and matched Brisant uniquely.

## Fixture and accepted actions

| Field | Evidence |
| --- | --- |
| Backend | `default` |
| Original source | `/srv/vdr/video/Brisant/2026-10-21.17.15.1-0.rec` |
| Public ID | `rec_f30ec344800ac3135114c7770e823343` |
| Provider event start / size | `1792595700` / 57 MB |
| RENAME target | `Brisant-R3-DRYRUN` |
| MOVE target | Existing logical folder `Action`, `fld1_8cd8e2b721c8623855e671deed131727` |
| Final pre-DELETE path | `/srv/vdr/video/Action/Brisant-R3-DRYRUN/2026-10-21.17.15.1-0.rec` |

1. Non-mutating preflight: `RENAME_VALIDATE=PASS`, `RENAME_DRY_RUN=PASS`, `MOVE_VALIDATE=PASS`, `MOVE_DRY_RUN=PASS`, `DELETE_VALIDATE=PASS`, `DELETE_DRY_RUN=PASS`.
2. Authorized real RENAME: `RENAME_EXECUTE=PASS` and `RENAME_READBACK=PASS`; native path changed uniquely and public ID was preserved.
3. Authorized real MOVE: `MOVE_EXECUTE=PASS` and `MOVE_READBACK=PASS`; independent subsequent `/recordings.json` confirmed `ORIGINAL_COUNT=0`, `RENAMED_COUNT=0`, `MOVED_COUNT=1`; service active.
4. An initial DELETE-stage HTTP `TimeoutError` happened **before** the real DELETE invocation, leaving `API_V1_RECORDING_DELETE=NOT_RUN` for that incomplete attempt. A *read-only* native recheck confirmed the moved source still present. There was no automatic retry.
5. Separately authorized DELETE-only sequence: new `DELETE_VALIDATE=PASS`, `DELETE_DRY_RUN=PASS`, then **one real** `DELETE_EXECUTE=PASS`, `DELETE_SERVER_READBACK=PASS`; independent provider absence and public `rec_` retirement verified. Final `API_V1_RECORDING_DELETE=PASS`, `API_V1_RECORDING_HW_ACCEPTANCE=PASS`, `AUTOMATIC_RETRY=DISABLED`.

**Final state:** the test recording is removed from the active VDR collection and its public identity retired. The 1,037 count above was *pre-delete*; no post-delete total is asserted. Never rerun this destructive fixture sequence.

## Boundaries and next step

This certifies the bounded browser-authenticated Public-v1 R2/R3 real-host RENAME/MOVE/DELETE and opaque identity lifecycle. It does **not** imply that an unknown outcome *after* a real mutation POST is safe to retry, that durable operation/replay reconciliation is implemented, or that any other production Recording is approved for deletion.

Next in the accepted Public-v1 sequence: **[ADR-0068](../adr/ADR-0068-public-api-router-modularization-checkpoint.md)**, a pure responsibility split of existing `ApiRouter` / `PublicApiRuntime` behavior. Inventory exact methods/paths, JSON/status/headers, grants, CSRF/ETag, public and legacy clients; preserve every behavior and single authority. The subsequent order is E1 → E2 → M1 → M2 → S1 → P1. See [CURRENT](../CURRENT.md) and [Strict Roadmap](../planning/roadmap.md).
