#!/usr/bin/env python3
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def read(p): return (ROOT/p).read_text(encoding="utf-8")
protocol=read("core/agent/include/SuiteBridgeControlPlaneProtocol.h")
client_h=read("core/agent/include/SuiteBridgeLocalControlTransport.h")
server=read("vdr-plugin-suite-bridge/suitebridge_control_plane.cpp")
plugin=read("vdr-plugin-suite-bridge/suitebridge.cpp")
runtime=read("core/daemon/src/RecordingMediaHttpRuntime.cpp")
agent_main=read("apps/agent/main.cpp")
daemon_context=read("core/daemon/include/BackendRuntimeContext.h")
daemon_backend=read("core/daemon/src/DaemonRuntimeBackendContext.cpp")
tmpfiles=read("packaging/systemd/vdr-suite-live.conf")
for marker in ("ProtocolMajor = 1","MaximumRequestPayloadBytes","MaximumResponsePayloadBytes","LiveCapability = 1","LiveOpen = 2","LiveStatus = 3","LiveClose = 4","NativeProbeCapability = 5","NativeProbeExecute = 6","NativeProbeReadback = 7","CapabilityDiscovery = 8","OsdSnapshot = 9","OsdInput = 10","HbbtvDiscovery = 11","HbbtvRuntime = 12","HbbtvPresentation = 13","HbbtvMedia = 14","CriticalControl = 1","InteractiveControlRead = 2","ExternalPluginInteractive = 3","DeadlineExpired","Overloaded"):
    if marker not in protocol: raise SystemExit(f"control-plane protocol missing {marker}")
for marker in ("AF_UNIX","SOCK_SEQPACKET","SO_PEERCRED","queues_[lane].size() < queueCapacity_[lane]","ServiceClass::CriticalControl","ServiceClass::InteractiveControlRead","ServiceClass::ExternalPluginInteractive","deadline_expired_at_admission","deadline_expired_before_execution","queue_full"):
    if marker not in server: raise SystemExit(f"control-plane server missing {marker}")
for forbidden in ("AF_INET","SOCK_STREAM","runSvdrp","callPlugin","executeNative"):
    if forbidden in server: raise SystemExit(f"control-plane server contains forbidden marker {forbidden}")
if "SuiteBridgeTransportStatus::Unavailable" not in client_h: raise SystemExit("compatibility fallback must be pre-dispatch Unavailable-only")
if "SuiteBridgeLocalControlTransport" not in runtime or "SuiteBridgePrioritizedLiveTransport" not in runtime: raise SystemExit("Live runtime not wired to dedicated transport")
if "SuiteBridgeSvdrpTransport" not in runtime: raise SystemExit("SVDRP compatibility transport unexpectedly removed")
if "controlPlane_.Start" not in plugin or "controlPlane_.Stop" not in plugin: raise SystemExit("plugin lifecycle wiring missing")
for marker in ("Operation::NativeProbeCapability","Operation::NativeProbeExecute","Operation::NativeProbeReadback","nativeProbe_.Handle"):
    if marker not in plugin: raise SystemExit(f"native probe plugin control-plane wiring missing {marker}")
for marker in ("SuiteBridgePrioritizedNativeProbeTransport","SuiteBridgeLocalControlTransport","setBackendAgentNativeProbeTransport(nativeTransport.get())"):
    if marker not in agent_main: raise SystemExit(f"native probe Agent control-plane wiring missing {marker}")
for marker in ("SuiteBridgePrioritizedLocalTransport","SuiteBridgePrioritizedLegacyOsdInputTransport","osdReadTransport","osdInputTransport"):
    if marker not in agent_main: raise SystemExit(f"Legacy OSD local-control wiring missing {marker}")
for marker in ("Operation::CapabilityDiscovery","Operation::OsdSnapshot","Operation::OsdInput","SuiteBridgeOsdSnapshotPayload","osdInput_.Handle"):
    if marker not in plugin: raise SystemExit(f"Legacy OSD plugin local-control wiring missing {marker}")
for marker in ("Operation::HbbtvDiscovery","Operation::HbbtvRuntime","Operation::HbbtvPresentation","Operation::HbbtvMedia","hbbtvCommand_.Handle"):
    if marker not in plugin: raise SystemExit(f"HbbTV plugin local-control wiring missing {marker}")
for marker in ("SuiteBridgePrioritizedHbbtvTransport","SuiteBridgeHbbtvTransportStatus::Unavailable"):
    if marker not in client_h: raise SystemExit(f"HbbTV prioritized local-control transport missing {marker}")
for marker in ("hbbtvLocalControlTransport","hbbtvTransport","effectiveHbbtvTransport"):
    if marker not in daemon_context: raise SystemExit(f"HbbTV daemon context wiring missing {marker}")
for marker in ("SuiteBridgeLocalControlTransportConfig","SuiteBridgePrioritizedHbbtvTransport","hbbtvLocalControlTransport"):
    if marker not in daemon_backend: raise SystemExit(f"HbbTV daemon local-control construction missing {marker}")
if "MaximumResponsePayloadBytes = 131072" not in protocol:
    raise SystemExit("control-plane response bound must cover bounded OSD snapshot")
if "d /run/vdr/vdr-suite-control 0700 vdr vdr -" not in tmpfiles: raise SystemExit("private runtime directory not packaged")
print("SuiteBridge control-plane architecture checks passed")
