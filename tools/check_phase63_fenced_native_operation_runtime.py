#!/usr/bin/env python3
"""Static boundary guard for the Phase-63 fenced native probe runtime."""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]

# Human-readable summary required by the merged contract guard. The executable
# checks below prove each invariant against the actual Agent/SuiteBridge sources.
CONTRACT_INVARIANTS = (
    "vdr.native.probe only",
    "sideEffectClass = none",
    "mutations=disabled",
    "native execution sequence",
    "plugin instance epoch",
    "no production VDR mutation",
)

REQUIRED = [
    "core/agent/include/BackendAgentNativeProbe.h",
    "core/agent/src/BackendAgentNativeProbe.cpp",
    "core/agent/src/BackendAgentNativeProbeDelivery.cpp",
    "core/agent/src/BackendAgentCommandClient.cpp",
    "core/agent/src/BackendAgentCommandJson.cpp",
    "core/agent/include/SuiteBridgeSvdrpTransport.h",
    "core/agent/tests/test_backend_agent_native_probe.cpp",
    "core/agent/tests/test_backend_agent_native_probe_runtime.cpp",
    "core/agent/tests/test_backend_agent_native_probe_delivery.cpp",
    "core/agent/tests/test_suite_bridge_svdrp_native_probe_transport.cpp",
    "vdr-plugin-suite-bridge/suitebridge_native_probe.h",
    "vdr-plugin-suite-bridge/suitebridge_native_probe.cpp",
    "vdr-plugin-suite-bridge/tests/test_suitebridge_native_probe.cpp",
    "tools/run_phase63_fenced_native_operation_acceptance.sh",
    "docs/development/phase-63-fenced-native-operation-runtime.md",
]

errors: list[str] = []


def text(relative: str) -> str:
    path = ROOT / relative
    if not path.is_file():
        errors.append(f"missing required file: {relative}")
        return ""
    return path.read_text(encoding="utf-8")


for item in REQUIRED:
    text(item)

agent_protocol = text("core/agent/src/BackendAgentNativeProbe.cpp")
agent_client = text("core/agent/src/BackendAgentCommandClient.cpp")
backend_agent_client = text("core/agent/src/BackendAgentClient.cpp")
backend_agent_client_test = text(
    "core/agent/tests/test_backend_agent_client.cpp"
)
agent_command_json = text("core/agent/src/BackendAgentCommandJson.cpp")
agent_command = text("core/agent/src/BackendAgentCommand.cpp")
command_delivery = text(
    "core/agent/src/BackendAgentCommandDelivery.cpp"
)
command_delivery_test = text(
    "core/agent/tests/test_backend_agent_command_delivery.cpp"
)
agent_tests_make = text("mk/agent-tests.mk")
native_runtime_test = text(
    "core/agent/tests/test_backend_agent_native_probe_runtime.cpp"
)
transport_header = text("core/agent/include/SuiteBridgeSvdrpTransport.h")
agent_main = text("apps/agent/main.cpp")
plugin_header = text("vdr-plugin-suite-bridge/suitebridge_native_probe.h")
plugin_runtime = text("vdr-plugin-suite-bridge/suitebridge_native_probe.cpp")
plugin_svdrp = text("vdr-plugin-suite-bridge/suitebridge_svdrp.cpp")
packaged_config = text("packaging/systemd/backend-agent.conf")
acceptance = text("tools/run_phase63_fenced_native_operation_acceptance.sh")
acceptance_proxy = text(
    "tools/phase63-runtime-acceptance/native-probe-proxy.py"
)

required_tokens = {
    "agent protocol": (
        agent_protocol,
        [
            "vdr.native.probe",
            "nativeOperationSchema",
            "pluginInstanceEpoch",
            "sideEffectObserved",
            "backendAgentNativeProbeReceiptEvidence",
            "backendAgentNativeProbeResultEvidence",
            "backendAgentNativeProbeReadbackEvidence",
        ],
    ),
    "agent client": (
        agent_client,
        [
            "state.dispatchState",
            "native_capability_evidence",
            "native_receipt_evidence",
            "native_result_evidence",
            "native_readback_evidence",
            "native_probe_dispatch_reconciliation_required",
            "native probe old plugin epoch cannot be replayed",
        ],
    ),
    "plugin runtime": (
        plugin_header + plugin_runtime,
        [
            "ReceiptCapacity",
            "native_receipt_capacity_exhausted",
            "receiptCategory",
            "duplicateDisposition",
            "mutationsState",
            "disabled",
            "sideEffectObserved",
        ],
    ),
}
for label, (content, tokens) in required_tokens.items():
    for token in tokens:
        if token not in content:
            errors.append(f"{label} missing required token: {token}")

if 'value.commandType == "probe.noop"' not in agent_command or \
        'value.commandType == "vdr.native.probe"' not in agent_command:
    errors.append("command validation must preserve legacy probe.noop and add only vdr.native.probe")
if 'value.verificationPolicy == "readback_required"' not in agent_command:
    errors.append("native assignment must require readback")

if not re.search(
    r'item\.stringValue\s*!=\s*"probe\.noop"\s*&&\s*'
    r'item\.stringValue\s*!=\s*"vdr\.native\.probe"',
    agent_command_json,
):
    errors.append(
        "command poll parser must accept probe.noop and vdr.native.probe only"
    )

if (
    'nativePoll.supportedCommandTypes={"vdr.native.probe"};'
    not in native_runtime_test
    or 'unsupportedPoll.supportedCommandTypes={"vdr.native.mutate"};'
    not in native_runtime_test
):
    errors.append(
        "native command poll protocol requires positive and negative regression coverage"
    )

for token in [
    "const auto sleepUntilStop",
    "sleep_(1);",
    "if (!sleepUntilStop(reconnectDelay)) break;",
    "if (!sleepUntilStop(config_.heartbeatIntervalSeconds)) break;",
]:
    if token not in backend_agent_client:
        errors.append(
            "Backend Agent runtime wait is not stop-interruptible: "
            + token
        )

if "sleep_(config_.heartbeatIntervalSeconds);" in backend_agent_client:
    errors.append(
        "Backend Agent must not sleep for a full heartbeat interval"
    )

for token in [
    "test_heartbeat_wait_is_interruptible",
    "if (sleeps.size() == 7) stopRequested = true;",
    "assert(sleeps == std::vector<int>(7, 1));",
    "assert(sleeps == std::vector<int>({1}));",
    "assert(transport.paths.size() == requestsBeforeWait);",
]:
    if token not in backend_agent_client_test:
        errors.append(
            "Backend Agent interruptible wait regression missing token: "
            + token
        )

command_begin_count = command_delivery.count(
    'database_.execute("BEGIN IMMEDIATE;")'
)
command_lease_count = command_delivery.count(
    "database_.acquireTransactionLease()"
)

if command_begin_count != 4:
    errors.append(
        "command repository must retain exactly four explicit transactions"
    )

if command_lease_count != command_begin_count:
    errors.append(
        "every command repository transaction must hold the database lease"
    )

for token in [
    "std::atomic<bool> pollFinished{false};",
    'assert(db.execute("BEGIN IMMEDIATE;"));',
    "const bool pollBlocked=!pollFinished.load();",
    "transactionLease.unlock();",
    "pollThread.join();",
]:
    if token not in command_delivery_test:
        errors.append(
            "command transaction concurrency regression missing token: "
            + token
        )

command_target_start = agent_tests_make.find(
    "test-phase63-command-delivery-runtime:"
)
command_target_end = agent_tests_make.find(
    "\ntest-fast: test-phase63-command-delivery-runtime",
    command_target_start,
)
command_target = (
    agent_tests_make[command_target_start:command_target_end]
    if command_target_start >= 0 and command_target_end > command_target_start
    else ""
)

if (
    "$(BUILD_CXX) $(CXXFLAGS) -pthread \\\n"
    "\t\t$(SQLITE_SRC)"
    not in command_target
):
    errors.append(
        "command delivery concurrency test compile must use -pthread"
    )

execute = agent_client.find("executeNativeProbe(request)")
starting_matches = list(re.finditer(
    r'state\.dispatchState\s*=\s*"starting"',
    agent_client[:execute] if execute >= 0 else "",
))
starting = starting_matches[-1].start() if starting_matches else -1
persist_starting = agent_client.find(
    "persist(config.statePath, state, reason)", starting, execute)
if min(starting, persist_starting, execute) < 0 or not (starting < persist_starting < execute):
    errors.append("durable starting must precede native dispatch")

reserve = plugin_runtime.find("ReceiptEntry *entry = reserve()")
state_capture = plugin_runtime.find("vdrActiveProbe()", reserve)
if min(reserve, state_capture) < 0 or reserve >= state_capture:
    errors.append("plugin receipt reservation must precede VDR state capture")
if "replacementIndex_" in plugin_header + plugin_runtime:
    errors.append("receipt ledger must fail closed instead of evicting receipts")
if "return nullptr;" not in plugin_runtime[plugin_runtime.find("ReceiptEntry *SuiteBridgeNativeProbeService::reserve"):]:
    errors.append("full receipt ledger must fail closed")

help_start = plugin_svdrp.find("SVDRPHelpPages")
command_start = plugin_svdrp.find("SVDRPCommand", help_start)
help_section = plugin_svdrp[help_start:command_start]
if "NCAP" in help_section or "NPROBE" in help_section:
    errors.append("native probe commands must not be advertised as public SVDRP help")

private_index = transport_header.find("private:")
execute_request_index = transport_header.find("executeRequest(", private_index)
if private_index < 0 or execute_request_index < private_index:
    errors.append("free SVDRP request primitive must remain private")
for token in ["discoverNativeProbe", "executeNativeProbe", "readNativeProbe"]:
    if token not in transport_header:
        errors.append(f"typed local transport missing {token}")

if "--native-probe" not in agent_main or "loopbackHost" not in agent_main or \
        'config.commandTypes = {"vdr.native.probe"}' not in agent_main:
    errors.append("runtime activation must be explicit and loopback-only")
if "COMMAND_TYPES=\n" not in packaged_config:
    errors.append("packaged command types must remain disabled by default")

scoped_runtime = "\n".join([
    agent_protocol,
    agent_client,
    agent_command_json,
    transport_header,
    plugin_header,
    plugin_runtime,
    agent_main,
    text("core/agent/src/BackendAgentNativeProbeDelivery.cpp"),
])
if "mutations=enabled" in scoped_runtime or '"enabled"' in scoped_runtime:
    errors.append("mutations=enabled is forbidden")

for pattern in [
    r"\b(?:system|popen|fork|execl|execv|posix_spawn)\s*\(",
    r"\b(?:timer|recording|searchtimer|channel|epg|remote|osd)[A-Za-z_]*\s*=",
]:
    if re.search(pattern, scoped_runtime, flags=re.IGNORECASE):
        errors.append(f"forbidden runtime boundary matched: {pattern}")

for forbidden in ["set -e", "set -u", "set -o pipefail"]:
    if forbidden in acceptance:
        errors.append(f"acceptance runner contains forbidden shell option: {forbidden}")
if "|| fail" not in acceptance:
    errors.append("acceptance runner must use explicit failure handling")
for token in [
    "RUNTIME_TOUCHED=0",
    "RUNTIME_TOUCHED=1",
    "DROPIN_DIR_EXISTED=0",
    "make clean || fail candidate_clean_failed",
    "installed.sha256",
    'AGENT_UNIT="${AGENT_SERVICE%.service}.service"',
    'DROPIN_DIR="/etc/systemd/system/${AGENT_UNIT}.d"',
    "agent.candidate-execstart.txt",
    "agent_native_probe_dropin_not_loaded",
    "minimum_heartbeat_at",
    'value.get("lastHeartbeatAt",0)',
    "baseline.enqueue.last-output",
    "trap restore_runtime EXIT",
    "trap 'restore_runtime; exit 130' INT",
    "trap 'restore_runtime; exit 143' TERM",
]:
    if token not in acceptance:
        errors.append(f"acceptance runner missing safety token: {token}")
if "trap restore_runtime EXIT INT TERM" in acceptance:
    errors.append("acceptance signal traps must restore and terminate")
clean = acceptance.find("make clean || fail candidate_clean_failed")
build = acceptance.find("make daemon backend-agent", clean)
touched = acceptance.find("RUNTIME_TOUCHED=1")
first_runtime_stop = acceptance.find('systemctl stop "$AGENT_SERVICE"', touched)
if min(clean, build) < 0 or clean >= build:
    errors.append("acceptance must clean before building exact candidates")
if min(touched, first_runtime_stop) < 0 or touched >= first_runtime_stop:
    errors.append("acceptance must mark runtime touched before first service stop")

daemon_restart = acceptance.find(
    'systemctl restart "$DAEMON_SERVICE"'
)
baseline_replay_request = acceptance.find(
    '--replay "$BASELINE_ID"',
    daemon_restart,
)
baseline_replay_verified = acceptance.find(
    "control_plane_replay_reexecuted_native_probe",
    baseline_replay_request,
)
agent_restart = acceptance.find(
    'systemctl restart "$AGENT_SERVICE"',
    baseline_replay_verified,
)
recovery_enqueue = acceptance.find(
    'RECOVERY_ASSIGNMENT=',
    agent_restart,
)

if (
    min(
        daemon_restart,
        baseline_replay_request,
        baseline_replay_verified,
        agent_restart,
        recovery_enqueue,
    )
    < 0
    or not (
        daemon_restart
        < baseline_replay_request
        < baseline_replay_verified
        < agent_restart
        < recovery_enqueue
    )
):
    errors.append(
        "baseline replay must complete before agent generation replacement"
    )

epoch_start = acceptance.find("EPOCH_ASSIGNMENT=")
epoch_end = acceptance.find("FINAL_ASSIGNMENT=", epoch_start)
epoch_section = (
    acceptance[epoch_start:epoch_end]
    if epoch_start >= 0 and epoch_end > epoch_start
    else ""
)

for token in [
    "AGENT_PAUSED=0",
    "--signal=SIGSTOP",
    "--signal=SIGCONT",
    "agent.after-epoch-resume.json",
    "agent.epoch-process.txt",
    "agent_epoch_process_changed",
    "agent_epoch_generation_changed",
]:
    if token not in acceptance:
        errors.append(
            "epoch acceptance context preservation missing token: "
            + token
        )

if (
    'systemctl stop "$AGENT_SERVICE"' in epoch_section
    or 'systemctl start "$AGENT_SERVICE"' in epoch_section
):
    errors.append(
        "epoch acceptance must preserve the existing Agent process"
    )

epoch_drop = epoch_section.find(
    'wait_proxy_drop "$EPOCH_ID" 90'
)
epoch_pause = epoch_section.find(
    "--signal=SIGSTOP",
    epoch_drop,
)
epoch_vdr_restart = epoch_section.find(
    'systemctl restart "$VDR_SERVICE"',
    epoch_pause,
)
epoch_resume = epoch_section.find(
    "--signal=SIGCONT",
    epoch_vdr_restart,
)
epoch_online = epoch_section.find(
    "agent.after-epoch-resume.json",
    epoch_resume,
)
epoch_pid_check = epoch_section.find(
    "agent_epoch_process_changed",
    epoch_online,
)
epoch_generation_check = epoch_section.find(
    "agent_epoch_generation_changed",
    epoch_pid_check,
)
epoch_status = epoch_section.find(
    "EPOCH_STATUS=",
    epoch_generation_check,
)

if (
    min(
        epoch_drop,
        epoch_pause,
        epoch_vdr_restart,
        epoch_resume,
        epoch_online,
        epoch_pid_check,
        epoch_generation_check,
        epoch_status,
    )
    < 0
    or not (
        epoch_drop
        < epoch_pause
        < epoch_vdr_restart
        < epoch_resume
        < epoch_online
        < epoch_pid_check
        < epoch_generation_check
        < epoch_status
    )
):
    errors.append(
        "epoch acceptance must pause, restart VDR, resume and verify "
        "the unchanged Agent context before evaluating the fence"
    )

for token in [
    "PHASE63_REUSE_CANDIDATES_FROM_EVIDENCE",
    "PY_REUSE_SCOPE",
    'REUSE_SCOPE_RC="$?"',
    "reuse_candidate_build_inputs_changed",
    "sha256sum -c",
    "CANDIDATE_BUILD_REUSED=yes",
    "epoch_drop_hold_arm_failed",
    "epoch_drop_release_failed",
]:
    if token not in acceptance:
        errors.append(
            "acceptance deterministic epoch/reuse missing token: "
            + token
        )

if "<<'PY_REUSE_SCOPE' ||" in acceptance:
    errors.append(
        "reuse scope heredoc must not attach shell failure "
        "handling to the opening delimiter"
    )

reuse_scope_close = acceptance.find(
    '\nPY_REUSE_SCOPE\n    REUSE_SCOPE_RC="$?"'
)
reuse_scope_failure = acceptance.find(
    "reuse_candidate_build_inputs_changed",
    reuse_scope_close,
)
reuse_scope_hash = acceptance.find(
    'sha256sum -c "$REUSE_CANDIDATES_FROM/candidates.sha256"',
    reuse_scope_failure,
)

if (
    min(
        reuse_scope_close,
        reuse_scope_failure,
        reuse_scope_hash,
    )
    < 0
    or not (
        reuse_scope_close
        < reuse_scope_failure
        < reuse_scope_hash
    )
):
    errors.append(
        "candidate reuse scope must fail closed before "
        "candidate hash acceptance"
    )

for token in [
    "consume_drop_mode",
    'raw == "hold"',
    'drop_mode == "hold"',
    "stop_file.exists()",
    "STOP.wait(0.05)",
]:
    if token not in acceptance_proxy:
        errors.append(
            "acceptance proxy hold missing token: "
            + token
        )

hold_arm = acceptance.rfind(
    'printf \'hold\\n\' >"$PROXY_DROP"',
    0,
    epoch_start,
)

hold_drop = acceptance.find(
    'wait_proxy_drop "$EPOCH_ID" 90',
    epoch_start,
    epoch_end,
)

hold_pause = acceptance.find(
    "--signal=SIGSTOP",
    hold_drop,
    epoch_end,
)

hold_release = acceptance.find(
    'printf \'0\\n\' >"$PROXY_DROP"',
    hold_pause,
    epoch_end,
)

hold_vdr_restart = acceptance.find(
    'systemctl restart "$VDR_SERVICE"',
    hold_release,
    epoch_end,
)

if (
    min(
        hold_arm,
        epoch_start,
        hold_drop,
        hold_pause,
        hold_release,
        hold_vdr_restart,
    )
    < 0
    or not (
        hold_arm
        < epoch_start
        < hold_drop
        < hold_pause
        < hold_release
        < hold_vdr_restart
    )
):
    errors.append(
        "epoch response loss must remain held until "
        "the Agent is paused before VDR restart"
    )

if "sqlite3" in acceptance:
    errors.append("acceptance must not use manual SQLite inspection")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("Phase-63 fenced native operation runtime guard passed")
