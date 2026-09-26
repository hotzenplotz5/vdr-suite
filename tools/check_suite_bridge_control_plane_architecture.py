#!/usr/bin/env python3
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def read(p): return (ROOT/p).read_text(encoding="utf-8")
protocol=read("core/agent/include/SuiteBridgeControlPlaneProtocol.h")
client_h=read("core/agent/include/SuiteBridgeLocalControlTransport.h")
server=read("vdr-plugin-suite-bridge/suitebridge_control_plane.cpp")
plugin=read("vdr-plugin-suite-bridge/suitebridge.cpp")
runtime=read("core/daemon/src/RecordingMediaHttpRuntime.cpp")
tmpfiles=read("packaging/systemd/vdr-suite-live.conf")
for marker in ("ProtocolMajor=1","MaximumRequestPayloadBytes","MaximumResponsePayloadBytes","LiveCapability=1","LiveOpen=2","LiveStatus=3","LiveClose=4","CriticalControl=1","DeadlineExpired","Overloaded"):
    if marker not in protocol: raise SystemExit(f"control-plane protocol missing {marker}")
for marker in ("AF_UNIX","SOCK_SEQPACKET","SO_PEERCRED","queue_.size()<queueCapacity_","deadline_expired_at_admission","deadline_expired_before_execution","queue_full"):
    if marker not in server: raise SystemExit(f"control-plane server missing {marker}")
for forbidden in ("AF_INET","SOCK_STREAM","runSvdrp","callPlugin","executeNative"):
    if forbidden in server: raise SystemExit(f"control-plane server contains forbidden marker {forbidden}")
if "transportStatus!=SuiteBridgeTransportStatus::Unavailable" not in client_h: raise SystemExit("compatibility fallback must be pre-dispatch Unavailable-only")
if "SuiteBridgeLocalControlTransport" not in runtime or "SuiteBridgePrioritizedLiveTransport" not in runtime: raise SystemExit("Live runtime not wired to dedicated transport")
if "SuiteBridgeSvdrpTransport" not in runtime: raise SystemExit("SVDRP compatibility transport unexpectedly removed")
if "controlPlane_.Start" not in plugin or "controlPlane_.Stop" not in plugin: raise SystemExit("plugin lifecycle wiring missing")
if "d /run/vdr/vdr-suite-control 0700 vdr vdr -" not in tmpfiles: raise SystemExit("private runtime directory not packaged")
print("SuiteBridge control-plane architecture checks passed")
