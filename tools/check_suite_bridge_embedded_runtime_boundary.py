#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "core/agent/include/SuiteBridgeEmbeddedAgentRuntime.h"
SOURCE = ROOT / "core/agent/src/SuiteBridgeEmbeddedAgentRuntime.cpp"
CONTEXT = ROOT / "core/daemon/include/BackendRuntimeContext.h"
AGENT_SOURCES = ROOT / "mk/agent-sources.mk"


def require(condition: bool, message: str) -> None:
    if not condition:
        print(f"ERROR: {message}", file=sys.stderr)
        raise SystemExit(1)


def main() -> int:
    header = HEADER.read_text(encoding="utf-8")
    source = SOURCE.read_text(encoding="utf-8")
    context = CONTEXT.read_text(encoding="utf-8")
    agent_sources = AGENT_SOURCES.read_text(encoding="utf-8")
    combined = header + source

    require(
        "SuiteBridgeEmbeddedAgentConfig" in header,
        "embedded Agent runtime must expose typed configuration",
    )
    require(
        "SuiteBridgeEmbeddedAgentHealth" in header,
        "embedded Agent runtime must expose bounded health",
    )
    require(
        "std::make_unique<SuiteBridgeSvdrpTransport>" in source,
        "production runtime must own the accepted typed SVDRP transport",
    )
    require(
        "std::make_unique<SuiteBridgeObservationWorker>" in source,
        "runtime must compose the accepted SB.10c worker",
    )
    require(
        "std::unique_ptr<ISuiteBridgeLocalTransport>" in header,
        "testability must remain behind the typed local transport boundary",
    )
    require(
        "SuiteBridgeEmbeddedAgentRuntime> suiteBridgeAgentRuntime" in context,
        "BackendRuntimeContext must own the embedded Agent runtime",
    )
    require(
        "core/agent/src/SuiteBridgeEmbeddedAgentRuntime.cpp" in agent_sources,
        "embedded runtime source must be owned by AGENT_SRC",
    )

    forbidden = (
        "ApiRouter",
        "Database",
        "sqlite3",
        "RestfulApiVdrAdapter",
        "PLUG suitebridge",
        "system(",
        "popen(",
        "fork(",
        "mutationsEnabled = true",
    )
    for token in forbidden:
        require(
            token not in combined,
            f"embedded Agent runtime must not contain forbidden coupling: {token}",
        )

    print("check_suite_bridge_embedded_runtime_boundary passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
