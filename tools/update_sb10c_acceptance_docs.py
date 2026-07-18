#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

RUNTIME_HEAD = "7362ecec0d103e1e4659b80476ea5ad321d413e2"
AUTOMATED_HEAD = "10e82701f5633681b96df13d39ee0c05783ff68c"
SYNC_HEAD = "e1e0e5dea1486122e6edc2837b9702723a10e3d8"
MAIN_HEAD = "d2e6f1745cdba3592ac49f2d7dba33626136fbbe"
LIVE_EPOCH = "9587ed0c461a89827c75a26fc56d11c6"
PLUGIN_SHA256 = "a84c4571e951da94de2c0b5f9badf2c74034fe94b0c43483dfa9d9345d513b5d"

ARCHITECTURE = ROOT / "docs/architecture/suite-bridge-observation-lifecycle.md"
ROADMAP = ROOT / "vdr-plugin-suite-bridge/docs/ROADMAP.md"
HANDOFF = ROOT / "vdr-plugin-suite-bridge/docs/VDR-SUITE-HANDOFF.md"

texts = {
    ARCHITECTURE: ARCHITECTURE.read_text(encoding="utf-8"),
    ROADMAP: ROADMAP.read_text(encoding="utf-8"),
    HANDOFF: HANDOFF.read_text(encoding="utf-8"),
}


def replace_once(path: Path, old: str, new: str) -> None:
    count = texts[path].count(old)
    if count != 1:
        raise RuntimeError(
            f"{path}: expected one occurrence, found {count}: {old!r}"
        )
    texts[path] = texts[path].replace(old, new, 1)


def replace_between(path: Path, start: str, end: str, replacement: str) -> None:
    text = texts[path]
    if text.count(start) != 1:
        raise RuntimeError(f"{path}: start marker is not unique: {start!r}")
    if text.count(end) != 1:
        raise RuntimeError(f"{path}: end marker is not unique: {end!r}")
    start_index = text.index(start)
    end_index = text.index(end, start_index)
    texts[path] = text[:start_index] + replacement + text[end_index:]


replace_once(
    ARCHITECTURE,
    """SB.10c implementation contract.\n\nThe slice remains `active` until automated acceptance and controlled live VDR\nacceptance have passed. This document does not mark SB.10c completed.\n""",
    f"""SB.10c completed.\n\nFocused automated acceptance passed at:\n\n```text\n{AUTOMATED_HEAD}\n```\n\nControlled live VDR acceptance and complete rollback passed at:\n\n```text\n{RUNTIME_HEAD}\n```\n\nThe plugin runtime contract remained unchanged.\n""",
)

replace_once(
    ARCHITECTURE,
    """Controlled live acceptance remains required before SB.10c can be marked\ncompleted.\n""",
    f"""## Acceptance Result\n\nStatus: `completed`\n\nAutomated acceptance proved:\n\n- strict Make inventory closure;\n- retained SB.10a and SB.10b regressions;\n- deterministic polling, reconnect, freshness and delta behavior;\n- exact stale and offline thresholds;\n- epoch replacement, overflow suppression and counter-regression rejection;\n- hard mutation disablement;\n- interruptible worker shutdown with no surviving thread;\n- complete documentation and architecture checks;\n- a clean synchronized worktree.\n\nControlled live acceptance on VDR `2.7.9`, API version `11`, proved:\n\n- plugin version `0.10.0` loaded as `libvdr-suitebridge.so.11`;\n- installed object SHA-256 `{PLUGIN_SHA256}`;\n- retained handshake result `status=ready`;\n- initial observation command sequence `CAPS,SNAP`;\n- trusted follow-up polling through one additional `SNAP`;\n- observation state `snapshot_current`;\n- live epoch `{LIVE_EPOCH}`;\n- live total `4`;\n- `counter_overflow=false`;\n- clean worker stop;\n- unchanged channel, Timer, Recording and `setup.conf` state;\n- complete plugin removal, VDR restart and rollback;\n- no Suite Bridge object remained mapped;\n- a clean synchronized repository.\n\nNo plugin source, command, capability, schema or mutation state changed.\n""",
)

replace_once(
    ROADMAP,
    "- [SB.10b Local SVDRP Transport](../../docs/architecture/suite-bridge-svdrp-transport.md)\n",
    "- [SB.10b Local SVDRP Transport](../../docs/architecture/suite-bridge-svdrp-transport.md)\n- [SB.10c Observation Lifecycle](../../docs/architecture/suite-bridge-observation-lifecycle.md)\n",
)

replace_once(
    ROADMAP,
    "| SB.10b Local typed SVDRP transport | `3396840d41260bb3ed81bc652921b329263d7e58` |\n",
    f"| SB.10b Local typed SVDRP transport | `3396840d41260bb3ed81bc652921b329263d7e58` |\n| SB.10c Polling, reconnect and freshness | `{RUNTIME_HEAD}` |\n",
)

replace_once(
    ROADMAP,
    """SB.10a and SB.10b are Agent-side work. They do not change the plugin version,\nplugin commands, capability catalogue or local schemas.\n""",
    f"""SB.10c focused automated-acceptance head:\n\n```text\n{AUTOMATED_HEAD}\n```\n\nSB.10c controlled live-acceptance head:\n\n```text\n{RUNTIME_HEAD}\n```\n\nSB.10a, SB.10b and SB.10c are Agent-side work. They do not change the plugin\nversion, plugin commands, capability catalogue or local schemas.\n""",
)

replace_once(
    ROADMAP,
    "| SB.10c Polling, reconnect and freshness | active | Backend Agent | no |\n| SB.10d Embedded-Agent runtime integration | planned | Backend Agent / Suite runtime | no, unless a proven compatibility gap exists |\n",
    "| SB.10c Polling, reconnect and freshness | completed | Backend Agent | no |\n| SB.10d Embedded-Agent runtime integration | active | Backend Agent / Suite runtime | no, unless a proven compatibility gap exists |\n",
)

replace_between(
    ROADMAP,
    "## SB.10c — Read-Only Polling, Reconnect and Freshness\n",
    "## SB.10d — Embedded-Agent Runtime Integration\n",
    f"""## SB.10c — Read-Only Polling, Reconnect and Freshness\n\nStatus: `completed`\n\nPrimary owner: Backend Agent.\n\nFocused automated-acceptance head:\n\n```text\n{AUTOMATED_HEAD}\n```\n\nControlled live-acceptance head:\n\n```text\n{RUNTIME_HEAD}\n```\n\nIncluded synchronized Suite `main` head:\n\n```text\n{MAIN_HEAD}\n```\n\nImplemented behavior:\n\n- initial `CAPS 1` before `SNAP`;\n- trusted later `SNAP` polling without repeated discovery;\n- explicit compatibility, freshness, degraded, overflow and offline states;\n- one bounded last-good baseline;\n- monotonic same-epoch delta calculation;\n- epoch replacement without a transition delta;\n- overflow delta suppression;\n- same-epoch counter-regression rejection;\n- five-second polling;\n- 15-second stale and 60-second offline thresholds;\n- reconnect delays of `1, 2, 4, 8, 16, 30` seconds;\n- bounded diagnostics;\n- a deterministic thread-free service;\n- one thin joinable worker with interruptible waits;\n- idempotent start and stop;\n- hard mutation disablement.\n\nControlled live acceptance proved command sequence `CAPS,SNAP,SNAP`, state\n`snapshot_current`, live epoch `{LIVE_EPOCH}`, total `4`,\n`counter_overflow=false`, clean worker shutdown, unchanged VDR state and complete\nrollback.\n\nPlugin changes: none.\n\nPlugin version, commands, capabilities, schemas and mutation state remain\nunchanged.\n\n""",
)

replace_once(
    ROADMAP,
    "## SB.10d — Embedded-Agent Runtime Integration\n\nStatus: `planned`\n",
    "## SB.10d — Embedded-Agent Runtime Integration\n\nStatus: `active`\n",
)

replace_between(
    ROADMAP,
    "## Immediate Next Work\n",
    "## Non-Goals\n",
    f"""## Immediate Next Work\n\nThe next implementation slice is:\n\n```text\nSB.10d - Embedded-Agent runtime integration\n```\n\nBefore implementation, synchronize the bridge branch with current `main`, then\nreview `RuntimeConfig`, `BackendRuntimeContext`, `DaemonRuntime`, lifecycle,\nshutdown, health publication and RESTfulAPI coexistence.\n\nSB.10c is completed at `{RUNTIME_HEAD}`.\n\nExpected plugin changes: none.\n\n---\n\n""",
)

replace_once(
    ROADMAP,
    "- mark all of SB.10 completed before SB.10c and SB.10d acceptance;\n",
    "- mark all of SB.10 completed before SB.10d acceptance;\n",
)

replace_once(
    ROADMAP,
    "- [SB.10b Local SVDRP Transport](../../docs/architecture/suite-bridge-svdrp-transport.md)\n- [VDR-Suite ADR-0039: Backend Agent and Control Plane Boundary]",
    "- [SB.10b Local SVDRP Transport](../../docs/architecture/suite-bridge-svdrp-transport.md)\n- [SB.10c Observation Lifecycle](../../docs/architecture/suite-bridge-observation-lifecycle.md)\n- [VDR-Suite ADR-0039: Backend Agent and Control Plane Boundary]",
)

replace_once(
    HANDOFF,
    "- [SB.10b Local SVDRP Transport](../../docs/architecture/suite-bridge-svdrp-transport.md)\n",
    "- [SB.10b Local SVDRP Transport](../../docs/architecture/suite-bridge-svdrp-transport.md)\n- [SB.10c Observation Lifecycle](../../docs/architecture/suite-bridge-observation-lifecycle.md)\n",
)

replace_between(
    HANDOFF,
    "## Current Coordinated Snapshot\n",
    "The accepted plugin ADR and roadmap are coordination artifacts.",
    f"""## Current Coordinated Snapshot\n\n| Item | Current value |\n| --- | --- |\n| Last completed plugin runtime slice | `SB.9 - Read-only capability discovery and compatibility negotiation` |\n| Last completed Agent contract slice | `SB.10a - Transport-neutral local handshake contract` |\n| Last completed Agent transport slice | `SB.10b - Local typed SVDRP transport` |\n| Last completed Agent observation slice | `SB.10c - Read-only polling, reconnect and freshness` |\n| Next active coordinated slice | `SB.10d - Embedded-Agent runtime integration` |\n| Plugin name | `suitebridge` |\n| Plugin version | `0.10.0` |\n| Shared object | `libvdr-suitebridge.so.<VDR-APIVERSION>` |\n| SB.10a automated-acceptance head | `ba6deddbfba6d50b1152d584654a92f75340dcc3` |\n| SB.10b automated and live-acceptance head | `3396840d41260bb3ed81bc652921b329263d7e58` |\n| SB.10c synchronized implementation baseline | `{SYNC_HEAD}` |\n| SB.10c focused automated-acceptance head | `{AUTOMATED_HEAD}` |\n| SB.10c controlled live-acceptance head | `{RUNTIME_HEAD}` |\n| Suite `main` included before SB.10c | `{MAIN_HEAD}` |\n| Live VDR version | `2.7.9` |\n| Live VDR API version | `11` |\n| SB.10b and SB.10c shared-object SHA-256 | `{PLUGIN_SHA256}` |\n| SB.10c observed live epoch | `{LIVE_EPOCH}` |\n| Mutation state | `disabled` |\n\nSB.10a, SB.10b and SB.10c are Agent-side slices. None changes the plugin\nversion, commands, capability catalogue, schemas or mutation state.\n\n""",
)

replace_once(
    HANDOFF,
    """- a missing plugin reply `550` enables no fallback and is classified as\n  `LegacyOrUnknown`;\n""",
    """- a missing plugin reply `550` enables no fallback;\n- the retained one-shot handshake reports `LegacyOrUnknown`;\n- the SB.10c observation lifecycle exposes the precise `plugin_missing` state;\n""",
)

replace_once(
    HANDOFF,
    "---\n\n## Architectural Position\n",
    f"""---\n\n## SB.10c Read-Only Observation Lifecycle Acceptance\n\nStatus: `completed`\n\n| Item | Value |\n| --- | --- |\n| Synchronized baseline | `{SYNC_HEAD}` |\n| Focused automated-acceptance head | `{AUTOMATED_HEAD}` |\n| Controlled live-acceptance head | `{RUNTIME_HEAD}` |\n| Included Suite `main` head | `{MAIN_HEAD}` |\n| VDR / API version | `2.7.9` / `11` |\n| Installed shared object | `libvdr-suitebridge.so.11` |\n| Installed object SHA-256 | `{PLUGIN_SHA256}` |\n| Observed epoch | `{LIVE_EPOCH}` |\n| Observed total / overflow | `4` / `false` |\n\nImplemented and accepted behavior:\n\n- initial `CAPS 1`, then `SNAP`;\n- trusted subsequent `SNAP` polling;\n- bounded reconnect and explicit freshness;\n- retained last-good baseline;\n- safe same-epoch deltas;\n- epoch replacement and overflow suppression;\n- same-epoch counter-regression rejection;\n- bounded diagnostics;\n- deterministic service and clean joinable worker shutdown;\n- no daemon, RESTfulAPI, SQLite or plugin-source coupling;\n- mutations always disabled.\n\nLive acceptance proved `status=ready`, sequence `CAPS,SNAP,SNAP`, state\n`snapshot_current`, stable epoch, monotonic counters, clean worker stop, unchanged\nchannel, Timer, Recording and `setup.conf` state, complete plugin removal, VDR\nrestart and rollback.\n\nNo plugin source, command, capability or schema changed.\n\n---\n\n## Architectural Position\n""",
)

replace_once(
    HANDOFF,
    "| Polling, reconnect and freshness lifecycle | consumes health | owner | unchanged endpoint | next active slice |\n| Embedded Agent runtime integration | consumes health | owner with Suite runtime | unchanged endpoint | planned after SB.10c |\n",
    f"| Polling, reconnect and freshness lifecycle | consumes health | owner | unchanged endpoint | completed at `{RUNTIME_HEAD}` |\n| Embedded Agent runtime integration | consumes health | owner with Suite runtime | unchanged endpoint | next active slice |\n",
)

replace_once(
    HANDOFF,
    """SB.10a and SB.10b consume the accepted plugin runtime contract but do not create\na later plugin runtime slice.\n""",
    """SB.10a, SB.10b and SB.10c consume the accepted plugin runtime contract but do\nnot create a later plugin runtime slice.\n""",
)

replace_between(
    HANDOFF,
    "## Next Safe Coordinated Slice\n",
    "## Planned Plugin Direction After SB.10\n",
    f"""## Next Safe Coordinated Slice\n\n`SB.10d - Embedded-Agent runtime integration`\n\nStatus: `active`\n\nPrimary ownership: **Backend Agent and Suite runtime**.\n\nExpected plugin changes: **none**, unless a specific bounded compatibility gap is\nproven.\n\nCompleted prerequisites:\n\n- SB.10a: `ba6deddbfba6d50b1152d584654a92f75340dcc3`;\n- SB.10b: `3396840d41260bb3ed81bc652921b329263d7e58`;\n- SB.10c automated: `{AUTOMATED_HEAD}`;\n- SB.10c live: `{RUNTIME_HEAD}`.\n\nBefore SB.10d code, compare current `main`, review runtime configuration,\nconstruction, start, stop, destruction and health publication, and preserve\nRESTfulAPI as the broad VDR domain-read adapter. Mutations remain disabled.\n\n---\n\n""",
)

replace_between(
    HANDOFF,
    "## Immediate Coordination Notes\n",
    "## Gold-Standard Definition of Done\n",
    f"""## Immediate Coordination Notes\n\n- Plugin slices SB.1 through SB.9 remain live accepted.\n- SB.10a, SB.10b and SB.10c are completed Agent-side slices.\n- SB.10c automated acceptance head is `{AUTOMATED_HEAD}`.\n- SB.10c live acceptance head is `{RUNTIME_HEAD}`.\n- The accepted live sequence was `CAPS,SNAP,SNAP`.\n- The accepted state was `snapshot_current`.\n- Channel, Timer, Recording and `setup.conf` state remained unchanged.\n- Complete rollback removed the plugin, configuration and process mapping.\n- Plugin version remains `0.10.0`.\n- `CAPS` and `SNAP` remain the only plugin commands.\n- Discovery and capability schemas remain `1`; snapshot and local-contract schemas\n  remain `2`.\n- `mutations` remains `disabled`.\n- The next active work is SB.10d.\n\n---\n\n## Gold-Standard Definition of Done\n""",
)

replace_once(
    HANDOFF,
    "- [Back to SB.10b Local SVDRP Transport](../../docs/architecture/suite-bridge-svdrp-transport.md)\n",
    "- [Back to SB.10c Observation Lifecycle](../../docs/architecture/suite-bridge-observation-lifecycle.md)\n",
)

for path, text in texts.items():
    if not text.endswith("\n"):
        raise RuntimeError(f"{path}: missing final newline")
    path.write_text(text, encoding="utf-8")

print("SB.10c acceptance documentation updated")
for path in texts:
    print(path.relative_to(ROOT))
