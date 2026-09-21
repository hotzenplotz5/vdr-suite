#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools/run_sb10d_live_acceptance.py"
PROBE = ROOT / "core/agent/tests/test_suite_bridge_embedded_agent_runtime_live.cpp"
MAKEFILE = ROOT / "mk/agent-tests.mk"
LIVE_SOURCE = ROOT / "vdr-plugin-suite-bridge/suitebridge_live_source.h"


def require(condition: bool, message: str) -> None:
    if not condition:
        print(f"ERROR: {message}", file=sys.stderr)
        raise SystemExit(1)


def main() -> int:
    runner = RUNNER.read_text(encoding="utf-8")
    probe = PROBE.read_text(encoding="utf-8")
    makefile = MAKEFILE.read_text(encoding="utf-8")
    live_source = LIVE_SOURCE.read_text(encoding="utf-8")

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
        "run_disabled_daemon_connection_probe(",
        "disabledDaemonNoSuiteBridgeConnection",
        "stateBefore",
        "stateAfter",
        "rollbackComplete",
        "finally:",
    )
    for token in required_runner_tokens:
        require(token in runner, f"live runner is missing safety contract: {token}")

    install_operation = '''        run(
            [
                "install",
'''
    require(
        runner.count(install_operation) == 1,
        "live runner must contain one exact external install operation",
    )
    first_install = runner.index(install_operation)
    rollback_armed = runner.index("        plugin_staged = True")
    require(
        rollback_armed < first_install,
        "plugin rollback must be armed before the first external install write",
    )

    require(
        'result.get("saw_degraded") is not True' in runner,
        "live runner must require an observed degraded restart state",
    )
    require(
        'glob("libvdr-suitebridge.so*")' in runner,
        "live runner must reject and detect every Suite Bridge binary variant",
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

    # SuiteBridge live OPEN must use the same VDR-owned channel-switch path as
    # SVDRP CHAN and RESTfulAPI /remote/switch. It must not independently tune a
    # non-live-view device, which can consume a second tuner and diverge from
    # VDR's normal receiver-detach/Transfer-Mode lifecycle.
    required_live_source_tokens = (
        "Channels->SwitchTo(channel->Number())",
        "device = cDevice::ActualDevice()",
        "device->IsTunedToTransponder(channel)",
        "device->AttachReceiver(receiver.get())",
        "device->Detach(receiver.get())",
    )
    for token in required_live_source_tokens:
        require(token in live_source, f"live source is missing VDR switch contract: {token}")

    for forbidden in (
        "cDevice::GetDevice(channel, LIVEPRIORITY, false)",
        "device->SwitchChannel(channel, false)",
    ):
        require(
            forbidden not in live_source,
            f"live source must not bypass VDR live switching: {forbidden}",
        )

    switch_position = live_source.index("Channels->SwitchTo(channel->Number())")
    device_position = live_source.index("device = cDevice::ActualDevice()")
    attach_position = live_source.index("device->AttachReceiver(receiver.get())")
    require(
        switch_position < device_position < attach_position,
        "live source must switch through VDR before resolving and attaching ActualDevice",
    )

    print("check_sb10d_live_acceptance_contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
