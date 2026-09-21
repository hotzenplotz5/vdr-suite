#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

errors = []

def read(path: str) -> str:
    target = ROOT / path
    if not target.exists():
        errors.append(f"missing Phase 68.B file: {path}")
        return ""
    return target.read_text(encoding="utf-8")

def require(condition: bool, message: str) -> None:
    if not condition:
        errors.append(message)

caps_h = read("vdr-plugin-suite-bridge/suitebridge_capabilities.h")
caps_cpp = read("vdr-plugin-suite-bridge/suitebridge_capabilities.cpp")
plugin = read("vdr-plugin-suite-bridge/suitebridge.cpp")
svdrp = read("vdr-plugin-suite-bridge/suitebridge_svdrp.cpp")
payload_h = read("vdr-plugin-suite-bridge/suitebridge_osd_snapshot_contract.h")
payload_cpp = read("vdr-plugin-suite-bridge/suitebridge_osd_snapshot_contract.cpp")
transport_h = read("core/agent/include/ISuiteBridgeLocalTransport.h")
transport_cpp = read("core/agent/src/SuiteBridgeSvdrpTransport.cpp")
parser_h = read("core/agent/include/SuiteBridgeOsdFrameParser.h")
parser_cpp = read("core/agent/src/SuiteBridgeOsdFrameParser.cpp")
buffer_h = read("core/agent/include/SuiteBridgeOsdFrameBuffer.h")
buffer_cpp = read("core/agent/src/SuiteBridgeOsdFrameBuffer.cpp")
source_h = read("core/agent/include/SuiteBridgeOsdFrameSource.h")
source_cpp = read("core/agent/src/SuiteBridgeOsdFrameSource.cpp")
agent_sources = read("mk/agent-sources.mk")
phase_make = read("mk/phase68-legacy-osd-tests.mk")

require(
    "std::array<SuiteBridgeCapabilityDescriptor, 12>" in caps_h,
    "Phase 68.B capability catalog must remain explicitly bounded",
)
require(
    '{"osd.view", SuiteBridgeCapabilityState::Available}' in caps_cpp,
    "osd.view must be advertised only after the private full-frame source exists",
)
require(
    '{"osd.control", SuiteBridgeCapabilityState::Available}' in caps_cpp,
    "osd.control must be advertised after the Phase 68.G typed input path exists",
)
require(
    "representation=semantic public-endpoint=none input=allowlisted" in plugin,
    "SuiteBridge must preserve semantic/no-public OSD while advertising allowlisted input",
)

for fragment in (
    '"OSDSNAP\\n"',
    'strcasecmp(Command, "OSDSNAP")',
    "statusMonitor_.CaptureOsdSnapshot()",
    "SuiteBridgeOsdSnapshotPayload payload(snapshot)",
    "SuiteBridgeOsdSnapshotPayload::SchemaVersion()",
):
    require(fragment in svdrp, f"missing private OSDSNAP wiring fragment: {fragment}")

require(
    "MaximumPayloadBytes = 131000" in payload_h,
    "plugin OSD payload must remain below the bounded SVDRP reply ceiling",
)
require(
    'AppendJsonString(snapshot.title.data())' in payload_cpp,
    "semantic OSD title must pass through bounded JSON serialization",
)
require(
    "SuiteBridgeLocalCommand::OsdSnapshot" in transport_cpp and
    '"PLUG suitebridge OSDSNAP\\r\\n"' in transport_cpp,
    "Agent transport must use the typed private OSDSNAP command",
)
require(
    "OsdSnapshot" in transport_h,
    "typed local transport must expose an OSD snapshot command",
)

for fragment in (
    "MaximumPayloadBytes = 131000",
    'frame.surface.surfaceId = "primary-native-osd"',
    "frame.fullFrame = true",
    "OsdSurfaceState::Degraded",
    "validEpoch",
    "osdFrameWithinLimits",
):
    require(fragment in parser_h + parser_cpp,
            f"missing bounded OSD parser contract: {fragment}")

for fragment in (
    "resyncRequired",
    "next.frameSequence < previous.frameSequence",
    "next.frameSequence == previous.frameSequence",
    "observed.droppedUpdates >",
    "previous.surface.osdEpoch != next.surface.osdEpoch",
    "ReplacedBackendGeneration",
):
    require(fragment in buffer_h + buffer_cpp,
            f"missing Agent-local OSD resync contract: {fragment}")

for fragment in (
    'capabilityAvailable("osd.view")',
    "SuiteBridgeLocalCommand::OsdSnapshot",
    "SuiteBridgeOsdFrameSourceState::ResyncRequired",
):
    require(fragment in source_h + source_cpp,
            f"missing capability-gated Agent-local OSD source contract: {fragment}")

require(
    "AGENT_OSD_OBSERVATION_SRC" in agent_sources,
    "Agent source inventory must own the Phase 68.B OSD pipeline",
)
require(
    "test-phase68-osd-agent-local-resync" in phase_make and
    "test_suite_bridge_osd_frame_pipeline.cpp" in phase_make,
    "Phase 68.B pipeline must be wired into normal CI",
)

combined = "\n".join(
    (
        payload_h,
        payload_cpp,
        parser_h,
        parser_cpp,
        buffer_h,
        buffer_cpp,
        source_h,
        source_cpp,
    )
)

for forbidden in (
    "cRemote",
    "Put(",
    "SVDRPCommand",
    "restfulapi",
    "RestApi",
    "HttpServer",
    "WebSocket",
    "skinDesigner",
    "SkinDesigner",
):
    require(
        forbidden not in combined,
        f"Phase 68.B crossed forbidden view-only/public/input boundary: {forbidden}",
    )

if errors:
    for error in errors:
        print(f"ERROR={error}")
    raise SystemExit(1)

print("RESULT=PHASE68B_OSD_AGENT_LOCAL_RESYNC_CONTRACT_PASS")
