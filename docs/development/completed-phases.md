# VDR-Suite Completed Phases

## Purpose

This is the compact authoritative entry point for completed implementation. Detailed historical records remain in [the completed-phase archive](completed-phases/README.md); future numbered work belongs in the Strict Roadmap.

## Latest completed markers

```text
Latest completed numbered runtime phase:
Phase 67 - Broadcast Companion Services: Teletext and HbbTV

Phase-66 closeout merge:
PR #264 -> de12956ecc283663c820865bb577e7dcf6c5f0ee

Latest completed non-numbered Home hardening/rebuild marker:
work/home-rebuild -> 0cce4d1c9e58abe4d529132e92340ae4cbb7a99c
merged to main -> ea5967b983aee9ccc3f855b685db01abbfb2326a

Phase-67 completion:
Teletext -> PR #293 -> d92e7907637122368908ce4c7564a0332db9c487
HbbTV / numbered closeout -> PR #300 -> 5fe2b73abaeb85f2b5c2cecf7c0c86753aef3d30

Current active numbered runtime phase:
Phase 68 - Legacy OSD Compatibility Bridge
```

See [Phase 66 Closeout](phase-66-closeout.md) for the numbered completion gate and [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md) for the subsequent Home correctness/performance completion record.

## Completed range overview

| Range / block | Status | Result | Archive / closeout |
| --- | --- | --- | --- |
| Phase 1.x-45.x | Completed | Core platform, daemon, VDR adapter, multi-backend reads, Recording actions, hardening and EPG search. | Historical phase records |
| Phase 46 | Completed | Metadata and people foundations. | [Phase 46](completed-phases/phase-46.md) |
| Phase 47-50 | Completed | SearchTimer backend, validation and controlled workflow. | [Archive](completed-phases/README.md) |
| Phase 51-55 | Completed | Live parity discovery, preview runtime, adapter hardening and acceptance. | [Archive](completed-phases/README.md) |
| Phase 56 | Completed | Library boundaries, packaging and developer documentation. | [Phase 56](completed-phases/phase-56.md) |
| Phase 57 | Completed | Multi-site backend administration and server-enforced read-only foundation. | [Phase 57](completed-phases/phase-57.md) |
| Phase 58 | Completed slices; historical umbrella retained | Frontend and Live-parity foundation slices. | [Phase 58](completed-phases/phase-58.md) |
| Phase 59 | Completed | Frontend Client API and module ownership. | [Phase 59](completed-phases/phase-59.md) |
| Phase 60 | Completed | Frontend platform, lazy Recording cache, Recordings 2, metadata and artwork preparation. | [Phase 60](completed-phases/phase-60.md) |
| Phase 61 | Completed | Persistent Recording/EPG metadata, people and Genre platform. | [Phase 61 archive](completed-phases/phase-61.md) |
| Phase 62 | Completed | Identity, RBAC, browser-session security and accountability. | [Phase 62 closeout](phase-62-closeout.md) |
| Phase 63 | Completed | Secure Backend Agent lifecycle, fenced native execution and provider ownership. | Phase-63 development records |
| Phase 64 | Completed | Timer intent/assignment/binding orchestration, fulfillment, reconciliation and failover. | [Phase 64 closeout](phase-64-closeout.md) |
| Phase 65 | Completed | Recording/Live MediaSessions, Streaming Gateway, output policy and normalized playback semantics. | [Phase 65 closeout](phase-65-closeout.md) |
| Phase 66 | Completed | Responsive Media Home, Live hero/preview, Continue Watching, discovery/history and Golden desktop/mobile acceptance. | [Phase 66 closeout](phase-66-closeout.md) |
| Phase 67 | Completed | Broadcast Companion Services: normalized Teletext plus fenced HbbTV discovery/application-session/presentation-media runtime; Golden Journeys 8 and 9 accepted. | [Phase 67 closeout](phase-67-closeout.md) |
| Phase 67 Teletext vertical | Completed | Domain-first Teletext service/page path, embedded generation fencing, authorized API, first-party rendering and Golden Journey 8. | [Phase 67 Teletext closeout](phase-67-teletext-closeout.md) |
| Post-Phase-66 Home hardening/rebuild | Completed, non-numbered | Home performance, Recording Discovery, Series metadata/artwork/hierarchy, preview caching, EPG recovery, Movies/Genres presentation and canonical folder artwork. | [Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md) |
| Post-Phase-66 native Recording editing | Completed, non-numbered | VDR-native marks and cutting through Suite safety boundaries. | [Recording Editing Closeout](post-phase66-recording-editing-closeout.md) |

## Phase 66 durable completion marker

```text
phase66_status=COMPLETED
merge_pr=264
merge_commit=de12956ecc283663c820865bb577e7dcf6c5f0ee
PHASE66_GOLDEN_DESKTOP=PASS
PHASE66_GOLDEN_MOBILE=PASS
PHASE66_GOLDEN_ACCEPTANCE=PASS
```

The numbered Phase-66 completion marker remains PR #264. Later merged Home work is post-phase hardening and must not be reclassified as a new Phase-66 slice.

## Post-Phase-66 Home completion marker

```text
accepted_home_rebuild_head=0cce4d1c9e58abe4d529132e92340ae4cbb7a99c
main_merge=ea5967b983aee9ccc3f855b685db01abbfb2326a
phase67_started=YES
phase67_teletext_vertical=COMPLETED
phase67_hbbtv=OPEN
```

The final branch included the canonical folder-poster correction after the broader H0-H5 Home rebuild. Real browser acceptance confirmed canonical TVScraper portrait projection for the reported `The Exorcist` folder case without reintroducing per-Recording metadata HTTP fan-out.

## Completion boundaries

- Phase 61 is not reopened by provider adapters, diagnostics or recommendation work.
- Phase 62 is not reopened by later protected feature routes using its identity/security model.
- Phase 63 is not reopened by Timer/media features using the Agent/provider foundation.
- Phase 64 is not reopened by broad Timer UI work.
- Phase 65 is not reopened by later client, Home, Teletext/HbbTV or OSD work using MediaSession/playback contracts.
- Phase 66 is not reopened by bounded Home correctness/performance work after its accepted closeout.
- Historical runtime fingerprints remain distinct from later daemon/browser evidence.
- ADR acceptance remains separate from runtime completion.

## Next work

```text
Phase 67 - Broadcast Companion Services: Teletext and HbbTV [COMPLETED]
Teletext vertical [COMPLETED]
HbbTV discovery/session/presentation-media runtime [COMPLETED]
Phase 68 - Legacy OSD Compatibility Bridge [ACTIVE: 68.A READ-ONLY OSD OBSERVATION]
```

ADR-0054 remains binding as the completed Broadcast Companion architecture. Phase-67 numbered evidence is recorded in [Phase 67 Closeout](phase-67-closeout.md).

## Verification

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

## Related documents

- [Current State](../CURRENT.md)
- [Latest Completed Marker](completed-phases-latest.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase 67 Closeout](phase-67-closeout.md)
- [Phase 67 Teletext Closeout](phase-67-teletext-closeout.md)
- [Phase 66 Closeout](phase-66-closeout.md)
- [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md)
- [Phase 65 Closeout](phase-65-closeout.md)
- [Phase 64 Closeout](phase-64-closeout.md)
