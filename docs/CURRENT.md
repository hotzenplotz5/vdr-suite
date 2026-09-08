# VDR-Suite Current State

This is the sole repository authority for volatile operational status. Verify live GitHub state before acting.

## Current position

Phase 66 — Media Home and Browse Experience: completed. Post-Phase-66 native recording editing: completed and merged through PR #268. Phase 67 — Broadcast Companion Services: Teletext and HbbTV: next, not started, separate explicit runtime kickoff required.

The native editing product head `a5dfef23478ddb96005471d4f2fca8b2bd18e726` passed all six jobs of CI #8852, run `34162336126`, and merged as `8bdf508908454a43c3e00466d93037bb95f7dc17` on 7 September 2026. The scope includes native marks read/modify, cut preview/execution, result discovery and browser editing. Real yaVDR evidence includes marks mutation, native cutting and non-destructive browser verification. The observed test-recording `index`/`info` change remains unresolved. No universal byte-preservation claim or additional destructive acceptance is implied.

The final Home Series order-reconciliation fix is included. CI passed; no separate new real-mobile acceptance of the final commit is claimed. The unrelated Series metadata/artwork symptom remains deferred with unknown root cause.

Documentation closeout branch: `docs/post-phase66-recording-editing-closeout`. Read live GitHub for its PR, head and CI status.

## Phase order

64 Timer orchestration [COMPLETED] → 65 Streaming Gateway and Media Sessions [COMPLETED] → 66 Media Home and Browse Experience [COMPLETED] → 67 Broadcast Companion Services [NEXT; NOT STARTED] → 68 Legacy OSD → 69 Public API → 70 Recommendations.

Preserve Phase-62 identity, Phase-63 Agent execution, Phase-64 fencing, Phase-65 MediaSession and Phase-66 Home ownership. VDR remains canonical for marks and cutting. Unknown native outcomes are reconciliation-only. No recording mutation, cut, installation or restart is authorized by documentation work.

## References

[New Chat Handoff](NEW-CHAT-HANDOFF.md) · [Strict Roadmap](planning/roadmap.md) · [Native Editing Closeout](development/post-phase66-recording-editing-closeout.md) · [Native Editing Status](development/post-phase66-native-recording-editing-status.md) · [Phase 66 Closeout](development/phase-66-closeout.md) · [ADR-0054](adr/ADR-0054-broadcast-companion-teletext-hbbtv.md) · [ADR-0059](adr/ADR-0059-vdr-native-recording-editing-marks-cutting-authority.md)
