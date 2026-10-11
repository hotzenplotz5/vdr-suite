# VDR-Suite Project State Snapshot

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Planning Index](index.md)
- [Roadmap](roadmap.md)
- [Lazy Recording Loading](lazy-recording-loading.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Current Project Status](../development/current-status.md)

---

## Purpose

This file retains the high-level project state snapshot template for `tools/update_project_progress.py`. The previous Phase-58 block was outdated; this snapshot now records the durable accepted architecture baseline. **Exact active branch, PR, CI, deployment and slice status belongs only in [Current State](../CURRENT.md).** Do not use the legacy snapshot copier to replace independently maintained current-state sections without verifying the target markers.

It is intentionally **not** a percentage-based product progress tracker.

VDR-Suite has many completed foundations, but it is not production-complete. Some domains are deliberately read-only, dry-run-only or guarded until later safety phases.

The generated state blocks are written by:

    tools/update_project_progress.py

---

<!-- PROJECT_STATE_SNAPSHOT_START -->
## Project State Snapshot

This is a repository-scoped implementation summary, not a production-completion percentage or evidence of an installed host version.

### Verified numbered foundations

- Phase 62: Human/Device/Actor identity boundaries, browser session security, RBAC and accountability foundations.
- Phase 63: Backend Agent identity, generation fencing and provider-owned command execution.
- Phase 64: TimerIntent, TimerAssignment, NativeTimerBinding and safe native Timer orchestration.
- Phase 65: Streaming Gateway and MediaSessions with bounded client playback semantics.
- Phase 66: responsive Media Home and Recording discovery, with separately merged Home/Recording post-phase hardening.
- Phase 67: Teletext and HbbTV Broadcast Companion services.
- Phase 68: Legacy OSD viewing, controller lease and allowlisted native input.
- Phase 69: accepted Public-v1/API compatibility baseline and reference-client coverage.

### Current cross-cutting product work

- Multiuser Account/backend access administration remains a separate product workstream; MU.9A-F and post-MU.9 usability are merged, with later MU.10A/MU.10B Device pairing acceptance recorded in [Current State](../CURRENT.md).
- Subsequent R2 Recording browse and authenticated Device Recording playback source is merged into `main`, including verified index-derived duration/preparing state and audio/SRT/WebVTT track/subtitle routes. Neither this merge nor this document proves the running daemon or TV hardware acceptance.
- The accepted original Phase-69.F contract baseline is unchanged by additive post-phase candidate routes.

### Deferred / separately gated

- Phase 70 Recommendations/Content Knowledge Graph is not started and requires its own accepted runtime ADR.
- Broad Timer product UI, growing-Recording seek/timeshift and further client/federation surfaces remain separate implementation/acceptance decisions.
- Real yaVDR deployment, Device media selection and installed build identity must be verified independently; no source-merge assertion substitutes for runtime acceptance.

Progress source: {{PROJECT_PROGRESS_SOURCE_LINK}}
<!-- PROJECT_STATE_SNAPSHOT_END -->

---

## Maintenance Rule

Update this file when the verified project state changes.

After editing this file, run:

    python3 tools/update_project_progress.py
    make test-docs
    make test-phase

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Planning Index](index.md)
