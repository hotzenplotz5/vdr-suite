# Completed Phases Latest Marker

## Latest completed numbered runtime phase

```text
Phase 68 - Legacy OSD Compatibility Bridge
```

Phase 68 is completed for the accepted 68.A-G scope. The final real yaVDR
runtime candidate is `7ae51d090cbe06570b8a70e787137232df83f124` and produced
`RESULT=PHASE68G_REAL_NATIVE_OSD_INPUT_PASS`.

See [Phase 68 Closeout](phase-68-closeout.md).

## Latest completed non-numbered Home work

After the Phase-66 closeout, bounded Home and Recording hardening continued
without reopening the numbered phase. The consolidated Home-rebuild branch
ended at:

```text
accepted_home_rebuild_head=0cce4d1c9e58abe4d529132e92340ae4cbb7a99c
merged_to_main=ea5967b983aee9ccc3f855b685db01abbfb2326a
```

See [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md).

## Previous completed numbered runtime phases

- **Phase 67 - Broadcast Companion Services: Teletext and HbbTV** — normalized Teletext plus fenced HbbTV discovery/application-session/presentation-media runtime; Golden Journeys 8 and 9 accepted.
- **Phase 66 - Media Home and Browse Experience** — responsive Media Home/Browse and Golden desktop/mobile acceptance.
- **Phase 65 - Streaming Gateway and Media Sessions** — authenticated Recording/Live playback, least-transformation delivery/output policy and normalized playback semantics.
- **Phase 64 - Timer Intent and Multi-Backend Orchestration** — durable intent/assignment/binding model, managed native fulfillment, readback/reconciliation and controlled failover.
- **Phase 63 - Backend Agent and Secure Multi-Site Runtime** — secure Agent lifecycle, generation fencing and provider ownership.
- **Phase 62 - Identity, RBAC and Accountability Foundation** — persistent identity, backend-scoped authorization, browser-session security and accountability.

## Current active numbered runtime phase

```text
None - Phase 68 is completed
Next: Phase 69 - Public API and Client Compatibility Hardening
```

Phase 69 has not started. Completing Phase 68 does not silently authorize
Phase-69 implementation.

## Evidence boundary

Post-phase hardening does not renumber completed history. Historical closeouts
retain exact candidate evidence for the state they closed; current status belongs
in [Current State](../CURRENT.md).

## Maintenance rules

- Keep this marker aligned with CURRENT, Roadmap, Phase Map and Current Status.
- Keep numbered phases and non-numbered hardening distinguishable.
- Do not promote accepted architecture to implemented runtime without evidence.
- Update the matching closeout whenever a numbered phase or bounded post-phase workstream closes.
