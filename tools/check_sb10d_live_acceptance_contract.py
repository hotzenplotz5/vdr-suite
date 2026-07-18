#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools/run_sb10d_live_acceptance.py"
PROBE = ROOT / "core/agent/tests/test_suite_bridge_embedded_agent_runtime_live.cpp"
MAKEFILE = ROOT / "mk/agent-tests.mk"


def require(condition: bool, message: str) -> None:
    if not condition:
        print(f"ERROR: {message}", file=sys.stderr)
        raise SystemExit(1)


def main() -> int:
    runner = RUNNER.read_text(encoding="utf-8")
    probe = PROBE.read_text(encoding="utf-8")
    makefile = MAKEFILE.read_text(encoding="utf-8")

    require(
        "SuiteBridgeEmbeddedAgentRuntime runtime(config);" in probe,
        "live probe must exercise the exact SB.10d embedded runtime",
    )
    require(
        "initial_epoch" in probe and "final_epoch" in probe,
        "live probe must record restart epoch replacement",
    )
    require(
        "worker_stop=clean" in probe,
        "live probe must prove clean worker shutdown",
    )
    require(
        "mutationsEnabled" in probe,
        "live probe must reject mutation enablement",
    )

    for forbidden in (
        "DaemonRuntime",
        "ApiRouter",
        "sqlite3",
        "system(",
        "popen(",
        "mutationsEnabled = true",
    ):
        require(
            forbidden not in probe,
            f"live probe contains forbidden coupling: {forbidden}",
        )

    required_runner_tokens = (
        "os.geteuid()",
        "require_clean_worktree()",
        "Suite Bridge installation/configuration already exists",
        "capture_vdr_state(",
        "run_safe_daemon_probes(",
        "systemctl\", \"restart\", VDR_SERVICE",
        "vdrctl\", \"disable\", \"suitebridge",
        "wait_plugin_mapped(False)",
        "original_daemon_active",
        "stateBefore",
        "stateAfter",
        "rollbackComplete",
        "finally:",
    )
    for token in required_runner_tokens:
        require(token in runner, f"live runner is missing safety contract: {token}")

    first_install = runner.index('"install",')
    rollback_armed = runner.index("plugin_staged = True")
    require(
        rollback_armed < first_install,
        "plugin rollback must be armed before the first external install write",
    )

    for forbidden in (
        "shell=True",
        "os.system(",
        "subprocess.Popen(\"",
        "--max-risk\", \"destructive",
        "/api/vdr/timers/delete",
        "/api/vdr/recordings/delete",
    ):
        require(
            forbidden not in runner,
            f"live runner contains forbidden operation: {forbidden}",
        )

    require(
        "build-suite-bridge-embedded-runtime-live" in makefile,
        "Make must expose the reproducible live-probe build target",
    )
    require(
        "test-real-suite-bridge-embedded-runtime-live" in makefile,
        "Make must expose the explicit opt-in live target",
    )

    print("check_sb10d_live_acceptance_contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
