#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str((ROOT / "tools").resolve()))

from phase_status_contract import CURRENT, replace_current_status_values
from update_phase_status import replace_block_status, replace_inline_status

COMPLETED = "Phase 65 - Streaming Gateway and Media Sessions"
ACTIVE = "none - Phase 66 has not started"
NEXT = "Phase 66 - Media Home and Browse Experience"


def p(rel):
    return ROOT / rel


def read(rel):
    return p(rel).read_text(encoding="utf-8")


def write(rel, text):
    p(rel).write_text(text, encoding="utf-8")


def need(text, old, new, rel, count=1):
    found = text.count(old)
    if found < count:
        raise SystemExit(f"{rel}: expected {count} occurrence(s), found {found}: {old!r}")
    return text.replace(old, new, count)


def section_bounds(text, heading):
    marker = "## " + heading
    start = text.find(marker)
    if start < 0:
        raise SystemExit(f"missing section: {heading}")
    end = text.find("\n## ", start + len(marker))
    if end < 0:
        end = len(text)
    return start, end


def get_section(text, heading):
    start, end = section_bounds(text, heading)
    return text[start:end]


def replace_section(rel, heading, replacement):
    text = read(rel)
    start, end = section_bounds(text, heading)
    write(rel, text[:start] + replacement.strip() + "\n\n" + text[end:].lstrip("\n"))


def assert_contains(rel, *markers):
    text = read(rel)
    for marker in markers:
        if marker not in text:
            raise SystemExit(f"{rel}: missing required marker: {marker}")


# ADR-0058: accepted architecture, but no runtime kickoff.
rel = "docs/adr/ADR-0058-media-home-responsive-browse-preview.md"
text = read(rel)
text = need(text, "**Proposed**", "**Accepted**", rel)
text = need(
    text,
    "This ADR proposes the product and architecture boundary for a Media Home / Browse experience after completed Phase 65. It does not by itself start Phase 66 runtime work. If accepted, the Strict Roadmap and Phase Map must be reconciled explicitly before implementation begins.",
    "This ADR defines the accepted product and architecture boundary for the Media Home / Browse experience after completed Phase 65. Acceptance establishes the Phase-66 planning boundary and future phase sequence; it does not start Phase 66 runtime work. Runtime still requires a separate explicit kickoff.",
    rel,
)
text = need(text, "# Proposed numbered-phase reconciliation", "# Accepted numbered-phase reconciliation", rel)
text = need(text, "If this ADR is accepted, the proposed sequence becomes:", "The accepted future sequence is:", rel)
text = need(text, "# Proposed Phase-66 slices", "# Phase-66 slices", rel)
text = need(text, "Proposed order:", "Accepted implementation order:", rel)
text = need(text, "# Acceptance requirements for this ADR", "# Acceptance record for this ADR", rel)
text = need(
    text,
    "Before this ADR becomes **Accepted**:",
    "ADR-0058 is accepted because the following planning conditions are satisfied:",
    rel,
)
text = need(
    text,
    "After acceptance, Phase 66 still requires a separate explicit runtime kickoff before implementation starts.",
    "Acceptance does not start runtime work. Phase 66 still requires a separate explicit runtime kickoff before implementation starts.",
    rel,
)
text = text.replace("  -> proposes Media Home architecture and future sequencing reconciliation\n  -> does not start Phase 66 runtime", "  -> accepts Media Home architecture and future sequencing reconciliation\n  -> does not start Phase 66 runtime")
write(rel, text)

# Accepted Phase-66 planning contract.
rel = "docs/development/phase-66-media-home-browse-experience.md"
text = read(rel)
text = need(text, "Status: **Proposed implementation contract under ADR-0058. Runtime not started.**", "Status: **Accepted implementation contract under ADR-0058. Runtime not started.**", rel)
text = need(text, "# Proposed future phase sequence", "# Accepted future phase sequence", rel)
text = need(text, "If ADR-0058 is accepted, the intended numbered sequence is:", "Under accepted ADR-0058, the numbered sequence is:", rel)
text = need(
    text,
    "Merging this planning contract does not implicitly authorize Phase-66 runtime implementation unless the accompanying ADR/roadmap reconciliation explicitly marks ADR-0058 accepted and a separate Phase-66 kickoff is given.",
    "Merging this accepted planning contract does not implicitly authorize Phase-66 runtime implementation. ADR-0058 and the roadmap establish architecture and phase order; a separate explicit Phase-66 runtime kickoff is still required.",
    rel,
)
write(rel, text)

# ADR index: move 0058 from Proposed to Accepted and make sequencing authoritative.
rel = "docs/adr/index.md"
text = read(rel)
line57 = "- [ADR-0057: Recording Network Interruption Recovery](ADR-0057-recording-network-interruption-recovery.md)\n"
line58 = "- [ADR-0058: Media Home, Responsive Browse and Preview Experience](ADR-0058-media-home-responsive-browse-preview.md)\n"
if text.count(line58) != 1:
    raise SystemExit(f"{rel}: expected exactly one pre-acceptance ADR-0058 entry")
text = text.replace(line57, line57 + line58)
text = need(text, "Next available canonical ADR after the current proposal:", "Next available canonical ADR:", rel)
text = need(text, "## Proposed Canonical ADRs\n\n- [ADR-0058: Media Home, Responsive Browse and Preview Experience](ADR-0058-media-home-responsive-browse-preview.md)", "## Proposed Canonical ADRs\n\nNone.", rel)
start, end = section_bounds(text, "Future-phase sequencing note")
sequencing = """## Future-phase sequencing note

The Strict Roadmap owns phase numbering/order. Accepted ADR-0058 establishes the not-yet-started post-Phase-65 sequence:

```text
65 Streaming [completed]
66 Media Home and Browse Experience
67 Broadcast Companion: Teletext + HbbTV
68 Legacy OSD
69 Public API Hardening
70 Recommendation / Knowledge Graph
```

ADR-0054, ADR-0047 and ADR-0048 retain their architecture decisions. ADR-0058 supersedes only their older not-yet-started phase-number statements. Phase 66 Media Home remains runtime-not-started until a separate explicit kickoff."""
text = text[:start] + sequencing + "\n\n" + text[end:].lstrip("\n")
text = text.replace("[Proposed Phase 66 Media Home contract]", "[Phase 66 Media Home contract]")
write(rel, text)

# ADR-0054 keeps architecture; only future numbering changes.
rel = "docs/adr/ADR-0054-broadcast-companion-teletext-hbbtv.md"
text = read(rel)
text = need(
    text,
    "Accepted during the post-Phase-64 roadmap reconciliation. Acceptance establishes the architecture and future phase sequencing only; it does not start Phase-66 runtime implementation.",
    "Accepted during the post-Phase-64 roadmap reconciliation. The Broadcast Companion architecture remains accepted. Its original future phase numbering is superseded by accepted ADR-0058: Broadcast Companion runtime is now Phase 67 and is not started by this ADR.",
    rel,
)
start, end = section_bounds(text, "Sequencing Decision")
seq54 = """# Sequencing Decision

Accepted ADR-0058 now owns the not-yet-started future sequence after completed Phase 65:

```text
Phase 65 - Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 - Media Home and Browse Experience
  -> Phase 67 - Broadcast Companion Services: Teletext and HbbTV
  -> Phase 68 - Legacy OSD Compatibility Bridge
  -> Phase 69 - Public API and Client Compatibility Hardening
  -> Phase 70 - Recommendation and Content Knowledge Graph
```

This supersedes only ADR-0054's former phase-number sequencing statement. It does **not** supersede the Broadcast Companion architecture in this ADR, nor ADR-0047 or ADR-0048.

Completed history through Phase 65 is unchanged. Phase 66 Media Home is next and remains not started until a separate explicit kickoff. Broadcast Companion runtime follows as Phase 67."""
text = text[:start] + seq54 + "\n\n" + text[end:].lstrip("\n")
text = need(
    text,
    "After Phase 65 closes and Phase 66 is explicitly authorized, Phase 66 should use coherent verticals rather than micro-slices.",
    "After Phase 66 Media Home closes and Phase 67 Broadcast Companion is explicitly authorized, Broadcast Companion implementation should use coherent verticals rather than micro-slices.",
    rel,
)
write(rel, text)

# Canonical phase tuple and its mechanical mirrors.
CURRENT.write_text(replace_current_status_values(CURRENT.read_text(encoding="utf-8"), completed=COMPLETED, active=ACTIVE, next_phase=NEXT), encoding="utf-8")
for rel in ["docs/planning/roadmap.md", "docs/planning/phase-map.md"]:
    replace_block_status(p(rel), COMPLETED, ACTIVE, NEXT)
for rel in ["docs/NEW-CHAT-HANDOFF.md", "docs/development/current-status.md"]:
    replace_inline_status(p(rel), COMPLETED, ACTIVE, NEXT)

# CURRENT authority.
rel = "docs/CURRENT.md"
text = read(rel)
text = text.replace("[Phase 65 Final Closeout](development/phase-65-closeout.md)", "[Phase 65 Closeout](development/phase-65-closeout.md)")
anchor = "- [ADR-0057 Recording Network Interruption Recovery](adr/ADR-0057-recording-network-interruption-recovery.md)\n"
if "ADR-0058 Media Home" not in text:
    text = need(text, anchor, anchor + "- [ADR-0058 Media Home, Responsive Browse and Preview Experience](adr/ADR-0058-media-home-responsive-browse-preview.md)\n- [Phase 66 Media Home and Browse Experience](development/phase-66-media-home-browse-experience.md)\n", rel)
text = need(
    text,
    "Phase 66 is the next strict numbered runtime phase but is **not started**. Accepted ADR-0054 defines its architecture only. Teletext/HbbTV runtime remains blocked until a separate explicit Phase-66 kickoff.",
    "Phase 66 is the next strict numbered runtime phase but is **not started**. Accepted ADR-0058 defines the Media Home / Browse architecture and bounded Phase-66 implementation contract. Architecture acceptance does not authorize runtime; a separate explicit Phase-66 kickoff is required. Accepted ADR-0054 remains the Broadcast Companion architecture for the now-following Phase 67.",
    rel,
)
old = """Phase 64 - Timer Intent and Multi-Backend Orchestration [COMPLETED]
  -> Phase 65 - Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 - Broadcast Companion Services: Teletext and HbbTV [NEXT; NOT STARTED]
  -> Phase 67 - Legacy OSD Compatibility Bridge
  -> Phase 68 - Public API and Client Compatibility Hardening
  -> Phase 69 - Recommendation and Content Knowledge Graph"""
new = """Phase 64 - Timer Intent and Multi-Backend Orchestration [COMPLETED]
  -> Phase 65 - Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 - Media Home and Browse Experience [NEXT; NOT STARTED]
  -> Phase 67 - Broadcast Companion Services: Teletext and HbbTV
  -> Phase 68 - Legacy OSD Compatibility Bridge
  -> Phase 69 - Public API and Client Compatibility Hardening
  -> Phase 70 - Recommendation and Content Knowledge Graph"""
text = need(text, old, new, rel)
text = text.replace("The Phase-66 Broadcast Companion architecture is defined by accepted ADR-0054. Teletext/HbbTV runtime remains blocked until Phase 66 is explicitly started.", "The Phase-66 Media Home architecture is defined by accepted ADR-0058. Runtime remains blocked until Phase 66 is explicitly started. Broadcast Companion architecture remains defined by ADR-0054 for Phase 67.")
write(rel, text)

# Handoff current-oriented sections.
replace_section("docs/NEW-CHAT-HANDOFF.md", "Mandatory reading order", """## Mandatory reading order

1. [Current State](CURRENT.md) — sole volatile phase/status authority.
2. [Strict Roadmap](planning/roadmap.md) and [Phase Map](planning/phase-map.md) — accepted numbered execution order.
3. [Phase 65 Closeout](development/phase-65-closeout.md) — completed Streaming/MediaSession/playback boundary and durable acceptance evidence.
4. [ADR-0058 Media Home, Responsive Browse and Preview Experience](adr/ADR-0058-media-home-responsive-browse-preview.md) — accepted Phase-66 product/architecture decision.
5. [Phase 66 Media Home and Browse Experience](development/phase-66-media-home-browse-experience.md) — accepted bounded implementation sequence; runtime not started.
6. [Golden User Journeys](planning/golden-user-journeys.md) — desktop/mobile Home and later product acceptance.
7. ADR-0046/0053/0055/0056/0057 when Home work touches accepted Phase-65 playback semantics.
8. [ADR-0054 Broadcast Companion Services](adr/ADR-0054-broadcast-companion-teletext-hbbtv.md) for later Phase-67 Teletext/HbbTV architecture.
9. [Target Platform Architecture](architecture/target-platform-architecture.md), [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md) and [ADR Index](adr/index.md) as required.
10. [Agent Workflow Rules](../AGENTS.md) before repository writes, PR-state changes or installation guidance.

[Current Project Status](development/current-status.md), [Current Architecture State](development/current-architecture-state.md), [Completed Phases](development/completed-phases.md), [Phase 64 Closeout](development/phase-64-closeout.md) and older Phase-65 development records provide stable historical context.""")
replace_section("docs/NEW-CHAT-HANDOFF.md", "Stable project position", """## Stable project position

- Latest completed numbered runtime phase: **Phase 65 - Streaming Gateway and Media Sessions**.
- Current active numbered runtime phase: **none; Phase 66 has not started**.
- Next strict numbered runtime phase: **Phase 66 - Media Home and Browse Experience**.
- Phase 65.A through 65.D are closed for their accepted bounded scopes; ADR-0056 semantic consolidation and ADR-0057 Recording network recovery are completed Phase-65 history.
- ADR-0058 is accepted and owns the Media Home / responsive browse / deferred-preview architecture.
- The accepted Phase-66 contract is planning authority only. Runtime still requires a separate explicit kickoff.
- ADR-0054 remains accepted Broadcast Companion architecture and is now sequenced as Phase 67.
- Broad polished Timer UI remains a cross-cutting milestone gated on required access administration.

Do not copy current branch SHA or active PR tip here; use `CURRENT.md` and GitHub.""")
replace_section("docs/NEW-CHAT-HANDOFF.md", "Current implementation boundary", """## Current implementation boundary

Phase 65 is completed. Phase 66 - Media Home and Browse Experience is next but has not started.

Before any Phase-66 runtime work:

1. re-read live `main`, `CURRENT.md`, ADR-0058 and the Strict Roadmap;
2. verify no Phase-66 runtime branch has already started elsewhere;
3. create a fresh runtime branch from the accepted planning baseline;
4. begin only with Slice 66.1 — Home Shell and Responsive Information Architecture;
5. preserve existing Channel/EPG/Recording/Metadata truth and the completed Phase-65 MediaSession/playback-owner architecture;
6. do not pull deferred preview, history persistence, Teletext/HbbTV, Legacy OSD, public-API hardening or native-app work into Slice 66.1.

Architecture acceptance is not runtime authorization; do not start Phase 66. A separate explicit kickoff is required.""")
replace_section("docs/NEW-CHAT-HANDOFF.md", "Phase ordering and broad Timer UI", """## Phase ordering and broad Timer UI

```text
Phase 64 reliable Timer orchestration engine [COMPLETED]
  -> Phase 65 Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 Media Home and Browse Experience [NEXT; NOT STARTED]
  -> Phase 67 Broadcast Companion Services: Teletext and HbbTV
  -> Phase 68 Legacy OSD Compatibility Bridge
  -> Phase 69 Public API and Client Compatibility Hardening
  -> Phase 70 Recommendation and Content Knowledge Graph
```

Completed history is not renumbered. ADR-0058 changes only the not-yet-started future sequence; ADR-0054/0047/0048 retain their architecture.

Broad Timer UI remains non-numbered and depends on completed Phase 62 + completed Phase 64 + required account/backend access administration.""")
rel = "docs/NEW-CHAT-HANDOFF.md"
text = read(rel)
text = text.replace("## Streaming planning already exists", "## Completed Phase 65 media architecture")
text = text.replace("client playback abstraction [65.D ACTIVE]", "client playback abstraction [65.D CLOSED]")
text = text.replace("ADR-0056 playback semantic consolidation [ACTIVE]", "ADR-0056 playback semantic consolidation [CLOSED]")
text = text.replace("## Broadcast Companion planning", "## Phase 67 Broadcast Companion planning")
text = text.replace("Teletext and HbbTV are planned as normal television-domain capabilities, not as Legacy OSD shortcuts.", "Teletext and HbbTV remain planned as normal television-domain capabilities for Phase 67, not as Legacy OSD shortcuts.")
write(rel, text)

# Stable current status narrative.
replace_section("docs/development/current-status.md", "Platform position", """## Platform position

Latest completed numbered runtime phase: **Phase 65 - Streaming Gateway and Media Sessions**.

Current active numbered runtime phase: **none; Phase 66 has not started**.

Next strict numbered runtime phase: **Phase 66 - Media Home and Browse Experience**.

Phase 65.A through 65.D are completed for their accepted bounded scopes. The final runtime-sensitive follow-up was ADR-0057 bounded completed-Recording network recovery. Durable evidence is in [Phase 65 Closeout](phase-65-closeout.md).

Accepted ADR-0058 and [Phase 66 Media Home and Browse Experience](phase-66-media-home-browse-experience.md) define the responsive Home / Browse architecture and bounded implementation sequence. Phase 66 remains runtime-not-started pending a separate kickoff. ADR-0054 remains Broadcast Companion architecture for Phase 67.

Historical completed context includes Phases 58, 61, 62, 63 and 64.""")
replace_section("docs/development/current-status.md", "Revised forward ordering", """## Revised forward ordering

```text
Phase 64 reliable Timer orchestration engine [COMPLETED]
  -> Phase 65 Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 Media Home and Browse Experience [NEXT; NOT STARTED]
  -> Phase 67 Broadcast Companion Services: Teletext and HbbTV
  -> Phase 68 Legacy OSD Compatibility Bridge
  -> Phase 69 Public API and Client Compatibility Hardening
  -> Phase 70 Recommendation and Content Knowledge Graph
```

Completed history through Phase 65 is unchanged. ADR-0058 owns the new future sequence. Phase 66 has not started and requires a separate explicit runtime kickoff.""")
rel = "docs/development/current-status.md"
text = read(rel)
text = text.replace("## Active Phase 65 streaming architecture", "## Completed Phase 65 streaming architecture")
text = text.replace("The active media direction is provider-private and transformation-minimal:", "The accepted Phase-65 media direction is provider-private and transformation-minimal:")
text = text.replace("Phase 65.D is the active client playback abstraction vertical:", "Phase 65.D is the completed client playback abstraction vertical:")
text = text.replace("Phase 65.D remains active.", "Phase 65.D is completed.")
text = text.replace("**Phase 65.D remains active.**", "**Phase 65.D is completed.**")
write(rel, text)

# Strict Roadmap current position.
replace_section("docs/planning/roadmap.md", "Current phase position", """## Current phase position

```text
Latest completed numbered runtime phase:
Phase 65 - Streaming Gateway and Media Sessions

Current active numbered runtime phase:
none - Phase 66 has not started

Next strict numbered runtime phase:
Phase 66 - Media Home and Browse Experience
```

Phase 65 is completed. Durable evidence lives in [Phase 65 Closeout](../development/phase-65-closeout.md). ADR-0058 is accepted and defines the next Media Home / Browse architecture, but Phase 66 runtime has not started and still requires a separate explicit kickoff.

Future order: Phase 66 Media Home -> Phase 67 Broadcast Companion -> Phase 68 Legacy OSD -> Phase 69 Public API hardening -> Phase 70 Recommendation / Knowledge Graph.""")
rel = "docs/planning/roadmap.md"
text = read(rel)
p65s, p65e = section_bounds(text, "Phase 65 — Streaming Gateway and Media Sessions")
p65 = text[p65s:p65e]
p65 = re.sub(r"Status: \*\*[^\n]+\*\*", "Status: **Completed.**", p65, count=1)
p65 = p65.replace("#### 65.D — Client playback abstraction — ACTIVE", "#### 65.D — Client playback abstraction — CLOSED")
p65 = p65.replace("**ADR-0056 Playback semantic consolidation — ACTIVE**", "**ADR-0056 Playback semantic consolidation — CLOSED**")
p65 = p65.replace("Mandatory Phase-65.D work is 1-4.", "Mandatory Phase-65.D work was 1-4 and is completed.")
p65 = p65.replace("Phase 65 is already active. Phase 65.A through 65.C are closed for their bounded accepted scopes; Phase 65.D remains active under ADR-0056. Phase 66 remains blocked until the complete Phase-65 gate is satisfied and a separate Phase-66 kickoff is explicit.", "Phase 65 is completed. Phase 65.A through 65.D are closed for their bounded accepted scopes. See [Phase 65 Closeout](../development/phase-65-closeout.md). Phase 66 Media Home remains runtime-blocked until a separate explicit kickoff.")
text = text[:p65s] + p65 + text[p65e:]
write(rel, text)

# Shift the four existing future roadmap sections and insert new Phase 66.
text = read(rel)
bcast = get_section(text, "Phase 66 — Broadcast Companion Services: Teletext and HbbTV")
legacy = get_section(text, "Phase 67 — Legacy OSD Compatibility Bridge")
api = get_section(text, "Phase 68 — Public API and Client Compatibility Hardening")
rec = get_section(text, "Phase 69 — Recommendation and Content Knowledge Graph")
bcast = bcast.replace("## Phase 66 — Broadcast Companion Services: Teletext and HbbTV", "## Phase 67 — Broadcast Companion Services: Teletext and HbbTV", 1)
bcast = bcast.replace("Status: **Planned after Phase 65; architecture accepted, runtime not started.**", "Status: **Planned after Phase 66; architecture accepted, runtime not started.**", 1)
bcast = bcast.replace("Runtime implementation remains blocked until Phase 65 closes and Phase 66 is explicitly started.", "Runtime implementation follows completed Phase 66 Media Home and remains blocked until Phase 67 is explicitly started.")
bcast = bcast.replace("### 66.A", "### 67.A").replace("### 66.B", "### 67.B").replace("### 66.C", "### 67.C")
bcast = bcast.replace("### Phase-66 safety invariants", "### Phase-67 safety invariants").replace("### Phase-66 acceptance gate", "### Phase-67 acceptance gate").replace("not required to close Phase 66", "not required to close Phase 67")
legacy = legacy.replace("## Phase 67 — Legacy OSD Compatibility Bridge", "## Phase 68 — Legacy OSD Compatibility Bridge", 1).replace("Status: **Planned after Phase 66.**", "Status: **Planned after Phase 67.**", 1)
for suffix in "ABCDE":
    legacy = legacy.replace(f"#### 67.{suffix}", f"#### 68.{suffix}")
legacy = legacy.replace("### Phase-67 acceptance gate", "### Phase-68 acceptance gate")
api = api.replace("## Phase 68 — Public API and Client Compatibility Hardening", "## Phase 69 — Public API and Client Compatibility Hardening", 1).replace("Status: **Planned after Phase 67.**", "Status: **Planned after Phase 68.**", 1)
for suffix in "ABCDEF":
    api = api.replace(f"#### 68.{suffix}", f"#### 69.{suffix}")
api = api.replace("### Phase-68 acceptance gate", "### Phase-69 acceptance gate")
rec = rec.replace("## Phase 69 — Recommendation and Content Knowledge Graph", "## Phase 70 — Recommendation and Content Knowledge Graph", 1).replace("stable public resource semantics from Phase 68;", "stable public resource semantics from Phase 69;")
home = """## Phase 66 — Media Home and Browse Experience

Status: **Next; not started.**

Binding architecture: [ADR-0058: Media Home, Responsive Browse and Preview Experience](../adr/ADR-0058-media-home-responsive-browse-preview.md).

Binding implementation contract: [Phase 66 Media Home and Browse Experience](../development/phase-66-media-home-browse-experience.md).

### Phase goal

Turn completed Phase-65 media and existing Channel/EPG/Recording/Metadata domains into one responsive first-party landing/browse experience without creating a second source of truth or playback lifecycle.

Core rule: **Browse first, playback second.** Browse focus updates immediately; optional Live preview is deferred, cancelable and attached only through the existing Phase-65 MediaSession / canonical playback owner.

### Coherent implementation sequence

```text
66.1 Home Shell and Responsive Information Architecture
66.2 Live-TV Hero Carousel
66.3 Deferred Live Preview
66.4 Continue Watching
66.5 Recording Discovery Rails
66.6 Recently Watched / History
66.7 Visual Polish and Accessibility
66.8 Golden User Journey and Real-System Acceptance
```

### Hard invariants

- Home projects existing Channel, ProgramEvent, Recording, Metadata, Genre and artwork truth.
- No Home-specific media identity, metadata database, MediaSession owner, restart path or cleanup engine.
- Browse focus, preview intent and explicit `Watch Live` remain distinct.
- Rapid focus movement creates no preview session; stale preview startup cannot attach after focus moves.
- Desktop/tablet/mobile use responsive recomposition, not a scaled desktop page.
- Continue Watching requires truthful resume evidence; Recently Watched is separate history semantics.
- Browser-local state is not silently promoted to cross-client truth.
- Teletext/HbbTV, Legacy OSD, public API hardening, recommendations, native apps and Live timeshift are outside Phase 66.

### Phase-66 acceptance gate

Phase 66 closes only when Home is the accepted first-party landing experience, responsive desktop/mobile browse works without waiting for preview, deferred preview uses canonical Phase-65 ownership/cleanup, Continue Watching and Recording rails are truthful, history has explicit semantics if durable, Golden Home journeys pass on real supported environments, and exact final CI/packaging/rollback gates pass.

Architecture acceptance does not start this phase. A separate explicit runtime kickoff is required before Slice 66.1."""
start66, _ = section_bounds(text, "Phase 66 — Broadcast Companion Services: Teletext and HbbTV")
_, end69 = section_bounds(text, "Phase 69 — Recommendation and Content Knowledge Graph")
text = text[:start66] + home + "\n\n---\n\n" + bcast.strip() + "\n\n---\n\n" + legacy.strip() + "\n\n---\n\n" + api.strip() + "\n\n---\n\n" + rec.strip() + "\n\n" + text[end69:].lstrip("\n")
text = text.replace("Independent/third-party client compatibility becomes a formal Phase-68 contract.", "Independent/third-party client compatibility becomes a formal Phase-69 contract.")
write(rel, text)

# Phase Map compact authority.
replace_section("docs/planning/phase-map.md", "Current position", """## Current position

```text
Latest completed numbered runtime phase:
Phase 65 - Streaming Gateway and Media Sessions

Current active numbered runtime phase:
none - Phase 66 has not started

Next strict numbered runtime phase:
Phase 66 - Media Home and Browse Experience
```

Read [Current State](../CURRENT.md) and [Phase 65 Closeout](../development/phase-65-closeout.md) for exact operational/evidence state. ADR-0058 and the [Phase 66 Media Home contract](../development/phase-66-media-home-browse-experience.md) define the accepted next planning boundary; runtime is not started.""")
replace_section("docs/planning/phase-map.md", "Revised numbered forward sequence", """## Revised numbered forward sequence

| Order | Phase | Status | Track | Primary completion direction |
| ---: | --- | --- | --- | --- |
| 1 | Phase 64 | Completed | Timer Intent and Multi-Backend Orchestration | Reliable Timer intent/assignment/binding orchestration and controlled failover. |
| 2 | Phase 65 | Completed | Streaming Gateway and Media Sessions | Authenticated Recording/Live playback, least-transformation delivery/output policy and normalized persistent playback semantics. |
| 3 | Phase 66 | Next; not started | Media Home and Browse Experience | Responsive Home, immediate browse, deferred canonical preview, truthful Continue Watching and product acceptance. |
| 4 | Phase 67 | Planned after Phase 66 | Broadcast Companion Services: Teletext and HbbTV | Domain-first Teletext and broadcast-application runtime. |
| 5 | Phase 68 | Planned after Phase 67 | Legacy OSD Compatibility Bridge | Isolated OSD observation/control compatibility. |
| 6 | Phase 69 | Planned after Phase 68 | Public API and Client Compatibility Hardening | Stable `/api/v1` and independent-client contracts. |
| 7 | Phase 70 | Vision | Recommendation and Content Knowledge Graph | Explainable provenance-aware recommendations after prerequisites. |""")
replace_section("docs/planning/phase-map.md", "Phase 65 compact boundary", """## Phase 65 compact boundary

Binding decisions: ADR-0046 + ADR-0053 + ADR-0055 + ADR-0056 + ADR-0057.

```text
private media source
  -> explicitly owned provider / ProviderStreamLease
  -> least-transformation adaptation
  -> Streaming Gateway / MediaSession
  -> normalized MediaPlaybackContract
  -> persistent client playback owner
  -> replaceable transport adapter
  -> platform playback engine
```

Phase 65 is completed. Growing-Recording seek and Live-TV timeshift remain truthful deferred capabilities rather than unfinished Phase 65. Durable evidence: [Phase 65 Closeout](../development/phase-65-closeout.md).""")
rel = "docs/planning/phase-map.md"
text = read(rel)
start, _ = section_bounds(text, "Phase 66 compact boundary")
_, end = section_bounds(text, "Phase 69 compact boundary")
compact = """## Phase 66 compact boundary

Binding architecture: accepted ADR-0058; implementation contract: [Phase 66 Media Home and Browse Experience](../development/phase-66-media-home-browse-experience.md).

```text
existing Channel / EPG / Recording / Metadata truth
  -> responsive Home projection
  -> immediate browse focus
  -> optional deferred preview through canonical Phase-65 playback ownership
  -> explicit full playback through existing owners
```

Phase 66 is next but not started.

## Phase 67 compact boundary

Binding architecture: accepted ADR-0054.

```text
Live Channel / ProgramEvent
  +--> TeletextService -> Page/Subpage
  +--> BroadcastApplication -> HbbTV Application Session
```

## Phase 68 compact boundary

Binding architecture: ADR-0047. Legacy OSD remains compatibility-only; domain-first Home/EPG/Timer/Recording/Streaming/Teletext/HbbTV remains preferred.

## Phase 69 compact boundary

Binding architecture: ADR-0048. Stabilize `/api/v1`, errors, revisions/preconditions/idempotency, deterministic collections and compatibility/deprecation contracts.

## Phase 70 compact boundary

Recommendation/knowledge-graph runtime requires its own accepted ADR and does not gain hidden mutation authority."""
text = text[:start] + compact + "\n\n" + text[end:].lstrip("\n")
write(rel, text)
replace_section("docs/planning/phase-map.md", "Product acceptance", """## Product acceptance

- Phase 64: Timer scheduling/fail-closed engine journeys.
- Phase 65: completed Live-TV and Recording-playback journeys.
- Phase 66: desktop and mobile Media Home browse/preview journeys.
- Phase 67: Teletext and HbbTV journeys.
- Phase 68: explicit Legacy OSD compatibility journey.
- Phase 69: public/client compatibility hardening.
- Broad Timer Product UI later completes its user-facing journey without reopening Phase 64.
- Phase 70 recommendation work must add its own accepted journey.

See [Golden User Journeys](golden-user-journeys.md).""")
replace_section("docs/planning/phase-map.md", "Numbering rules", """## Numbering rules

- Completed history is never renumbered.
- Phases 61 through 65 are closed for their accepted scopes.
- Phase 66 Media Home and Browse Experience is next and has not started.
- ADR-0058 acceptance is planning authority, not runtime kickoff.
- Phase 67 Broadcast Companion retains ADR-0054 architecture.
- Phase 68 Legacy OSD retains ADR-0047 architecture.
- Phase 69 Public API hardening retains ADR-0048 architecture.
- Phase 70 Recommendation / Knowledge Graph remains vision and requires its own accepted runtime ADR.
- Future not-yet-started phases may be reordered only through explicit repository reconciliation.
- Cross-cutting product/admin work does not silently advance the numbered phase.""")

# Golden Journeys: two explicit Home journeys; shift later journey numbers/phases.
rel = "docs/planning/golden-user-journeys.md"
text = read(rel)
start, _ = section_bounds(text, "Journey 6 — Teletext while watching Live TV")
_, relationship_end = section_bounds(text, "Relationship to phase completion")
journeys = """## Journey 6 — Browse Media Home on desktop

```text
open VDR-Suite
  -> Home useful before preview
  -> browse Live hero rapidly
  -> Now/Next and artwork follow focus immediately
  -> focus settles
  -> one delayed Live preview attaches
  -> explicit Watch Live
  -> return Home
  -> Continue Watching / Recording discovery rail
```

Acceptance proves browse focus is independent of playback/session state, rapid movement creates no preview sessions, stale preview cannot attach after focus changes, preview relinquishes through the canonical Phase-65 owner, existing domain identities are reused, and keyboard/reduced-motion behavior remains usable.

This is a Phase-66 product journey under ADR-0058. Runtime remains not started until separately authorized.

---

## Journey 7 — Browse Media Home on a phone

```text
open VDR-Suite on phone
  -> one dominant Live hero with neighbor peeks
  -> swipe channels
  -> Now/Next follows focus immediately
  -> settled focus may preview inside hero
  -> Watch Live / EPG touch actions
  -> Continue Watching rail
  -> bottom navigation Home / Live / Recordings / Search / More
```

Acceptance proves mobile is semantic recomposition rather than scaled desktop, swipe/touch never waits on MediaSession startup, obsolete preview work is canceled/relinquished, no persistent floating mini-player steals the viewport, and canonical content/playback identities are shared with desktop.

This is a Phase-66 product journey under ADR-0058.

---

## Journey 8 — Teletext while watching Live TV

Live TV -> Teletext indication -> open -> page/subpage navigation -> close -> Live remains usable.

Acceptance uses Suite Teletext service/page contracts, truthful freshness, correct backend/channel identity and no raw VDR/plugin command channel. This is a Phase-67 journey under ADR-0054.

---

## Journey 9 — Launch one HbbTV broadcast application

Live Channel -> discovered app -> authorized BroadcastApplicationSession -> isolated runtime -> normalized input -> close/channel change -> cleanup.

Acceptance proves bounded discovery, no unrestricted browser/plugin control endpoint, isolation from Suite secrets, stale-context fencing and reuse of Phase-65 MediaSession semantics for Suite-owned media. This is a Phase-67 journey under ADR-0054.

---

## Journey 10 — Use one legacy native OSD workflow safely

Explicit Legacy OSD -> authorized session -> authoritative frame -> optional fenced controller lease -> allowlisted input -> resulting frame -> close.

Acceptance proves domain-first features are not routed through OSD when normal contracts exist, view/control remain separate and no arbitrary command tunnel exists. This is a Phase-68 journey.

---

## Journey 11 — Manage a Timer safely through the broad Timer UI

This remains a cross-cutting product milestone. EPG/Timer -> permission -> revision-safe TimerIntent mutation -> visible assignment/fulfillment -> authoritative readback/reconciliation -> final state.

Acceptance preserves intent-first ownership, read-only enforcement, truthful `outcome_unknown`, no unsafe blind retry and no browser use of private SuiteBridge/SVDRP Timer writes.

---

## Relationship to phase completion

```text
Phase 64 [completed] -> engine portions of Journeys 3, 4 and Timer-related Journey 5
Phase 65 [completed] -> Journeys 1 and 2 + media Journey 5
Phase 66 [next; not started] -> Journeys 6 and 7
Phase 67 -> Journeys 8 and 9
Phase 68 -> Journey 10
Broad Timer Product UI -> Journey 11 + user-facing Journey 3
Phase 69 -> public/client compatibility hardening
```

Phase 70 recommendation work must add its own user-visible journey before runtime acceptance."""
text = text[:start] + journeys + "\n\n" + text[relationship_end:].lstrip("\n")
text = text.replace("This is cross-cutting and reused by Phases 64–68.", "This is cross-cutting and reused by Phases 64–69.")
write(rel, text)

# Documentation and planning indexes.
rel = "docs/index.md"
text = read(rel)
text = text.replace("- [Phase 65.D Playback Semantics Consolidation](development/phase-65d-playback-semantics-consolidation.md) — active bounded implementation contract under ADR-0056.", "- [Phase 65 Closeout](development/phase-65-closeout.md) — completed Streaming/MediaSession/playback boundary and final acceptance evidence.\n- [ADR-0058 Media Home, Responsive Browse and Preview Experience](adr/ADR-0058-media-home-responsive-browse-preview.md) — accepted next-phase architecture.\n- [Phase 66 Media Home and Browse Experience](development/phase-66-media-home-browse-experience.md) — accepted bounded implementation contract; runtime not started.\n- [Phase 65.D Playback Semantics Consolidation](development/phase-65d-playback-semantics-consolidation.md) — completed Phase-65 semantic contract/history.")
if "development/phase-65-closeout.md" not in text:
    raise SystemExit(f"{rel}: Phase65 closeout link missing after update")
write(rel, text)

replace_section("docs/planning/index.md", "Stable phase dependency chain", """## Stable phase dependency chain

```text
Phase 62 — Identity, RBAC and Accountability
  -> Phase 63 — Backend Agent and Secure Multi-Site Runtime
  -> Phase 64 — Timer Intent and Multi-Backend Orchestration
  -> Phase 65 — Streaming Gateway and Media Sessions
  -> Phase 66 — Media Home and Browse Experience
  -> Phase 67 — Broadcast Companion Services: Teletext and HbbTV
  -> Phase 68 — Legacy OSD Compatibility Bridge
  -> Phase 69 — Public API and Client Compatibility Hardening
  -> Phase 70 — Recommendation and Content Knowledge Graph
```

Current completed/active/next state belongs only in [Current State](../CURRENT.md).""")
replace_section("docs/planning/index.md", "Current Phase-65 planning anchor", """## Current Phase-66 planning anchor

Accepted planning for the next Media Home boundary is:

- [ADR-0058](../adr/ADR-0058-media-home-responsive-browse-preview.md) — responsive browse-first/deferred-preview architecture;
- [Phase 66 Media Home and Browse Experience](../development/phase-66-media-home-browse-experience.md) — bounded Slice 66.1-66.8 contract;
- [Golden User Journeys](golden-user-journeys.md) — desktop/mobile Home acceptance.

Phase 65 is completed. Accepted planning does not itself start Phase 66 runtime.""")
rel = "docs/planning/index.md"
text = read(rel)
if "[Phase 65 Closeout](../development/phase-65-closeout.md)" not in text:
    text = text.replace("- [Phase 64 Final Closeout](../development/phase-64-closeout.md)", "- [Phase 64 Final Closeout](../development/phase-64-closeout.md)\n- [Phase 65 Closeout](../development/phase-65-closeout.md)")
write(rel, text)

rel = "docs/development/index.md"
text = read(rel)
text = text.replace("- [Phase 65.D Playback Semantics Consolidation Contract](phase-65d-playback-semantics-consolidation.md)", "- [Phase 66 Media Home and Browse Experience](phase-66-media-home-browse-experience.md)\n- [ADR-0058 Media Home Architecture](../adr/ADR-0058-media-home-responsive-browse-preview.md)\n- [Phase 65 Closeout](phase-65-closeout.md)")
start, end = section_bounds(text, "Current media-domain work")
current_work = """## Current media/home work

Phase 65 - Streaming Gateway and Media Sessions is completed. Use [Phase 65 Closeout](phase-65-closeout.md) and Phase-65.D contracts for durable evidence/history.

Phase 66 - Media Home and Browse Experience is next but not started. Accepted [ADR-0058](../adr/ADR-0058-media-home-responsive-browse-preview.md) and the [Phase 66 implementation contract](phase-66-media-home-browse-experience.md) define bounded responsive Home work.

After a separate runtime kickoff, the first slice is 66.1 Home Shell and Responsive Information Architecture. It preserves completed Phase-65 playback ownership and existing Channel/EPG/Recording/Metadata ownership. Deferred preview, durable history, Teletext/HbbTV and later compatibility/API work do not enter Slice 66.1 by implication.

Growing-Recording seek and Live-TV timeshift remain truthful deferred capability work and do not reopen Phase 65."""
text = text[:start] + current_work + "\n\n" + text[end:].lstrip("\n")
write(rel, text)

# Root compact roadmap.
rel = "ROADMAP.md"
text = read(rel)
start, _ = section_bounds(text, "Current position")
end, _ = section_bounds(text, "Cross-cutting product milestones")
root_current = """## Current position

```text
Latest completed numbered runtime phase:
Phase 65 - Streaming Gateway and Media Sessions

Current active numbered runtime phase:
none - Phase 66 has not started

Next strict numbered runtime phase:
Phase 66 - Media Home and Browse Experience
```

Phase 65 is completed; see [Phase 65 Closeout](docs/development/phase-65-closeout.md). Accepted [ADR-0058](docs/adr/ADR-0058-media-home-responsive-browse-preview.md) and [Phase 66 Media Home and Browse Experience](docs/development/phase-66-media-home-browse-experience.md) define the next planning boundary. Runtime remains not started until a separate kickoff.

## Revised strict forward sequence

```text
Phase 64 - Timer Intent and Multi-Backend Orchestration [COMPLETED]
  -> Phase 65 - Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 - Media Home and Browse Experience [NEXT; NOT STARTED]
  -> Phase 67 - Broadcast Companion Services: Teletext and HbbTV
  -> Phase 68 - Legacy OSD Compatibility Bridge
  -> Phase 69 - Public API and Client Compatibility Hardening
  -> Phase 70 - Recommendation and Content Knowledge Graph
```

Completed history is unchanged; ADR-0058 changes only not-yet-started future numbering."""
text = text[:start] + root_current + "\n\n" + text[end:]
write(rel, text)

# Completed-phase entrypoints: update only current next-work markers.
for rel in ["docs/development/completed-phases.md", "docs/development/completed-phases-latest.md"]:
    text = read(rel)
    text = text.replace("Phase 66 - Broadcast Companion Services: Teletext and HbbTV", "Phase 66 - Media Home and Browse Experience")
    text = text.replace("Accepted ADR-0054 defines its architecture", "Accepted ADR-0058 defines its Media Home / Browse architecture")
    text = text.replace("Teletext/HbbTV runtime implementation", "Media Home runtime implementation")
    write(rel, text)

# Implementation dependency map: add Home projection and shift later phase owners.
rel = "docs/planning/implementation-dependency-map.md"
text = read(rel)
text = need(text, "  -> Streaming Gateway and Media Sessions\n       -> normalized playback semantics\n  -> Broadcast Companion Services: Teletext and HbbTV\n  -> Legacy OSD Compatibility Bridge\n  -> stable Public API and Client Compatibility Hardening\n  -> Recommendation and Content Knowledge Graph", "  -> Streaming Gateway and Media Sessions [completed]\n       -> normalized playback semantics [completed]\n  -> Media Home and Browse Experience\n  -> Broadcast Companion Services: Teletext and HbbTV\n  -> Legacy OSD Compatibility Bridge\n  -> stable Public API and Client Compatibility Hardening\n  -> Recommendation and Content Knowledge Graph", rel)
f = get_section(text, "Dependency F — Broadcast Companion Services")
g = get_section(text, "Dependency G — Legacy OSD Compatibility Bridge")
h = get_section(text, "Dependency H — Stable Public API and Client Hardening")
i = get_section(text, "Dependency I — Recommendation and Content Knowledge Graph")
home_dep = """## Dependency F — Media Home and Browse Experience

Phase 66 depends on completed Phase-65 media semantics plus stable Channel, ProgramEvent, Recording, Metadata, Genre and artwork identities.

```text
existing Suite read models
  -> responsive Home projection
  -> browse focus / rail selection
  -> optional deferred preview intent
  -> canonical Phase-65 MediaSession / playback owner when preview settles
```

Home does not own parallel content identity; browsing does not depend on preview startup; stale preview cannot attach after focus changes; Continue Watching consumes truthful resume evidence; durable history needs explicit actor/privacy/retention semantics."""
f = f.replace("## Dependency F — Broadcast Companion Services", "## Dependency G — Broadcast Companion Services", 1).replace("Phase 66 depends on A-B and on Phase-65 media semantics where HbbTV/application media uses Suite-owned resources.", "Phase 67 depends on A-B, completed Phase-65 media semantics where HbbTV/application media uses Suite-owned resources, and follows completed Phase 66 Home.").replace("Architecture is defined by accepted ADR-0054; runtime remains unauthorized until Phase 65 closes and Phase 66 is explicitly started.", "Architecture is defined by ADR-0054; runtime remains not started and requires an explicit Phase-67 kickoff after Phase 66 closes.")
g = g.replace("## Dependency G — Legacy OSD Compatibility Bridge", "## Dependency H — Legacy OSD Compatibility Bridge", 1).replace("Phase 67 depends on identity/authorization", "Phase 68 depends on identity/authorization")
h = h.replace("## Dependency H — Stable Public API and Client Hardening", "## Dependency I — Stable Public API and Client Hardening", 1).replace("Phase 68 depends on mature implemented resources", "Phase 69 depends on mature implemented resources")
i = i.replace("## Dependency I — Recommendation and Content Knowledge Graph", "## Dependency J — Recommendation and Content Knowledge Graph", 1).replace("Phase 69 requires a dedicated accepted ADR", "Phase 70 requires a dedicated accepted ADR")
start, _ = section_bounds(text, "Dependency F — Broadcast Companion Services")
_, end = section_bounds(text, "Dependency I — Recommendation and Content Knowledge Graph")
text = text[:start] + home_dep + "\n\n---\n\n" + f.strip() + "\n\n---\n\n" + g.strip() + "\n\n---\n\n" + h.strip() + "\n\n---\n\n" + i.strip() + "\n\n" + text[end:].lstrip("\n")
text = text.replace("Third-party public compatibility is formalized in Phase 68.", "Third-party public compatibility is formalized in Phase 69.")
write(rel, text)

# Domain dependency map: Home is a projection, not new domain authority.
rel = "docs/planning/domain-dependency-map.md"
text = read(rel)
text = text.replace("This document defines dependency direction between VDR-Suite domain models. Accepted architecture remains authoritative; ADR-0054 is represented as accepted architecture while runtime completion remains separately governed by the Strict Roadmap.", "This document defines dependency direction between VDR-Suite domain models. Accepted architecture remains authoritative. ADR-0058 Media Home is a projection over existing domains rather than a new source of truth; ADR-0054 Broadcast Companion remains accepted architecture for later Phase 67.")
marker = "# 10A. Broadcast Companion Domain — ADR-0054\n\nThis section reflects accepted architecture. Runtime remains planned for Phase 66 after Phase 65 and requires an explicit Phase-66 start."
projection = """# 10A. Media Home Projection — ADR-0058

Media Home is a first-party composition/projection, not a new domain source of truth.

```text
Channel + ProgramEvent + Recording + MetadataEntity + ArtworkAsset
  -> Home browse projection
  -> client focus / rail state
  -> optional preview intent
  -> existing MediaSession / playback owner when media is requested
```

Forbidden: content identity depending on Home card position, MediaSession authority depending on DOM focus/animation, or cross-client resume/history truth depending on browser-local storage by implication.

---

# 10B. Broadcast Companion Domain — ADR-0054

This section reflects accepted architecture. Runtime is planned for Phase 67 after Phase 66 Media Home and requires an explicit Phase-67 start."""
text = need(text, marker, projection, rel)
write(rel, text)

# Gap matrix: close Phase65 gaps, shift future owners, add Media Home gap.
rel = "docs/planning/architecture-audit-gap-matrix.md"
text = read(rel)
rows = {
"| G-19 | Streaming Gateway and authenticated MediaSession | Strong foundation | Phase 65.A-C implement accepted Recording/Live MediaSession/Gateway runtime, provider privacy/leases, least-transformation delivery, completed-Recording progressive paths, calibrated transcode/output policy and deterministic lifecycle cleanup. Phase-65.D client semantics now build above this stable server boundary rather than replacing it. | ADR-0046, ADR-0053, ADR-0055 / Phase 65 |": "| G-19 | Streaming Gateway and authenticated MediaSession | Closed foundation | Phase 65 is completed with Recording/Live MediaSession/Gateway runtime, provider privacy/leases, least-transformation delivery/output policy, deterministic cleanup and normalized persistent playback semantics. | ADR-0046, ADR-0053, ADR-0055 / Phase 65 |",
"| G-20 | Legacy OSD viewer/controller bridge | Planned | ADR-0047 is accepted; runtime is now sequenced after Broadcast Companion as Phase 67. RemoteAction/LiveOverlay is not the Legacy OSD plane. | ADR-0047 / Phase 67 |": "| G-20 | Legacy OSD viewer/controller bridge | Planned | ADR-0047 is accepted; runtime is sequenced after Broadcast Companion as Phase 68. RemoteAction/LiveOverlay is not the Legacy OSD plane. | ADR-0047 / Phase 68 |",
"| G-25 | Stable public API version/error/compatibility contract | Planned/partial | Internal Suite client contracts exist; stable independent-client API hardening remains Phase 68. | ADR-0048 / Phase 68 |": "| G-25 | Stable public API version/error/compatibility contract | Planned/partial | Internal Suite client contracts exist; stable independent-client API hardening remains Phase 69. | ADR-0048 / Phase 69 |",
"| G-34 | Client playback engine / media adaptation boundary | Strong foundation | ADR-0053 is implemented through browser Recording/Live playback, typed least-transformation selection, persistent playback ownership, completed-Recording seek/restart, normalized track selection, browser-local Volume/Mute, bounded continuous-fMP4 MSE forward buffering and sync-safe exact HLS resume. The remaining Phase-65.D architecture gap is semantic consolidation, not another player core. | ADR-0053, ADR-0055 / Phase 65.D |": "| G-34 | Client playback engine / media adaptation boundary | Closed foundation | Phase 65 completed browser Recording/Live playback, least-transformation selection, persistent ownership, seek/restart, normalized tracks, Volume/Mute, bounded fMP4 buffering and sync-safe exact HLS resume without another player core. | ADR-0053, ADR-0055 / Phase 65.D |",
"| G-38 | Teletext domain service | Planned | No canonical Teletext runtime exists yet. Accepted ADR-0054 models service/page/subpage data independently of OSD rendering. | ADR-0054 / Phase 66 |": "| G-38 | Teletext domain service | Planned | No canonical Teletext runtime exists yet. Accepted ADR-0054 models service/page/subpage data independently of OSD rendering. | ADR-0054 / Phase 67 |",
"| G-39 | HbbTV broadcast application domain/runtime | Planned | No canonical HbbTV runtime exists yet. Accepted ADR-0054 models application discovery/session/runtime without public raw plugin/browser commands. | ADR-0054 / Phase 66 |": "| G-39 | HbbTV broadcast application domain/runtime | Planned | No canonical HbbTV runtime exists yet. Accepted ADR-0054 models application discovery/session/runtime without public raw plugin/browser commands. | ADR-0054 / Phase 67 |",
"| G-41 | Recommendation/content graph | Deferred vision | Requires stable identities, privacy/preferences, provenance and Phase-68 public resource semantics plus a dedicated ADR. | future ADR / Phase 69 |": "| G-41 | Recommendation/content graph | Deferred vision | Requires stable identities, privacy/preferences, provenance and Phase-69 public resource semantics plus a dedicated ADR. | future ADR / Phase 70 |",
"| G-42 | Normalized playback presentation/timeline/continuity/failure semantics | Planned on strong implementation foundation | ADR-0056 is accepted after real Phase-65.D transport/timeline/resume evidence. Remaining implementation is a provider-free `MediaPlaybackContract`, canonical owner lifecycle publication, explicit presentation generation/discontinuity and classified failures. Diagnostics follow semantic correctness and are not authority. | ADR-0056 / Phase 65.D |": "| G-42 | Normalized playback presentation/timeline/continuity/failure semantics | Closed foundation | ADR-0056 mandatory semantics are completed: provider-free `MediaPlaybackContract`, canonical owner lifecycle publication, explicit presentation generation/discontinuity and classified failures. | ADR-0056 / Phase 65.D |",
}
for old, new in rows.items():
    text = need(text, old, new, rel)
anchor = rows[list(rows.keys())[-1]] + "\n"
text = need(text, anchor, anchor + "| G-43 | Responsive Media Home / browse-first preview composition | Planned; architecture accepted | ADR-0058 and the Phase-66 contract define responsive Home composition, Live hero browsing, deferred preview, truthful Continue Watching, discovery rails and desktop/mobile Golden Journeys. Runtime has not started. | ADR-0058 / Phase 66 |\n", rel)
start, _ = section_bounds(text, "Priority view")
end, _ = section_bounds(text, "Maintenance rules")
priority = """## Priority view

### Next numbered runtime product domain — Phase 66

Media Home / Browse architecture is accepted via ADR-0058; runtime remains not started and requires a separate explicit kickoff. Slice 66.1 is Home Shell and Responsive Information Architecture. Later slices add Live hero browsing, deferred canonical preview, truthful Continue Watching, Recording discovery rails, explicit history semantics if needed, accessibility/polish and real desktop/mobile acceptance.

Phase 66 preserves completed Phase-65 MediaSession/playback ownership and existing Channel/ProgramEvent/Recording/Metadata/Genre/artwork truth. Browse focus remains independent of preview state; stale preview must be canceled/relinquished; browser-local state is not fabricated into cross-client authority.

### Following television product domain — Phase 67

Teletext/HbbTV architecture remains accepted via ADR-0054 and follows Phase 66. Runtime is not started.

### Later compatibility/platform work

Legacy OSD (Phase 68), public API hardening (Phase 69), storage federation and Recommendation/Content Graph (Phase 70) remain separate.

### Cross-cutting product work

Account/backend access administration, broad Timer UI, audit/operations and client-family rollout may progress when their own prerequisites are met without advancing the numbered phase."""
text = text[:start] + priority + "\n\n" + text[end:]
write(rel, text)

# Remove stale/temporary one-shot machinery from final tree.
for rel in [".github/workflows/phase65-closeout-sync.yml", ".github/workflows/phase66-planning-sync.yml", "tools/phase66_planning_sync_once.py"]:
    if p(rel).exists():
        p(rel).unlink()

# Strong post-edit markers before normal repository validation.
checks = {
    "docs/CURRENT.md": ["Phase 65 - Streaming Gateway and Media Sessions", "none - Phase 66 has not started", "Phase 66 - Media Home and Browse Experience", "Phase 65 Closeout", "ADR-0058"],
    "docs/NEW-CHAT-HANDOFF.md": ["Phase 66 - Media Home and Browse Experience", "Phase 66 has not started", "do not start Phase 66.", "development/phase-65-closeout.md"],
    "docs/development/current-status.md": ["Phase 66 - Media Home and Browse Experience", "Phase 66 has not started", "Phase 65 Closeout"],
    "docs/planning/roadmap.md": ["## Phase 65 — Streaming Gateway and Media Sessions\n\nStatus: **Completed.**", "## Phase 66 — Media Home and Browse Experience\n\nStatus: **Next; not started.**", "## Phase 67 — Broadcast Companion Services: Teletext and HbbTV", "## Phase 68 — Legacy OSD Compatibility Bridge", "## Phase 69 — Public API and Client Compatibility Hardening", "## Phase 70 — Recommendation and Content Knowledge Graph", "../development/phase-65-closeout.md"],
    "docs/planning/phase-map.md": ["Phase 66 | Next; not started | Media Home and Browse Experience", "Phase 67 | Planned after Phase 66 | Broadcast Companion Services: Teletext and HbbTV", "../development/phase-65-closeout.md"],
    "docs/adr/ADR-0058-media-home-responsive-browse-preview.md": ["**Accepted**"],
    "docs/development/phase-66-media-home-browse-experience.md": ["Status: **Accepted implementation contract under ADR-0058. Runtime not started.**"],
    "docs/planning/golden-user-journeys.md": ["Journey 6 — Browse Media Home on desktop", "Journey 7 — Browse Media Home on a phone"],
}
for rel, markers in checks.items():
    assert_contains(rel, *markers)

for rel, marker in {
    "docs/planning/roadmap.md": "## Phase 66 — Broadcast Companion Services: Teletext and HbbTV",
    "docs/planning/phase-map.md": "| 3 | Phase 66 | Planned after Phase 65 | Broadcast Companion Services",
}.items():
    if marker in read(rel):
        raise SystemExit(f"{rel}: stale current planning marker remains: {marker}")

print("PHASE66_PLANNING_SYNC=PASS")
