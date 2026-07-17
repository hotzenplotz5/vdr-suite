#!/usr/bin/env python3

from pathlib import Path

WORKFLOW_PATH = Path(".github/workflows/phase-60-15-closeout.yml")
SCRIPT_PATH = Path(__file__)

OLD_LATEST = "Phase 60.14k - Recording Detail UX Polish"
NEW_LATEST = "Phase 60.15 - Recording Metadata and Poster Preparation"
OLD_NEXT = "Phase 60.15 - Recording Metadata and Poster Preparation"
NEW_NEXT = "Phase 61 - Suite Metadata Database and External Provider Integration"


def replace_once(path: str, before: str, after: str, label: str) -> None:
    file_path = Path(path)
    text = file_path.read_text(encoding="utf-8")
    count = text.count(before)
    if count != 1:
        raise SystemExit(f"{path}: {label}: expected one match, found {count}")
    file_path.write_text(text.replace(before, after, 1), encoding="utf-8")


def replace_count(path: str, before: str, after: str, expected: int, label: str) -> None:
    file_path = Path(path)
    text = file_path.read_text(encoding="utf-8")
    count = text.count(before)
    if count != expected:
        raise SystemExit(f"{path}: {label}: expected {expected} matches, found {count}")
    file_path.write_text(text.replace(before, after), encoding="utf-8")


def main() -> None:
    replace_once(
        "README.md",
        f"Latest completed implementation slice:\n\n```text\n{OLD_LATEST}\n```",
        f"Latest completed implementation slice:\n\n```text\n{NEW_LATEST}\n```",
        "latest slice",
    )
    replace_once(
        "README.md",
        f"Next planned implementation slice:\n\n```text\n{OLD_NEXT}\n```",
        f"Next planned implementation phase:\n\n```text\n{NEW_NEXT}\n```",
        "next phase",
    )
    replace_once(
        "README.md",
        f"## Next Work\n\n```text\n{OLD_NEXT}\n```",
        f"## Next Work\n\n```text\n{NEW_NEXT}\n```",
        "next work",
    )

    replace_once(
        "docs/CURRENT.md",
        f"Latest completed implementation slice:\n\n```text\n{OLD_LATEST}\n```",
        f"Latest completed implementation slice:\n\n```text\n{NEW_LATEST}\n```",
        "latest slice",
    )
    replace_once(
        "docs/CURRENT.md",
        f"Next runtime implementation slice:\n\n```text\n{OLD_NEXT}\n```",
        f"Next runtime implementation phase:\n\n```text\n{NEW_NEXT}\n```",
        "next phase",
    )
    replace_once(
        "docs/CURRENT.md",
        """## Immediate Repository Work

Begin Phase 60.15 with an evidence-first audit of:

```text
Recording domain objects
Recording serializers and REST representations
Web Client API Recording contracts
lazy Recording loading and cache ownership
current poster and artwork placeholders
metadata/provider coupling risks
```

The first Phase 60.15 slice must define field ownership and migration boundaries before introducing provider integration.

It must preserve:

- Recording browsing without metadata providers;
- lazy folder loading;
- current backend scope;
- frontend module ownership;
- provider-neutral architecture;
- all existing Recording regression coverage.
""",
        """## Immediate Repository Work

Begin Phase 61 with an evidence-first design of:

```text
MetadataEntity and MetadataAssignment identity
provider, provenance, evidence and confidence contracts
normalized metadata schema and migrations
artwork asset storage and derivative policy
backend-aware provider registry
asynchronous refresh, invalidation and recovery
```

Phase 60.15 is complete. Its provider-scoped source evidence remains internal, while clients consume Suite-owned metadata fields, opaque artwork identities and authenticated artwork URLs.

Phase 61 must preserve:

- Recording browsing without metadata providers;
- lazy folder loading and cached fallback;
- backend scope and provider failure isolation;
- frontend module ownership;
- provider-neutral public contracts;
- migration, backup and recovery coverage.
""",
        "immediate work",
    )
    replace_once(
        "docs/CURRENT.md",
        """```text
1. Phase 60.15 - Recording Metadata Preparation
2. Phase 61 - Suite Metadata Platform
3. Phase 62 - Identity, RBAC and Audit
4. Phase 63 - Backend Agent and Multi-Site Runtime
5. Phase 64 - Timer Intent and Orchestration
6. Phase 65 - Streaming Gateway
7. Phase 66 - Legacy OSD Bridge
8. Phase 67 - Public API and Client Hardening
9. Phase 68 - Recommendation and Knowledge Graph
```""",
        """```text
1. Phase 61 - Suite Metadata Platform
2. Phase 62 - Identity, RBAC and Audit
3. Phase 63 - Backend Agent and Multi-Site Runtime
4. Phase 64 - Timer Intent and Orchestration
5. Phase 65 - Streaming Gateway
6. Phase 66 - Legacy OSD Bridge
7. Phase 67 - Public API and Client Hardening
8. Phase 68 - Recommendation and Knowledge Graph
```""",
        "strict future sequence",
    )

    replace_once(
        "docs/NEW-CHAT-HANDOFF.md",
        f"Latest completed implementation slice:\n\n```text\n{OLD_LATEST}\n```",
        f"Latest completed implementation slice:\n\n```text\n{NEW_LATEST}\n```",
        "latest slice",
    )
    replace_once(
        "docs/NEW-CHAT-HANDOFF.md",
        f"Next runtime implementation slice:\n\n```text\n{OLD_NEXT}\n```",
        f"Next runtime implementation phase:\n\n```text\n{NEW_NEXT}\n```",
        "next phase",
    )
    replace_once(
        "docs/NEW-CHAT-HANDOFF.md",
        "Future execution follows the strict numbered sequence from Phase 60.15 onward.",
        "Future execution follows the strict numbered sequence from Phase 61 onward.",
        "future sequence sentence",
    )

    replace_once(
        "docs/planning/roadmap.md",
        f"Latest completed implementation slice\n{OLD_LATEST}",
        f"Latest completed implementation slice\n{NEW_LATEST}",
        "latest slice",
    )
    replace_once(
        "docs/planning/roadmap.md",
        f"Next runtime implementation slice\n{OLD_NEXT}",
        f"Next runtime implementation phase\n{NEW_NEXT}",
        "next phase",
    )
    replace_once(
        "docs/planning/roadmap.md",
        "Status: **Next runtime implementation slice.**",
        "Status: **Completed.**",
        "phase 60.15 status",
    )
    replace_once(
        "docs/planning/roadmap.md",
        """Non-goals:

- no final metadata database schema;
""",
        """Completed outcomes:

- provider-neutral native, provider-derived and artwork metadata contracts;
- RESTfulAPI metadata enrichment without frontend provider coupling;
- additive SQLite cache persistence with legacy migration and restart coverage;
- deterministic placeholders for metadata-poor recordings;
- Suite-owned opaque artwork IDs and authenticated same-origin URLs;
- allowlisted JPEG, PNG and WebP delivery with traversal, size and symlink defenses;
- lazy Recording folder and detail behavior retained with full CI coverage.

Non-goals retained for Phase 61:

- no final metadata database schema;
""",
        "phase 60.15 outcomes",
    )
    replace_once(
        "docs/planning/roadmap.md",
        "Status: Planned after Phase 60.15.",
        "Status: **Next runtime implementation phase.**",
        "phase 61 status",
    )

    replace_once(
        "docs/planning/phase-map.md",
        "| Phase 60.1-60.14k | Completed | Frontend Platform and Recording UX | Platform bootstrap, lazy Recording cache, folder navigation and detail UX. |",
        "| Phase 60.1-60.15 | Completed | Frontend Platform, Recording UX and Metadata Preparation | Platform bootstrap, lazy Recording cache, detail UX, provider-neutral metadata, persistent artwork preparation and authenticated local artwork delivery. |",
        "completed phase 60 range",
    )
    replace_once(
        "docs/planning/phase-map.md",
        f"Latest completed implementation slice\n{OLD_LATEST}\n\nNext runtime implementation slice\n{OLD_NEXT}",
        f"Latest completed implementation slice\n{NEW_LATEST}\n\nNext runtime implementation phase\n{NEW_NEXT}",
        "current position",
    )
    replace_once(
        "docs/planning/phase-map.md",
        """| 1 | Phase 60.15 | Planned next implementation slice | Recording Metadata Preparation | Add provider-neutral metadata and artwork hooks while preserving lazy Recording behavior. |
| 2 | Phase 61 | Planned | Suite Metadata Platform | Build normalized suite-owned metadata, provider, provenance and artwork services. |
| 3 | Phase 62 | Planned | Identity, RBAC and Audit | Add user, service and Agent identities, scoped authorization and mutation accountability foundation. |
| 4 | Phase 63 | Planned | Backend Agent and Multi-Site Runtime | Implement Agent enrollment, secure transport, generation, lease, health and fenced commands. |
| 5 | Phase 64 | Planned | Timer Intent and Orchestration | Separate intent, assignment and native timers; add scheduler and reconciler. |
| 6 | Phase 65 | Planned | Streaming Gateway | Add authenticated short-lived media sessions over internal providers. |
| 7 | Phase 66 | Planned | Legacy OSD Bridge | Add isolated compatibility sessions with viewer and controller permissions. |
| 8 | Phase 67 | Planned | Public API and Client Hardening | Stabilize `/api/v1`, errors, revisions, compatibility and client contracts. |
| 9 | Phase 68 | Vision | Recommendation and Knowledge Graph | Add explainable recommendations after metadata and platform foundations mature. |""",
        """| 1 | Phase 61 | Planned next runtime phase | Suite Metadata Platform | Build normalized suite-owned metadata, provider, provenance and artwork services. |
| 2 | Phase 62 | Planned | Identity, RBAC and Audit | Add user, service and Agent identities, scoped authorization and mutation accountability foundation. |
| 3 | Phase 63 | Planned | Backend Agent and Multi-Site Runtime | Implement Agent enrollment, secure transport, generation, lease, health and fenced commands. |
| 4 | Phase 64 | Planned | Timer Intent and Orchestration | Separate intent, assignment and native timers; add scheduler and reconciler. |
| 5 | Phase 65 | Planned | Streaming Gateway | Add authenticated short-lived media sessions over internal providers. |
| 6 | Phase 66 | Planned | Legacy OSD Bridge | Add isolated compatibility sessions with viewer and controller permissions. |
| 7 | Phase 67 | Planned | Public API and Client Hardening | Stabilize `/api/v1`, errors, revisions, compatibility and client contracts. |
| 8 | Phase 68 | Vision | Recommendation and Knowledge Graph | Add explainable recommendations after metadata and platform foundations mature. |""",
        "planned phase table",
    )

    replace_once(
        "docs/planning/index.md",
        f"Latest completed implementation slice\n{OLD_LATEST}",
        f"Latest completed implementation slice\n{NEW_LATEST}",
        "latest slice",
    )
    replace_once(
        "docs/planning/index.md",
        f"Next runtime implementation slice\n{OLD_NEXT}",
        f"Next runtime implementation phase\n{NEW_NEXT}",
        "next phase",
    )
    replace_once(
        "docs/planning/index.md",
        """```text
1. Phase 60.15 - Recording Metadata Preparation
2. Phase 61 - Suite Metadata Platform
3. Phase 62 - Identity, RBAC and Audit
4. Phase 63 - Backend Agent and Multi-Site Runtime
5. Phase 64 - Timer Intent and Orchestration
6. Phase 65 - Streaming Gateway
7. Phase 66 - Legacy OSD Bridge
8. Phase 67 - Public API and Client Hardening
9. Phase 68 - Recommendation and Knowledge Graph
```""",
        """```text
1. Phase 61 - Suite Metadata Platform
2. Phase 62 - Identity, RBAC and Audit
3. Phase 63 - Backend Agent and Multi-Site Runtime
4. Phase 64 - Timer Intent and Orchestration
5. Phase 65 - Streaming Gateway
6. Phase 66 - Legacy OSD Bridge
7. Phase 67 - Public API and Client Hardening
8. Phase 68 - Recommendation and Knowledge Graph
```""",
        "strict sequence",
    )

    replace_once(
        "docs/planning/implementation-dependency-map.md",
        "# Step 2 - Phase 60.15 Recording Metadata Representation Preparation\n\n## Inputs",
        "# Step 2 - Phase 60.15 Recording Metadata Representation Preparation\n\nStatus: Completed.\n\n## Inputs",
        "phase 60.15 status",
    )
    replace_once(
        "docs/planning/implementation-dependency-map.md",
        "# Step 3 - Phase 61 Suite Metadata Platform\n\n## Prerequisites",
        "# Step 3 - Phase 61 Suite Metadata Platform\n\nStatus: Next runtime implementation phase.\n\n## Prerequisites",
        "phase 61 status",
    )

    replace_once(
        "docs/planning/tvscraper-recording-metadata-roadmap.md",
        "Status: Planned next implementation slice.",
        "Status: Completed.",
        "phase 60.15 status",
    )
    replace_once(
        "docs/planning/tvscraper-recording-metadata-roadmap.md",
        """Expected preparation:

- identify VDR-owned technical Recording fields
- identify normalized suite metadata fields
- distinguish EPG, plugin, external catalog, sidecar and manual provenance
- define poster and artwork placeholders through suite asset identities
- avoid direct TVScraper or scraper2vdr coupling in frontend modules
- define API fields that degrade cleanly for EPG-only backends
- preserve lazy folder and detail loading behavior
- keep metadata reads backend-scoped and capability-aware

This slice prepares contracts. It does not need to implement the complete Phase 61 provider and persistence system.
""",
        """Completed preparation:

- VDR-owned technical fields remain separate from provider-derived Recording metadata
- RESTfulAPI scraper metadata maps into provider-neutral movie and series/episode value types
- source-scoped artwork references remain internal cache evidence
- Recording metadata persists through the existing SQLite lazy cache and restart path
- deterministic poster placeholders preserve EPG-only and metadata-poor behavior
- Suite-owned opaque artwork IDs replace provider paths at the client boundary
- authenticated local artwork delivery supports JPEG, PNG and WebP below allowlisted roots
- lazy folder and detail loading remain unchanged and regression covered

Phase 60.15 intentionally does not implement the complete Phase 61 normalized metadata entity, assignment, provenance and provider platform.
""",
        "completed preparation",
    )
    replace_once(
        "docs/planning/tvscraper-recording-metadata-roadmap.md",
        "Status: Planned major milestone.",
        "Status: Planned next major milestone.",
        "phase 61 status",
    )

    replace_once(
        "docs/development/current-status.md",
        f"Latest completed implementation slice:\n\n```text\n{OLD_LATEST}\n```",
        f"Latest completed implementation slice:\n\n```text\n{NEW_LATEST}\n```",
        "latest slice",
    )
    replace_once(
        "docs/development/current-status.md",
        "Current implementation focus:\n\n```text\nPhase 58 - Frontend and Live Parity\n```",
        f"Current implementation focus:\n\n```text\n{NEW_NEXT}\n```",
        "current focus",
    )
    replace_once(
        "docs/development/current-status.md",
        f"Next planned implementation slice:\n\n```text\n{OLD_NEXT}\n```",
        f"Next planned implementation phase:\n\n```text\n{NEW_NEXT}\n```",
        "next phase",
    )
    replace_once(
        "docs/development/current-status.md",
        "Phase 60.14k completes the Recording Browser UX polish train after lazy Recording cache hardening. It keeps the server-side lazy folder model, removes duplicate cache-derived folder entries from product views, simplifies single-recording navigation, declutters Recording detail cards, and keeps technical path, ID and size information behind explicit disclosure controls.",
        "Phase 60.15 completes Recording metadata and poster preparation on top of the lazy Recording browser. It separates native and provider-derived fields, persists metadata through the existing cache, keeps deterministic placeholders, exposes opaque Suite-owned artwork identities, and serves authenticated local artwork without exposing provider paths.",
        "latest implementation summary",
    )
    replace_once(
        "docs/development/current-status.md",
        "- Phase 60.14k collapses Recording actions, moves size into technical details, and removes redundant explanatory list text.\n",
        """- Phase 60.14k collapses Recording actions, moves size into technical details, and removes redundant explanatory list text.
- Phase 60.15a-c adds provider-neutral native, provider and artwork metadata contracts plus safe RESTfulAPI mapping.
- Phase 60.15d-f attaches metadata to Recording read models and adds deterministic list/detail poster placeholders.
- Phase 60.15g persists metadata through the existing SQLite Recording cache with additive legacy migration and restart coverage.
- Phase 60.15h-i adds opaque Suite artwork IDs, authenticated allowlisted local image delivery, frontend same-origin loading and placeholder fallback.
- GitHub Actions run `29554158956` verified documentation, frontend, Make inventory, full fast regression, daemon build and packaging for the completed artwork-delivery slice.
""",
        "phase 60.15 evidence",
    )

    replace_once(
        "docs/development/completed-phases-latest.md",
        f"## Latest Completed Implementation Slice\n\n```text\n{OLD_LATEST}\n```",
        f"## Latest Completed Implementation Slice\n\n```text\n{NEW_LATEST}\n```",
        "latest slice",
    )
    replace_once(
        "docs/development/completed-phases-latest.md",
        "## Immediate Repository Work\n\n```text\nADR-0042 through ADR-0049\nArchitecture diagrams\nDomain and implementation dependency maps\n```",
        "## Completed Architecture Prerequisite\n\n```text\nADR-0042 through ADR-0049\nArchitecture diagrams\nDomain and implementation dependency maps\n```",
        "architecture prerequisite",
    )
    replace_once(
        "docs/development/completed-phases-latest.md",
        f"## Next Runtime Implementation Slice\n\n```text\n{OLD_NEXT}\n```",
        f"## Next Runtime Implementation Phase\n\n```text\n{NEW_NEXT}\n```",
        "next phase",
    )

    replace_once(
        "docs/development/completed-phases.md",
        f"## Latest Completed Implementation Slice\n\n```text\n{OLD_LATEST}\n```",
        f"## Latest Completed Implementation Slice\n\n```text\n{NEW_LATEST}\n```",
        "latest slice",
    )
    replace_once(
        "docs/development/completed-phases.md",
        "| Phase 60.1-60.14k | Completed | Frontend platform, lazy Recording cache and Recording detail UX. | [Phase 60](completed-phases/phase-60.md) |",
        "| Phase 60.1-60.15 | Completed | Frontend platform, lazy Recording cache, Recording detail UX, provider-neutral metadata and authenticated local artwork preparation. | [Phase 60](completed-phases/phase-60.md) |",
        "phase 60 range",
    )
    replace_once(
        "docs/development/completed-phases.md",
        """## Next Work Boundary

The next repository work is the ADR-0042 through ADR-0049 architecture contract and diagram package.

The next runtime implementation slice after that package is:

```text
Phase 60.15 - Recording Metadata and Poster Preparation
```
""",
        f"""## Next Work Boundary

The architecture contract package and Phase 60.15 runtime preparation are complete.

The next runtime implementation phase is:

```text
{NEW_NEXT}
```
""",
        "next work boundary",
    )

    replace_once(
        "docs/development/completed-phases/README.md",
        "- Phase 60 records completed frontend platform and Recording UX slices through Phase 60.14k.\n- Planned Phase 60.15 work does not belong here until it is completed.",
        "- Phase 60 records completed frontend platform, Recording UX, metadata preparation and authenticated local artwork delivery through Phase 60.15.\n- Phase 61 normalized metadata platform work remains planned and does not belong here until it is completed.",
        "phase 60 archive semantics",
    )

    replace_once(
        "docs/development/completed-phases/phase-60.md",
        "# Completed Phase 60 Slices - Frontend Platform and Recording UX",
        "# Completed Phase 60 Slices - Frontend Platform, Recording UX and Metadata Preparation",
        "title",
    )
    replace_once(
        "docs/development/completed-phases/phase-60.md",
        "Completed through Phase 60.14k",
        "Completed through Phase 60.15",
        "status",
    )
    replace_once(
        "docs/development/completed-phases/phase-60.md",
        "Phase 60 built the frontend platform foundation and hardened lazy Recording loading, folder navigation and Recording detail behavior.",
        "Phase 60 built the frontend platform foundation, hardened lazy Recording loading and detail behavior, and completed provider-neutral Recording metadata and poster preparation.",
        "scope",
    )
    replace_once(
        "docs/development/completed-phases/phase-60.md",
        "- browser runtime verification and regression coverage through Phase 60.14k.",
        """- browser runtime verification and regression coverage through Phase 60.14k;
- provider-neutral native, provider-derived and artwork metadata value types;
- safe RESTfulAPI metadata enrichment without frontend provider coupling;
- additive SQLite Recording metadata persistence and legacy migration;
- deterministic poster placeholders for metadata-poor recordings;
- opaque Suite-owned artwork IDs and authenticated local JPEG, PNG and WebP delivery;
- traversal, size, unsupported-format and symlink-escape defenses;
- frontend real-poster loading with same-origin validation and placeholder fallback.
""".rstrip(),
        "completed outcomes",
    )
    replace_once(
        "docs/development/completed-phases/phase-60.md",
        f"## Latest Completed Slice\n\n```text\n{OLD_LATEST}\n```",
        f"## Latest Completed Slice\n\n```text\n{NEW_LATEST}\n```",
        "latest slice",
    )
    replace_once(
        "docs/development/completed-phases/phase-60.md",
        """## Next Boundary

The next implementation slice is Phase 60.15, but it starts only after the ADR-0042 through ADR-0049 contract and diagram package is complete.

Phase 60.15 prepares provider-neutral Recording metadata and artwork hooks without implementing the full Phase 61 metadata platform.
""",
        f"""## Next Boundary

The next runtime implementation phase is {NEW_NEXT}.

Phase 61 builds the normalized metadata entity, assignment, provider, provenance, evidence, confidence, storage, refresh and recovery platform on the completed Phase 60.15 representation and artwork boundary.
""",
        "next boundary",
    )

    replace_once(
        "tools/check_completed_phase_markers.py",
        f'LATEST_SLICE = "{OLD_LATEST}"',
        f'LATEST_SLICE = "{NEW_LATEST}"',
        "latest constant",
    )
    replace_once(
        "tools/check_completed_phase_markers.py",
        f'NEXT_RUNTIME = "{OLD_NEXT}"',
        f'NEXT_RUNTIME = "{NEW_NEXT}"',
        "next constant",
    )

    replace_once(
        "tools/check_phase_map_coverage.py",
        f'LATEST_SLICE = "{OLD_LATEST}"',
        f'LATEST_SLICE = "{NEW_LATEST}"',
        "latest constant",
    )
    replace_once(
        "tools/check_phase_map_coverage.py",
        f'NEXT_SLICE = "{OLD_NEXT}"',
        f'NEXT_SLICE = "{NEW_NEXT}"',
        "next constant",
    )
    replace_once(
        "tools/check_phase_map_coverage.py",
        '    "Phase 60.1-60.14k",',
        '    "Phase 60.1-60.15",',
        "completed phase range",
    )

    WORKFLOW_PATH.unlink(missing_ok=False)
    SCRIPT_PATH.unlink(missing_ok=False)


if __name__ == "__main__":
    main()
