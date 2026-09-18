#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
GEN_H = ROOT / "core/agent/include/BackendRuntimeGeneration.h"
GEN_CPP = ROOT / "core/agent/src/BackendRuntimeGeneration.cpp"
AGENT_REPO = ROOT / "core/agent/src/BackendAgentRepository.cpp"
LIFE_H = ROOT / "core/daemon/include/EmbeddedBackendLifecycle.h"
LIFE_CPP = ROOT / "core/daemon/src/EmbeddedBackendLifecycle.cpp"
AUTH_CPP = ROOT / "core/daemon/src/EmbeddedBackendTeletextAuthority.cpp"
TELETEXT_RUNTIME = ROOT / "core/daemon/src/DaemonTeletextRuntime.cpp"
DAEMON_RUNTIME = ROOT / "core/daemon/src/DaemonRuntime.cpp"
INIT = ROOT / "core/daemon/src/DaemonRuntimeInitialization.cpp"
POLL = ROOT / "core/daemon/src/DaemonRuntimePolling.cpp"
SOURCES = ROOT / "mk/daemon-sources.mk"

errors: list[str] = []

for path in (
    GEN_H, GEN_CPP, AGENT_REPO, LIFE_H, LIFE_CPP, AUTH_CPP,
    TELETEXT_RUNTIME, DAEMON_RUNTIME, INIT, POLL, SOURCES,
):
    if not path.is_file():
        errors.append(f"missing Phase 67 embedded lifecycle file: {path.relative_to(ROOT)}")

def read(path: Path) -> str:
    return path.read_text(encoding="utf-8") if path.is_file() else ""

generation = read(GEN_CPP)
agent_repo = read(AGENT_REPO)
lifecycle = read(LIFE_CPP)
authority = read(AUTH_CPP)
teletext = read(TELETEXT_RUNTIME)
runtime = read(DAEMON_RUNTIME)
initialization = read(INIT)
polling = read(POLL)
sources = read(SOURCES)

for fragment in (
    "backend_runtime_generations",
    "SELECT backend_generation AS value FROM backend_agents",
    "allocateInCurrentTransaction",
    "backend_generation_allocated",
):
    if fragment not in generation:
        errors.append(f"shared generation allocator missing fragment: {fragment}")

for fragment in (
    '#include "BackendRuntimeGeneration.h"',
    "generations.allocateInCurrentTransaction(",
    "result.backendGeneration = allocation.generation;",
    "latestGeneration <= current->backendGeneration",
):
    if fragment not in agent_repo:
        errors.append(f"external Agent connect missing shared generation fragment: {fragment}")

for fragment in (
    "embedded_backend_lifecycle",
    "startBackend(",
    "heartbeatBackend(",
    "stopBackend(",
    "statusForBackend(",
    "LeaseDurationSeconds = 30",
    "externalLeaseActive",
    "generations.latestGeneration(backendId)",
):
    if fragment not in lifecycle:
        errors.append(f"embedded lifecycle missing fragment: {fragment}")

for fragment in (
    "lifecycleService_.statusForBackend(backendId, now)",
    "result.online = state.online",
    "result.backendGeneration = state.backendGeneration",
):
    if fragment not in authority:
        errors.append(f"Teletext authority missing embedded lifecycle fragment: {fragment}")

for fragment in (
    "EmbeddedBackendTeletextAuthority",
    "EmbeddedBackendLifecycleService& embeddedBackendLifecycleService",
):
    if fragment not in teletext:
        errors.append(f"Teletext runtime missing embedded authority fragment: {fragment}")

for fragment in (
    "embeddedBackendLifecycleService_ =",
    "embeddedBackendLifecycleService_->ensureSchema()",
    "embeddedBackendLifecycleService_->startBackend(",
):
    if fragment not in initialization:
        errors.append(f"daemon initialization missing lifecycle fragment: {fragment}")

for fragment in (
    "SuiteBridgeObservationState::SnapshotCurrent",
    "embeddedBackendLifecycleService_->heartbeatBackend(",
):
    if fragment not in polling:
        errors.append(f"daemon polling missing lifecycle renewal fragment: {fragment}")

for fragment in (
    "*embeddedBackendLifecycleService_",
    "embeddedBackendLifecycleService_->stopBackend(",
    "embeddedBackendLifecycleService_.reset();",
):
    if fragment not in runtime:
        errors.append(f"daemon runtime missing lifecycle ownership fragment: {fragment}")

for source in (
    "core/daemon/src/EmbeddedBackendLifecycle.cpp",
    "core/daemon/src/EmbeddedBackendTeletextAuthority.cpp",
):
    if source not in sources:
        errors.append(f"daemon source manifest missing: {source}")

for forbidden in (
    "backendGeneration = 1",
    "providerGeneration",
    "ownershipGeneration",
):
    if forbidden in authority or forbidden in teletext:
        errors.append(f"Teletext authority must not synthesize backend generation: {forbidden}")

if errors:
    for error in errors:
        print(error, file=sys.stderr)
    raise SystemExit(1)

print("Phase 67 embedded backend lifecycle contract ok")
