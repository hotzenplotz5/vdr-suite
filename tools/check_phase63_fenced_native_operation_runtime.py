#!/usr/bin/env python3
"""Static boundary guard for the Phase-63 fenced native probe runtime."""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
PROVIDER_SELECTION_RUNTIME = (
    ROOT / "docs/development/phase-63-local-provider-selection-runtime.md"
).is_file()

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
native_handler_path = ROOT / "core/agent/src/BackendAgentNativeProbeCommandHandler.cpp"
native_handler = (
    native_handler_path.read_text(encoding="utf-8")
    if native_handler_path.is_file()
    else ""
)
native_command_runtime = (
    agent_client + "\n" + native_handler
    if native_handler
    else agent_client
)
state_store_path = ROOT / "core/agent/src/BackendAgentCommandStateStore.cpp"
state_store = (
    state_store_path.read_text(encoding="utf-8")
    if state_store_path.is_file()
    else ""
)
native_state_owner = state_store if state_store else agent_client
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
agent_sources = text("mk/agent-sources.mk")
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

agent_epoch_fence_token = (
    "native probe selected provider cannot be replayed"
    if PROVIDER_SELECTION_RUNTIME
    else "native probe old plugin epoch cannot be replayed"
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
    "agent native command runtime": (
        native_command_runtime,
        [
            "state.dispatchState",
            "native_probe_dispatch_reconciliation_required",
            agent_epoch_fence_token,
        ],
    ),
    "agent native state owner": (
        native_state_owner,
        [
            "native_capability_evidence",
            "native_receipt_evidence",
            "native_result_evidence",
            "native_readback_evidence",
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

if native_handler:
    for token in [
        '#include "BackendAgentNativeProbeCommandHandler.h"',
        "backendAgentNativeProbeCommandAvailability",
        "backendAgentNativeProbeCommandReconcile",
        "backendAgentNativeProbeCommandSetDefaultTransport",
    ]:
        if token not in agent_client:
            errors.append(
                "Native Probe CommandClient handoff missing token: " + token
            )
    for token in [
        "executeNativeProbe(request)",
        "readNativeProbe(readbackRequest)",
        "backendAgentNativeProbeSelectionMatchesCapability",
        "native_probe_dispatch_reconciliation_required",
        agent_epoch_fence_token,
    ]:
        if token not in native_handler:
            errors.append(
                "Native Probe handler missing bounded runtime token: " + token
            )
    for token in [
        "executeNativeProbe(request)",
        "readNativeProbe(readbackRequest)",
        "backendAgentNativeProbeParseEvidence",
        "backendAgentNativeProbeSelectionMatchesCapability",
    ]:
        if token in agent_client:
            errors.append(
                "Native Probe execution ownership leaked back into CommandClient: "
                + token
            )
    for token in [
        "IBackendAgentControlPlaneTransport",
        "/api/agent/v1/commands/receipt",
        "/api/agent/v1/commands/result",
        "sendReceipt(",
        "sendResult(",
        "BackendAgentNativeTimerDelete",
        "vdr.timer.delete",
    ]:
        if token in native_handler:
            errors.append(
                "Native Probe handler crossed its bounded ownership boundary: "
                + token
            )
    if agent_sources.count(
            "core/agent/src/BackendAgentNativeProbeCommandHandler.cpp") != 1:
        errors.append(
            "Native Probe command handler must occur exactly once in Agent sources"
        )
    if "AGENT_NATIVE_PROBE_COMMAND_HANDLER_SRC :=" not in agent_sources:
        errors.append("Native Probe command handler source set missing")

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
expected_command_begin_count = 5 if PROVIDER_SELECTION_RUNTIME else 4
expected_command_lease_count = 7 if PROVIDER_SELECTION_RUNTIME else 4

if command_begin_count != expected_command_begin_count:
    errors.append(
        "command repository explicit transaction count changed unexpectedly: "
        f"expected {expected_command_begin_count}, got {command_begin_count}"
    )

if command_lease_count != expected_command_lease_count:
    errors.append(
        "command repository transaction/provider lease count changed unexpectedly: "
        f"expected {expected_command_lease_count}, got {command_lease_count}"
    )

if command_lease_count < command_begin_count:
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

native_dispatch_owner = native_handler if native_handler else agent_client
execute = native_dispatch_owner.find("executeNativeProbe(request)")
starting_matches = list(re.finditer(
    r'state\.dispatchState\s*=\s*"starting"',
    native_dispatch_owner[:execute] if execute >= 0 else "",
))
starting = starting_matches[-1].start() if starting_matches else -1
persist_needle = (
    "persist(statePath, state, reason)"
    if native_handler
    else "persist(config.statePath, state, reason)"
)
persist_starting = native_dispatch_owner.find(
    persist_needle, starting, execute)
if min(starting, persist_starting, execute) < 0 or not (
    starting < persist_starting < execute
):
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
if "COMMAND_TYPES=vdr.timer.create,vdr.timer.update,vdr.timer.toggle,vdr.timer.delete\n" not in packaged_config:
    errors.append("packaged Timer command activation must remain exact")

scoped_runtime = "\n".join([
    agent_protocol,
    agent_client,
    native_handler,
    state_store,
    agent_command_json,
    transport_header,
    plugin_header,
    plugin_runtime,
    agent_main,
    text("core/agent/src/BackendAgentNativeProbeDelivery.cpp"),
])
if "mutations=enabled" in scoped_runtime or '"enabled"' in scoped_runtime:
    errors.append("mutations=enabled is forbidden")

# Phase 64 may classify already-validated Timer command state and capability
# availability in the generic Agent command owner. These boolean discriminators
# carry no mutation authority. Strip only their exact declarations and bounded
# fail-closed availability state from the old Phase-63 mutation-name heuristic.
# The bounded post-Phase-66 Recording-marks successor is independently guarded
# by check_recording_native_editing_runtime_wiring.py; its exact
# recordingMarksModify* state names are therefore excluded from this legacy
# name heuristic without weakening the Phase-63 native-probe contract itself.
allowed_timer_discriminators = (
    (
        "Timer-create",
        (
            "const bool timerCreateCommand = state.assignment.commandType ==\n"
            "        vdrsuite::agent::kBackendAgentNativeTimerCreateCommandType;"
        ),
        (
            "prepareFreshNativeTimerCreateLocalStarting",
            "executeFreshNativeTimerCreateAndPersistOutcome",
        ),
    ),
    (
        "Timer-delete",
        (
            "const bool timerDeleteCommand = state.assignment.commandType ==\n"
            "        vdrsuite::agent::kBackendAgentNativeTimerDeleteCommandType;"
        ),
        (
            "prepareFreshNativeTimerDeleteLocalStarting",
        ),
    ),
    (
        "Timer-modify",
        (
            "const bool timerModifyCommand =\n"
            "        state.assignment.commandType ==\n"
            "            vdrsuite::agent::kBackendAgentNativeTimerUpdateCommandType ||\n"
            "        state.assignment.commandType ==\n"
            "            vdrsuite::agent::kBackendAgentNativeTimerToggleCommandType;"
        ),
        (
            "prepareFreshNativeTimerModifyLocalStarting",
            "executeFreshNativeTimerModifyAndPersistOutcome",
        ),
    ),
)

scoped_runtime_boundary = scoped_runtime

timer_availability_discriminator = (
    "const bool timerType =\n"
    "            type == "
    "vdrsuite::agent::kBackendAgentNativeTimerCreateCommandType ||\n"
    "            type == "
    "vdrsuite::agent::kBackendAgentNativeTimerDeleteCommandType ||\n"
    "            type == "
    "vdrsuite::agent::kBackendAgentNativeTimerUpdateCommandType ||\n"
    "            type == "
    "vdrsuite::agent::kBackendAgentNativeTimerToggleCommandType;"
)
timer_snapshot_initial = "bool timerSnapshotCoherent = true;"
timer_snapshot_failed = "timerSnapshotCoherent = false;"
timer_availability_start = agent_client.find(
    "CommandAvailability availableCommands("
)
timer_availability_end = agent_client.find(
    "\nbool reconcileBackendAgentCommandState(",
    timer_availability_start,
)
timer_availability_runtime = (
    agent_client[timer_availability_start:timer_availability_end]
    if timer_availability_start >= 0
    and timer_availability_end > timer_availability_start
    else ""
)
timer_availability_tokens = (
    "if (!transport->discoverProvider(facts, reason))",
    "catch (...)",
    "if (supported.empty())",
    "if (!mergeProviderFacts(timerProviders, facts))",
    "if (timerSnapshotCoherent &&",
    "availability.localProviders.clear();",
)

if (
    agent_client.count(timer_availability_discriminator) != 1
    or timer_availability_runtime.count(timer_snapshot_initial) != 1
    or timer_availability_runtime.count(timer_snapshot_failed) != 5
    or any(
        token not in timer_availability_runtime
        for token in timer_availability_tokens
    )
):
    errors.append(
        "Timer availability discriminator must remain a single bounded "
        "fail-closed Phase-64 capability handoff"
    )
else:
    scoped_runtime_boundary = scoped_runtime_boundary.replace(
        timer_availability_discriminator, "", 1
    )
    scoped_runtime_boundary = scoped_runtime_boundary.replace(
        timer_snapshot_initial, "", 1
    )
    scoped_runtime_boundary = scoped_runtime_boundary.replace(
        timer_snapshot_failed, "", 5
    )

for label, discriminator, required_handoffs in allowed_timer_discriminators:
    if discriminator not in agent_client:
        continue

    if (
        agent_client.count(discriminator) != 1
        or any(
            handoff not in agent_client
            for handoff in required_handoffs
        )
    ):
        errors.append(
            f"{label} discriminator must remain a single bounded "
            "Phase-64 state handoff"
        )
        continue

    scoped_runtime_boundary = scoped_runtime_boundary.replace(
        discriminator, "", 1
    )

for pattern in [
    r"\b(?:system|popen|fork|execl|execv|posix_spawn)\s*\(",
    r"\b(?:timer|recording(?!MarksModify)|searchtimer|channel|epg|remote|osd)[A-Za-z_]*\s*=",
]:
    if re.search(pattern, scoped_runtime_boundary, flags=re.IGNORECASE):
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