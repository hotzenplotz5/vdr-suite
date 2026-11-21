#!/usr/bin/env python3
"""Read-only source contract for VDR-native video-root discovery wiring."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "vdr-plugin-suite-bridge/suitebridge_svdrp.cpp").read_text()
transport = (root / "core/agent/src/SuiteBridgeSvdrpTransport.cpp").read_text()
daemon = (root / "core/daemon/src/DaemonRuntimeInitialization.cpp").read_text()
test = (root / "core/agent/tests/test_suite_bridge_video_directory_discovery.cpp").read_text()
for name, body, required in (
    ("plugin", plugin, ("<vdr/videodir.h>", "cVideoDirectory::Name()", '"VPATH"', 'ReplyCode = 900;')),
    ("transport", transport, ('SuiteBridgeLocalCommand::VideoDirectory', '"PLUG suitebridge VPATH 1\\r\\n"')),
    ("daemon", daemon, ("backendRuntimeContext->backendId == bridge.backendId",
                       "!explicitlyConfigured", "SuiteBridgeVideoDirectoryDiscovery::discover",
                       "recordingActionConfig.videoDirectoryExplicit = rootAvailable",
                       "invalidNativeRoot", "recordingActionConfig.readOnly")),
    ("test", test, ("testNativeRoot", "testLegacyAndOffline", "testMaliciousOrInvalidNativeReplyFailsClosed")),
):
    for item in required:
        if item not in body:
            raise SystemExit(f"VPATH_WIRING=FAIL {name}: {item}")
print("VPATH_WIRING=PASS")
