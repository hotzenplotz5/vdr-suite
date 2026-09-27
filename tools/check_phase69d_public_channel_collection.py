#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "runtime_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "runtime_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "router": ROOT / "api/rest/include/ApiRouter.h",
    "http": ROOT / "core/http/src/TestHttpServer.cpp",
    "security": ROOT / "core/security/include/SecurityHttpGate.h",
    "daemon_init": ROOT / "core/daemon/src/DaemonRuntimeInitialization.cpp",
    "daemon_shutdown": ROOT / "core/daemon/src/DaemonRuntimeShutdown.cpp",
    "runtime_test": ROOT / "api/rest/tests/test_public_channel_collection.cpp",
    "security_test": ROOT / "core/security/tests/test_public_channel_collection_security.cpp",
    "inventory": ROOT / "tools/check_phase69_public_api_inventory.py",
    "doc": ROOT / "docs/development/phase-69d-public-channel-federation.md",
    "current": ROOT / "docs/CURRENT.md",
}

def read(name):
    path = FILES[name]
    if not path.is_file():
        raise SystemExit(
            f"missing Phase 69.D Channel collection file: {path.relative_to(ROOT)}"
        )
    return path.read_text(encoding="utf-8")

def require(text, token, label):
    if token not in text:
        raise SystemExit(f"missing {label}: {token}")

runtime_h = read("runtime_h")
for token in (
    "PublicChannelCollectionRequest",
    "PublicChannelCollectionResult",
    "registerChannelCollectionLookup",
    "resetChannelCollectionLookup",
    "authorizedBackendIds",
):
    require(runtime_h, token, "public Channel runtime boundary")

runtime_cpp = read("runtime_cpp")
for token in (
    'PublicChannelCollectionPath =\n    "/api/v1/channels"',
    "PublicChannelDefaultLimit = 50U",
    "PublicChannelMaximumLimit = 100U",
    "PublicChannelMaximumSources = 16U",
    'PublicChannelCollectionSort =\n    "backendId,channelId"',
    'PublicChannelCursorPrefix = "ch1_"',
    "PublicChannelCursorDecodeStatus::scopeMismatch",
    '"cursor_expired"',
    '"backend_unavailable"',
    '\\"partial\\":',
    '\\"sources\\":[',
):
    require(runtime_cpp, token, "federated public collection contract")

security = read("security")
for token in (
    'path == "/api/v1/channels"',
    'queryStringValues(request.path, "backendId")',
    'channelReadRequest.permission = "channels.view"',
    "gate.authorizedBackendIds",
    "gate.publicApiV1",
):
    require(security, token, "per-backend Channel authorization")

require(read("http"), "gate.authorizedBackendIds", "authorized source propagation")
require(read("router"), "authorizedBackendRefs", "router source propagation")

daemon = read("daemon_init")
start = daemon.find("registerChannelCollectionLookup")
end = daemon.find("registerTimerCreateAdmission", start)
if min(start, end) < 0:
    raise SystemExit("missing bounded daemon Channel callback window")
window = daemon[start:end]
for token in (
    "backendRegistryService_->getBackend",
    "hasSnapshotForBackend",
    "getChannelsForBackend",
    "backend->online",
    "backend->enabled",
):
    require(window, token, "existing read-authority composition")
for forbidden in (
    "getRecordingsForBackend",
    "LiveRemoteApiRuntime",
    "TimerAssignmentRepository",
):
    if forbidden in window:
        raise SystemExit(f"forbidden Channel callback coupling: {forbidden}")

require(
    read("daemon_shutdown"),
    "resetChannelCollectionLookup",
    "lifecycle-safe callback reset",
)

runtime_test = read("runtime_test")
for token in (
    '\\"partial\\":true',
    "backend_unavailable",
    "cursor_expired",
    "offset=1",
    'first.headers.find("ETag") == first.headers.end()',
):
    require(runtime_test, token, "runtime regression coverage")

security_test = read("security_test")
for token in (
    'Permission = "channels.view"',
    "authorizedBackendIds",
    "backend_scope_denied",
    "invalid_backend_scope",
):
    require(security_test, token, "security regression coverage")

require(read("inventory"), '"/api/v1/channels",', "stable public route inventory")

doc = read("doc")
for token in (
    "GET /api/v1/channels",
    "(backendId, channelId)",
    "backendId ASC, channelId ASC",
    "partial=true",
    "all requested sources fail",
    "no collection ETag",
    "Home",
    "LiveTV",
):
    require(doc, token, "documented Phase 69.D Channel contract")

require(
    read("current"),
    "[Phase 69.D federated Channel collection]",
    "CURRENT Phase 69.D navigation",
)

print("Phase 69.D federated public Channel collection guard passed.")
print(
    "Boundary: existing BackendRegistry + VdrSnapshotReadService only; "
    "explicit authorized sources, keyset pagination, partial source metadata; "
    "no Home, LiveTV, Recording or Timer mutation change."
)
