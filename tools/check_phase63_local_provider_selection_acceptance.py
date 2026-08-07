#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
runner_path = ROOT / "tools/run_phase63_local_provider_selection_acceptance.sh"
if not runner_path.is_file():
    raise SystemExit("missing provider selection acceptance runner")
runner = runner_path.read_text(encoding="utf-8")

errors: list[str] = []

for forbidden in ["set -e", "set -u", "set -o pipefail"]:
    if forbidden in runner:
        errors.append(f"acceptance runner contains forbidden shell option: {forbidden}")

for token in [
    "[[ \"${EUID:-$(id -u)}\" -eq 0 ]]",
    "git branch --show-current",
    "git rev-parse HEAD",
    "git status --porcelain",
    "provider-ownership-status",
    "--set-native-probe-owner",
    "--clear-native-probe-owner",
    "preexisting_active_provider_ownership_not_supported",
    "OWNER_TOUCHED=0",
    "OWNER_TOUCHED=1",
    "trap cleanup EXIT",
    "trap 'exit 130' INT",
    "trap 'exit 143' TERM",
    "run_phase63_fenced_native_operation_acceptance.sh",
    "provider-selection-db.json",
    "backend_agent_command_provider_selections",
    "payload_version",
    "local-provider-selection/1|",
    "PHASE_63_FENCED_NATIVE_OPERATION_ACCEPTANCE=PASS",
    "CONTROL_PLANE_REPLAY_NO_NATIVE_REEXECUTION=yes",
    "PLUGIN_EPOCH_REPLAY_FENCED=yes",
    "VDR_NATIVE_STATE_UNCHANGED=yes",
    "EXISTING_AGENT_IDENTITY_PRESERVED=yes",
    "CREDENTIAL_GENERATION_PRESERVED=yes",
    "ORIGINAL_CONFIGURATION_RESTORED=yes",
    "PHASE_63_LOCAL_PROVIDER_SELECTION_ACCEPTANCE=PASS",
    "PROVIDER_OWNERSHIP_EXPLICIT=yes",
    "SELECTED_PROVIDER_ID=suitebridge:local",
    "NATIVE_PROBE_PAYLOAD_V2_PERSISTED=yes",
    "PROVIDER_SELECTION_SIDECAR_PERSISTED=yes",
    "PROVIDER_OWNERSHIP_RESTORED=inactive",
    "MUTATIONS_REMAIN_DISABLED=yes",
]:
    if token not in runner:
        errors.append(f"provider acceptance missing safety/evidence token: {token}")

set_owner = runner.find("--set-native-probe-owner")
nested = runner.find("run_phase63_fenced_native_operation_acceptance.sh")
db_verify = runner.find("provider-selection-db.json", nested)
clear_owner = runner.rfind("--clear-native-probe-owner")
final_pass = runner.find("PHASE_63_LOCAL_PROVIDER_SELECTION_ACCEPTANCE=PASS")
if min(set_owner, nested, db_verify, clear_owner, final_pass) < 0 or not (
    set_owner < nested < db_verify < clear_owner < final_pass
):
    errors.append(
        "provider ownership must wrap nested native acceptance and DB evidence verification"
    )

active_check = runner.find("preexisting_active_provider_ownership_not_supported")
if active_check < 0 or active_check > set_owner:
    errors.append("pre-existing active provider ownership must fail before mutation")

cleanup_start = runner.find("cleanup()")
cleanup_end = runner.find("trap cleanup EXIT", cleanup_start)
cleanup_section = runner[cleanup_start:cleanup_end]
for token in ["OWNER_TOUCHED", "build_admin", "--clear-native-probe-owner", "provider_inactive"]:
    if token not in cleanup_section:
        errors.append(f"provider cleanup missing token: {token}")

if re.search(r"(?m)^[ \t]*(?:command[ \t]+)?(?:/usr/bin/)?sqlite3(?:[ \t]|$)", runner):
    errors.append("acceptance must not depend on the sqlite3 CLI")
if "sqlite3.connect" not in runner:
    errors.append("acceptance must verify persisted provider selection through read-only Python SQLite")
if 'mode=ro' not in runner:
    errors.append("provider selection DB verification must be read-only")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("Phase-63 local provider selection acceptance guard passed")
