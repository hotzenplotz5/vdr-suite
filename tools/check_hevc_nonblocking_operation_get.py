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
print("HEVC_NONBLOCKING_GET_BACKGROUND_SSE=PASS")
