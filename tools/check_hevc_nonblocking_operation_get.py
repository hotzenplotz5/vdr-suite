#!/usr/bin/env python3
"""Bounded structural regression: an actor-owned HEVC operation GET is never an FFprobe worker."""

from pathlib import Path

init = Path("core/daemon/src/DaemonRuntimeInitialization.cpp").read_text()
worker = Path("core/daemon/src/DaemonRuntime.cpp").read_text()
hero = Path("web/frontend/recordings2-hero-detail.js").read_text()

anchor = '503, "HEVC operation readback unavailable."'
start = init.find(anchor)
assert start >= 0, "HEVC operation GET registration missing"
end = init.find("const auto& op = found.operation;", start)
assert end > start, "operation GET response boundary missing"
get_handler = init[start:end]

assert "tryReconcileMediaShrinkStage(" not in get_handler, (
    "HTTP GET must not execute full private Stage readback"
)
assert "NativeProcessRunner" not in get_handler, (
    "HTTP GET must not start a potentially long FFprobe"
)
assert "mutationOperationReadService_->findForActor" in get_handler, (
    "actor-owned status must remain the authoritative read"
)
assert "recordingCacheRefreshQueue_.request(backend)" in get_handler, (
    "unknown Publish status must retain reconciliation hints"
)
assert "void DaemonRuntime::runMediaShrinkRecoveryWorker()" in worker
assert "tryReconcileMediaShrinkStage(" in worker, (
    "private readback must remain Core-owned, in the background"
)
assert "HEVC_STAGE_RECOVERY=BLOCKED" in worker, (
    "failed readbacks need actionable bounded diagnostics"
)
assert "deviceLiveEventHub_->publishOperation(op.operationId)" in init, (
    "verified recovery must notify actor-owned SSE"
)
assert "watchOperation('stage')" in hero, (
    "Recordings detail must retain actor-owned operation SSE"
)
# VDR RESTfulAPI carries both paths: relative_file_name for folders and
# absolute file_name for native operations. Publish readback MUST use the latter
# or a genuine native .shrink recording can never complete its Operation.
mapper = Path("core/vdr/src/RestfulApiRecordingMapper.cpp").read_text()
reconciliation = Path(
    "core/daemon/src/DaemonRuntimeRecordingCache.cpp"
).read_text()
assert "recording.path = relativePath.empty() ? fileName : relativePath;" in mapper
assert "recording.backendNativeId = fileName;" in mapper
start = reconciliation.index("// Mirror the native No-Replace target")
end = reconciliation.index("if (matches != 1) continue;", start)
lookup = reconciliation[start:end]
assert "fs::path(recording.backendNativeId)" in lookup, (
    "HEVC Publish must match the absolute native VDR file_name"
)
assert "fs::path(recording.path) == publishedNative" not in lookup, (
    "relative_file_name cannot match the absolute published directory"
)
assert "fs::path(source->relativeRecording.parent_path()" in lookup
assert 'string() + ".shrink")' in lookup
print("HEVC_PUBLISH_NATIVE_READBACK=PASS")

print("HEVC_NONBLOCKING_GET_BACKGROUND_SSE=PASS")
