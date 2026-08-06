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
agent_command = text("core/agent/src/BackendAgentCommand.cpp")
transport_header = text("core/agent/include/SuiteBridgeSvdrpTransport.h")
agent_main = text("apps/agent/main.cpp")
plugin_header = text("vdr-plugin-suite-bridge/suitebridge_native_probe.h")
plugin_runtime = text("vdr-plugin-suite-bridge/suitebridge_native_probe.cpp")
plugin_svdrp = text("vdr-plugin-suite-bridge/suitebridge_svdrp.cpp")
packaged_config = text("packaging/systemd/backend-agent.conf")
acceptance = text("tools/run_phase63_fenced_native_operation_acceptance.sh")

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
if "sqlite3" in acceptance:
    errors.append("acceptance must not use manual SQLite inspection")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("Phase-63 fenced native operation runtime guard passed")
