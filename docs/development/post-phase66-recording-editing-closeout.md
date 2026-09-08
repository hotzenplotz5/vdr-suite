# Post-Phase-66 Native Recording Editing — Closeout

## Status

The bounded native Recording-editing workstream is merged through PR #268. Phase 66 remains completed; Phase 67 has not started. This closeout preserves the distinction between delivered product capability, recorded real-system evidence, and unresolved observations. It does not authorize further Recording mutations.

## Merge checkpoint

- Repository: `hotzenplotz5/vdr-suite`
- PR: [#268](https://github.com/hotzenplotz5/vdr-suite/pull/268)
- Accepted product head: `a5dfef23478ddb96005471d4f2fca8b2bd18e726`
- Merge commit: `8bdf508908454a43c3e00466d93037bb95f7dc17`
- Merge date: 2026-09-07
- Final product CI: VDR-Suite CI #8852, complete six-job PASS.

The implementation and historical acceptance details remain in [Current Recording Editing Status](post-phase66-native-recording-editing-status.md) and [Workstream Slices](post-phase66-native-recording-editing-slices.md). This document does not replace their detailed evidence.

## Delivered capability

VDR remains the canonical owner of native marks and native cutting. The authenticated Recording editing surface uses the existing Control Plane, Agent and SuiteBridge authority chain. It does not introduce a proprietary marks store, browser filesystem writes, a separate cutter or a second playback owner.

The delivered scope includes native marks readback and revision-fenced add/delete/move/reset/replace operations; native cut preview and explicit confirmation; durable operation identity, provider/generation fencing and reconciliation without blind redispatch; native edited-result discovery; and integration into the existing Recordings 2 detail and playback composition. The original Recording is not automatically deleted or replaced.

The final bounded Home correction preserves existing Series cards and horizontal position across unchanged refreshes and reconciles ordering only when necessary. It is not a general Home performance optimization.

## Evidence boundaries

The real yaVDR Slice-2C marks acceptance is recorded as PASS on `b51b906becbaec8dfa72a5649e76327336eac2ea`, with final frames `7500,15000,18750,22500`, two sequences and zero in-use flags. The existing real-cut and browser acceptance records must be read as historical evidence, not permission to repeat an operation.

The later authenticated Edge acceptance on installed frontend head `402f6b92f259609ccfe4322be59e56667e3f54af` verified the existing Brisant source and edited result, native marks display, read-only cut preview, canonical playback-position control lifecycle and daemon-reconnection behavior. It performed no new marks POST or cut POST. The final Series correction is covered by its focused regression and the complete final product CI; no fresh real-device acceptance of that last correction is claimed here.

The recorded Brisant source `index` and `info` changes before the later installation remain unexplained. Source and edited-result video inventories and the source marks hash were unchanged across the recorded later browser checks, but this does not prove that every file was unchanged. The earlier cut-result discovery latency was not remeasured by repeating a cut.

## Subsequent recovery checkpoint — 2026-09-08

The historical PR #268 checkpoint above remains unchanged. A separate bounded follow-up, [PR #271](https://github.com/hotzenplotz5/vdr-suite/pull/271), subsequently merged HTTP deadline recovery, the optional browser-diagnostics build correction, bounded native RMARKS read-lock acquisition and the isolated lock regression harness.

- Accepted recovery head: `cf835a8165e2e9bb7e8d947b1bde2b8fad242409`
- Merge commit: `dc37a421aa538da1b6fee90f2ce05c04e224b8b5`
- Final hosted CI: [VDR-Suite CI #8879](https://github.com/hotzenplotz5/vdr-suite/actions/runs/34263624376), run `34263624376`, complete six-job PASS, including daemon build.
- Real yaVDR isolated RMARKS regression: PASS, `RMARKS_LOCK_TEST_RC=0` on `48fd4e870246c971bffe30de4a84c4a4f775c1cd`.
- Real yaVDR shutdown/HTTP guard and HTTP deadline regression: both RC=0 on the accepted recovery head `cf835a8165e2e9bb7e8d947b1bde2b8fad242409`.

The isolated RMARKS harness uses fake VDR headers and does not perform a Recording mutation. These results do not establish a new installed-plugin runtime acceptance, a new cut, or resolution of the earlier Brisant `index`/`info` observation. No installed yaVDR update, service restart or further Recording mutation is implied by the GitHub merge.

## Remaining boundaries

MARKAD remains optional and is not a second marks authority. No irreversible automatic MARKAD cut, automatic original deletion, active/growing Recording editing, generalized nonlinear editor, broad Home performance work or Phase-67 runtime work is included. Any unresolved acceptance item must retain its precise evidence status in the detailed record rather than being converted into an invented PASS.
