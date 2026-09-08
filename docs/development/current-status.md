# VDR-Suite Current Project Status

## Status ownership

Exact operational phase state is maintained only in [Current State](../CURRENT.md). This file provides stable narrative context. Read live GitHub state before making claims about active branches, PRs or CI.

## Platform position

Phase 66 — Media Home and Browse Experience is the latest completed numbered runtime phase. Phase 67 — Broadcast Companion Services: Teletext and HbbTV is next, but has not started and requires a separate explicit runtime kickoff.

The bounded post-Phase-66 native recording editing workstream is completed and merged through PR #268. It delivers VDR-native marks and cutting through the protected execution path, including browser editing and result discovery. The durable [closeout](post-phase66-recording-editing-closeout.md) records the exact merge, CI, real-system evidence and remaining limitations. The final Home Series order-reconciliation fix is included. This work does not reopen Phase 66 or start Phase 67.

## Completed platform foundations

Phase 62 established actor identity, scoped authorization, browser-session lifecycle, CSRF and accountability. Phase 63 established Backend Agent identity, generation/lease fencing, durable command handling and explicit provider ownership. Phase 64 completed reliable Timer orchestration, managed native fulfillment and controlled reassignment/failover. Phase 65 completed Streaming Gateway, MediaSession and client playback ownership. Phase 66 completed responsive Home/Browse and its integrated desktop/mobile Golden journeys.

These accepted boundaries remain authoritative for later work. Runtime Agent credentials cannot act as browser users or unrestricted administrators. Provider availability does not create execution authority. Unknown native dispatch outcomes are reconciled rather than blindly repeated. VDR remains canonical for recording marks and cutting; Suite does not maintain a parallel marks database or expose raw native execution as a public API.

## Native recording editing evidence

The final product head `a5dfef23478ddb96005471d4f2fca8b2bd18e726` passed all six jobs of CI #8852, run `34162336126`, and merged as `8bdf508908454a43c3e00466d93037bb95f7dc17` on 7 September 2026. Real yaVDR evidence includes controlled marks mutation, a native cut and subsequent non-destructive browser verification. The observed test-recording `index`/`info` change remains an unresolved evidence limitation. No universal byte-unchanged or corruption-free claim is made, and no additional destructive acceptance is authorized by this documentation.

The final Series fix reuses keyed cards and changes DOM order only when necessary, preserving horizontal scroll. Automated CI passed; a separate new real-mobile acceptance of the final commit is not claimed. The unrelated Series metadata/artwork projection symptom remains a deferred investigation with unknown root cause.

## Forward ordering

```text
Phase 64 — Timer orchestration [COMPLETED]
  -> Phase 65 — Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 — Media Home and Browse Experience [COMPLETED]
  -> Phase 67 — Broadcast Companion Services: Teletext and HbbTV [NEXT; NOT STARTED]
  -> Phase 68 — Legacy OSD Compatibility Bridge
  -> Phase 69 — Public API and Client Compatibility Hardening
  -> Phase 70 — Recommendation and Content Knowledge Graph
```

ADR-0054 owns the Phase-67 Broadcast Companion architecture. Teletext and HbbTV must not be implemented as unrestricted Legacy OSD or raw browser/plugin command channels. Architecture acceptance is not runtime authorization. Broad Timer UI remains a cross-cutting milestone gated on required access administration.

## References

- [Current State](../CURRENT.md)
- [Native Recording Editing Closeout](post-phase66-recording-editing-closeout.md)
- [Native Recording Editing Status and Evidence](post-phase66-native-recording-editing-status.md)
- [ADR-0059 Native Recording Editing](../adr/ADR-0059-vdr-native-recording-editing-marks-cutting-authority.md)
- [Phase 66 Closeout](phase-66-closeout.md)
- [Phase 65 Closeout](phase-65-closeout.md)
- [Phase 64 Closeout](phase-64-closeout.md)
- [Strict Roadmap](../planning/roadmap.md)
