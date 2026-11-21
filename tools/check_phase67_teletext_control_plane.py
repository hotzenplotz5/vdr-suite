#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
SERVICE_H = ROOT / "core/daemon/include/TeletextControlPlaneReadService.h"
SERVICE_CPP = ROOT / "core/daemon/src/TeletextControlPlaneReadService.cpp"
AUTH_H = ROOT / "core/daemon/include/EmbeddedBackendTeletextAuthority.h"
AUTH_CPP = ROOT / "core/daemon/src/EmbeddedBackendTeletextAuthority.cpp"
TEST = ROOT / "core/daemon/tests/test_teletext_control_plane_read_service.cpp"

errors: list[str] = []

for path in (SERVICE_H, SERVICE_CPP, AUTH_H, AUTH_CPP, TEST):
    if not path.is_file():
        errors.append(f"missing Phase 67 Teletext control-plane file: {path.relative_to(ROOT)}")

service_h = SERVICE_H.read_text(encoding="utf-8") if SERVICE_H.is_file() else ""
service_cpp = SERVICE_CPP.read_text(encoding="utf-8") if SERVICE_CPP.is_file() else ""
auth_cpp = AUTH_CPP.read_text(encoding="utf-8") if AUTH_CPP.is_file() else ""
test = TEST.read_text(encoding="utf-8") if TEST.is_file() else ""

for fragment in (
    "class ITeletextBackendAuthority",
    "struct TeletextBackendAuthorityState",
    "class TeletextControlPlaneReadService",
    "TeletextServiceSnapshot discoverService(",
    "TeletextPageSnapshot readPage(",
    "SuiteBridgeTeletextResolver*(const std::string&)",
):
    if fragment not in service_h:
        errors.append(f"control-plane header missing fragment: {fragment}")

for fragment in (
    "backendRegistryService_.getBackend(backendId)",
    "snapshotReadService_.hasSnapshotForBackend(backendId)",
    "snapshotReadService_.getChannelsForBackend(backendId)",
    "backendAuthority_.stateForBackend(backendId, nowProvider_())",
    "teletext_backend_generation_unavailable",
    "teletext_backend_not_online",
    "teletext_backend_generation_mismatch",
    "teletext_channel_not_in_backend_snapshot",
    "resolver->discoverService(",
    "resolver->readPage(",
):
    if fragment not in service_cpp:
        errors.append(f"control-plane implementation missing fence fragment: {fragment}")

if service_cpp.count("fenceContext(") < 5:
    errors.append("control-plane service must fence before and after provider reads")

for fragment in (
    "lifecycleService_.statusForBackend(backendId, now)",
    "state.online",
    "state.backendGeneration",
):
    if fragment not in auth_cpp:
        errors.append(f"Embedded backend authority missing lifecycle fragment: {fragment}")

for forbidden in (
    "TTXC 1",
    "TTXP 1",
    "/var/cache/vdr/vtx",
    "OsdTeletext::",
    "CallFirstService",
    "backendGeneration = 1",
    "backendGeneration{1}",
):
    if forbidden in service_h or forbidden in service_cpp or forbidden in auth_cpp:
        errors.append(f"control-plane layer must not own provider/wire detail: {forbidden}")

for fragment in (
    "testDiscoveryUsesAuthoritativeGenerationAndChannelFence",
    "testPageReadPreservesNormalizedProviderEvidence",
    "testGenerationMismatchFailsBeforeProviderRead",
    "testGenerationChangeDuringReadFailsClosed",
    "testUnknownChannelFailsBeforeProviderDiscovery",
    "testOfflineBackendFailsBeforeProviderDiscovery",
    '"Börse und Nachrichten"',
):
    if fragment not in test:
        errors.append(f"control-plane regression test missing coverage: {fragment}")

if errors:
    for error in errors:
        print(error, file=sys.stderr)
    raise SystemExit(1)

print("Phase 67 Teletext control-plane read contract ok")
