# Completed Phases Latest Marker

## Latest completed numbered runtime phase

```text
Phase 66 - Media Home and Browse Experience
```

Phase 66 is completed. Its numbered closeout is PR #264 / `de12956ecc283663c820865bb577e7dcf6c5f0ee` and records accepted desktop/mobile Golden Home journeys.

See [Phase 66 Closeout](phase-66-closeout.md).

## Latest completed non-numbered Home work

After the Phase-66 closeout, bounded Home and Recording hardening continued without reopening the numbered phase. The consolidated Home-rebuild branch ended at:

```text
accepted_home_rebuild_head=0cce4d1c9e58abe4d529132e92340ae4cbb7a99c
merged_to_main=ea5967b983aee9ccc3f855b685db01abbfb2326a
```

This work includes Home performance/retention hardening, Recording Discovery and Series metadata completion, canonical Series hierarchy/artwork behavior, Movies/Genres refinements, EPG/cache recovery and the final canonical folder-poster correction. Real-browser acceptance proved the reported `The Exorcist` folder card again used canonical TVScraper portrait artwork rather than a weak Recording still.

See [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md).

## Previous completed numbered runtime phases

- **Phase 65 - Streaming Gateway and Media Sessions** — authenticated Recording/Live playback, least-transformation delivery/output policy and normalized playback semantics.
- **Phase 64 - Timer Intent and Multi-Backend Orchestration** — durable intent/assignment/binding model, managed native fulfillment, readback/reconciliation and controlled failover.
- **Phase 63 - Backend Agent and Secure Multi-Site Runtime** — secure Agent lifecycle, generation fencing and provider ownership.
- **Phase 62 - Identity, RBAC and Accountability Foundation** — persistent identity, backend-scoped authorization, browser-session security and accountability.

## Current active numbered runtime phase

```text
Phase 68 - Legacy OSD Compatibility Bridge
68.A - Read-only OSD observation
```

Phase 67 is completed. Teletext merged through PR #293 and HbbTV discovery/session/presentation-media runtime merged through PR #300 / `5fe2b73abaeb85f2b5c2cecf7c0c86753aef3d30`.

Completed Teletext scope includes the normalized service/page domain, embedded backend lifecycle/generation fencing, authorized HTTP reads, real 25 x 40 / 1000-cell rendering, Live-TV companion integration and direct Media Home entry.

See [Phase 67 Closeout](phase-67-closeout.md) and [Phase 67 Teletext Closeout](phase-67-teletext-closeout.md).

Phase 68 Legacy OSD Compatibility Bridge is active at 68.A read-only OSD observation. See [Phase 68 Kickoff](phase-68-legacy-osd-kickoff.md) for the durable recovery checkpoint.

## Evidence boundary

Post-phase hardening does not renumber completed history. Historical closeouts retain exact candidate evidence for the state they closed; current status belongs in [Current State](../CURRENT.md).

## Maintenance rules

- Keep this marker aligned with CURRENT, Roadmap, Phase Map and Current Status.
- Keep numbered phases and non-numbered hardening distinguishable.
- Do not promote accepted architecture to implemented runtime without evidence.
- Update the matching closeout whenever a numbered phase or bounded post-phase workstream closes.
