#!/usr/bin/env bash
umask 077

SUCCESS=0
RUNTIME_RESTORED=0
AGENT_WAS_ACTIVE=0
CONFIG_BACKUP=""
STATE_BACKUP=""
STATE_EXISTED=0

fail() {
    printf 'PHASE_63_COMMAND_DELIVERY_UPGRADE_ACCEPTANCE=FAIL\n' >&2
    printf 'REASON=%s\n' "$1" >&2
    if [[ -n "${EVIDENCE_DIR:-}" ]]; then
        printf 'EVIDENCE=%s\n' "$EVIDENCE_DIR" >&2
    fi
    exit 1
}

restore_runtime() {
    local mode="${1:-best-effort}"
    if [[ "$RUNTIME_RESTORED" -eq 1 ]]; then
        return 0
    fi
    if [[ -n "${AGENT_SERVICE:-}" ]]; then
        systemctl stop "$AGENT_SERVICE" >/dev/null 2>&1 || [[ "$mode" != strict ]] || return 1
    fi
    if [[ -n "$CONFIG_BACKUP" && -f "$CONFIG_BACKUP" && -n "${CONFIG_PATH:-}" ]]; then
        cp -a "$CONFIG_BACKUP" "$CONFIG_PATH" >/dev/null 2>&1 || [[ "$mode" != strict ]] || return 1
        cmp -s "$CONFIG_BACKUP" "$CONFIG_PATH" || [[ "$mode" != strict ]] || return 1
    elif [[ "$mode" == strict ]]; then
        return 1
    fi
    if [[ -n "${COMMAND_STATE_PATH:-}" ]]; then
        if [[ "$STATE_EXISTED" -eq 1 ]]; then
            [[ -n "$STATE_BACKUP" && -f "$STATE_BACKUP" ]] || [[ "$mode" != strict ]] || return 1
            cp -a "$STATE_BACKUP" "$COMMAND_STATE_PATH" >/dev/null 2>&1 || [[ "$mode" != strict ]] || return 1
            cmp -s "$STATE_BACKUP" "$COMMAND_STATE_PATH" || [[ "$mode" != strict ]] || return 1
        else
            rm -f "$COMMAND_STATE_PATH" >/dev/null 2>&1 || [[ "$mode" != strict ]] || return 1
        fi
    fi
    if [[ "$AGENT_WAS_ACTIVE" -eq 1 && -n "${AGENT_SERVICE:-}" ]]; then
        systemctl start "$AGENT_SERVICE" >/dev/null 2>&1 || [[ "$mode" != strict ]] || return 1
    fi
    [[ -z "$CONFIG_BACKUP" ]] || rm -f "$CONFIG_BACKUP" >/dev/null 2>&1 || true
    [[ -z "$STATE_BACKUP" ]] || rm -f "$STATE_BACKUP" >/dev/null 2>&1 || true
    RUNTIME_RESTORED=1
    return 0
}
trap restore_runtime EXIT INT TERM

require_command() {
    command -v "$1" >/dev/null 2>&1 || fail "missing_command_$1"
}

wait_for_service_active() {
    local service="$1"
    local attempts="$2"
    local attempt
    for ((attempt=0; attempt<attempts; ++attempt)); do
        if [[ "$(systemctl is-active "$service" 2>/dev/null || true)" == active ]]; then
            return 0
        fi
        sleep 1
    done
    return 1
}

agent_status() {
    "$ADMIN_BINARY" --database "$DATABASE" --backend "$BACKEND_ID" --status
}

command_status() {
    "$COMMAND_ADMIN_BINARY" --database "$DATABASE" --backend "$BACKEND_ID" --status
}

protected_identity_metadata() {
    python3 - "$IDENTITY_PATH" <<'PY_IDENTITY' || return 1
from pathlib import Path
import stat
import sys

path = Path(sys.argv[1])
metadata = path.lstat()
if stat.S_ISLNK(metadata.st_mode) or not stat.S_ISREG(metadata.st_mode):
    raise SystemExit(1)
if metadata.st_mode & (stat.S_IRWXG | stat.S_IRWXO):
    raise SystemExit(1)
values = {}
for line in path.read_text(encoding="utf-8").splitlines():
    if not line or line.startswith("#"):
        continue
    key, separator, value = line.partition("=")
    if not separator or not key or key in values:
        raise SystemExit(1)
    values[key] = value
for key in ("agent_id", "credential_id", "credential_generation"):
    if not values.get(key):
        raise SystemExit(1)
print(values["agent_id"] + "\t" + values["credential_id"] + "\t" + values["credential_generation"])
PY_IDENTITY
}

wait_for_agent_online() {
    local expected_agent_id="$1"
    local attempts="$2"
    local attempt status
    for ((attempt=0; attempt<attempts; ++attempt)); do
        status="$(agent_status 2>/dev/null || true)"
        if printf '%s\n' "$status" | python3 -c '
import json,sys
expected=sys.argv[1]
try: value=json.load(sys.stdin)
except Exception: raise SystemExit(1)
raise SystemExit(0 if (
    value.get("present") is True
    and value.get("state") == "online"
    and value.get("agentId") == expected
    and value.get("readOnly") is True
    and int(value.get("backendGeneration",0)) > 0
) else 1)
' "$expected_agent_id"; then
            printf '%s\n' "$status"
            return 0
        fi
        sleep 1
    done
    return 1
}

runtime_paths() {
    python3 - "$CONFIG_PATH" "$BACKEND_ID" <<'PY_PATHS' || return 1
from pathlib import Path
import stat
import sys

path=Path(sys.argv[1])
expected_backend=sys.argv[2]
metadata=path.lstat()
if stat.S_ISLNK(metadata.st_mode) or not stat.S_ISREG(metadata.st_mode):
    raise SystemExit(1)
values={}
for raw in path.read_text(encoding="utf-8").splitlines():
    line=raw.strip()
    if not line or line.startswith("#"):
        continue
    key,separator,value=line.partition("=")
    if not separator or not key or key in values:
        raise SystemExit(1)
    values[key]=value
identity=values.get("IDENTITY_PATH","")
backend=values.get("BACKEND_ID","")
state=values.get("COMMAND_STATE_PATH","/var/lib/vdr-suite/backend-agent/commands.state")
command_types=values.get("COMMAND_TYPES","")
if not identity.startswith("/") or not state.startswith("/") or backend != expected_backend:
    raise SystemExit(1)
if any(ord(character)<0x20 for character in identity+state+command_types):
    raise SystemExit(1)
print(identity+"\t"+state+"\t"+command_types)
PY_PATHS
}

configure_command_runtime() {
    python3 - "$CONFIG_PATH" <<'PY_CONFIG' || return 1
from pathlib import Path
import os
import stat
import sys

path=Path(sys.argv[1])
metadata=path.lstat()
if stat.S_ISLNK(metadata.st_mode) or not stat.S_ISREG(metadata.st_mode):
    raise SystemExit(1)
lines=path.read_text(encoding="utf-8").splitlines()
seen=set()
updated=[]
replaced=False
for raw in lines:
    stripped=raw.strip()
    if not stripped or stripped.startswith("#"):
        updated.append(raw)
        continue
    key,separator,value=raw.partition("=")
    if not separator or not key or key in seen:
        raise SystemExit(1)
    seen.add(key)
    if key == "COMMAND_TYPES":
        updated.append("COMMAND_TYPES=probe.noop")
        replaced=True
    else:
        updated.append(raw)
if not replaced:
    updated.append("COMMAND_TYPES=probe.noop")
temporary=path.with_name(path.name+".phase63-command.tmp")
fd=os.open(temporary,os.O_WRONLY|os.O_CREAT|os.O_EXCL,stat.S_IMODE(metadata.st_mode))
try:
    payload=("\n".join(updated)+"\n").encode()
    view=memoryview(payload)
    while view:
        written=os.write(fd,view)
        if written<=0: raise OSError("short write")
        view=view[written:]
    os.fsync(fd)
    os.fchmod(fd,stat.S_IMODE(metadata.st_mode))
    os.fchown(fd,metadata.st_uid,metadata.st_gid)
finally:
    os.close(fd)
os.replace(temporary,path)
directory=os.open(path.parent,os.O_RDONLY|os.O_DIRECTORY)
try: os.fsync(directory)
finally: os.close(directory)
PY_CONFIG
}

vdr_fingerprint() {
    local output="$1"
    python3 - "$VDR_VIDEO_DIR" "$output" <<'PY_FINGERPRINT' || return 1
from pathlib import Path
import hashlib
import sys

video=Path(sys.argv[1])
output=Path(sys.argv[2])
files=[
    Path("/var/lib/vdr/channels.conf"),
    Path("/var/lib/vdr/timers.conf"),
    Path("/var/lib/vdr/setup.conf"),
    Path("/var/lib/vdr/remote.conf"),
    Path("/var/lib/vdr/plugins/epgsearch/epgsearch.conf"),
]
lines=[]
for path in files:
    if path.is_file():
        lines.append(f"file\t{path}\t{hashlib.sha256(path.read_bytes()).hexdigest()}")
if video.is_dir():
    recordings=sorted(str(path.relative_to(video)) for path in video.rglob("*.rec") if path.is_dir())
    digest=hashlib.sha256("\n".join(recordings).encode()).hexdigest()
    lines.append(f"recording-directories\t{video}\t{digest}\t{len(recordings)}")
output.write_text("\n".join(lines)+"\n",encoding="utf-8")
PY_FINGERPRINT
}

enqueue_probe() {
    local attempts="$1"
    local attempt output
    for ((attempt=0; attempt<attempts; ++attempt)); do
        output="$("$COMMAND_ADMIN_BINARY" --database "$DATABASE" --backend "$BACKEND_ID" --enqueue-probe --deadline-seconds 600 2>/dev/null || true)"
        if printf '%s\n' "$output" | python3 -c '
import json,sys
try: value=json.load(sys.stdin)
except Exception: raise SystemExit(1)
command=value.get("commandId","")
raise SystemExit(0 if isinstance(command,str) and command.startswith("cmd_") else 1)
'; then
            printf '%s\n' "$output"
            return 0
        fi
        sleep 1
    done
    return 1
}

command_id_from_json() {
    python3 -c 'import json,sys; print(json.load(sys.stdin)["commandId"])'
}

command_counts() {
    python3 -c '
import json,sys
value=json.load(sys.stdin)
print("%s\t%d\t%d\t%d" % (
 value.get("commandId",""),
 int(value.get("deliveryCount",0)),
 int(value.get("receiptReplayCount",0)),
 int(value.get("resultReplayCount",0))))
'
}

wait_for_command() {
    local expected_id="$1"
    local minimum_delivery="$2"
    local minimum_receipt_replay="$3"
    local minimum_result_replay="$4"
    local attempts="$5"
    local attempt status
    for ((attempt=0; attempt<attempts; ++attempt)); do
        status="$(command_status 2>/dev/null || true)"
        if printf '%s\n' "$status" | python3 -c '
import json,sys
expected=sys.argv[1]
minimums=[int(value) for value in sys.argv[2:5]]
try: value=json.load(sys.stdin)
except Exception: raise SystemExit(1)
actual=[int(value.get("deliveryCount",0)),int(value.get("receiptReplayCount",0)),int(value.get("resultReplayCount",0))]
raise SystemExit(0 if (
 value.get("present") is True
 and value.get("commandId") == expected
 and value.get("commandType") == "probe.noop"
 and value.get("state") == "completed"
 and value.get("resultCategory") == "succeeded"
 and value.get("dispatchState") == "effect_reported"
 and value.get("verificationState") == "not_required"
 and all(current>=minimum for current,minimum in zip(actual,minimums))
) else 1)
' "$expected_id" "$minimum_delivery" "$minimum_receipt_replay" "$minimum_result_replay"; then
            printf '%s\n' "$status"
            return 0
        fi
        sleep 1
    done
    return 1
}

[[ "${EUID:-$(id -u)}" -eq 0 ]] || fail root_required

EXPECTED_BRANCH="${PHASE63_EXPECTED_BRANCH:-}"
EXPECTED_HEAD="${PHASE63_EXPECTED_HEAD:-}"
EVIDENCE_DIR="${PHASE63_EVIDENCE_DIR:-}"
BACKEND_ID="${PHASE63_BACKEND_ID:-default}"
DATABASE="${PHASE63_DATABASE:-/var/lib/vdr-suite/vdr-suite.db}"
VDR_VIDEO_DIR="${PHASE63_VDR_VIDEO_DIR:-/srv/vdr/video.00}"
CONFIG_PATH="${PHASE63_AGENT_CONFIG_PATH:-/etc/vdr-suite/backend-agent.conf}"
DAEMON_SERVICE="${PHASE63_DAEMON_SERVICE:-vdr-suite-daemon}"
AGENT_SERVICE="${PHASE63_AGENT_SERVICE:-vdr-suite-backend-agent}"
VDR_SERVICE="${PHASE63_VDR_SERVICE:-vdr}"
DAEMON_BINARY="/usr/sbin/vdr-suite-daemon"
AGENT_BINARY="/usr/sbin/vdr-suite-backend-agent"
ENROLL_BINARY="/usr/sbin/vdr-suite-backend-agent-enroll"
ADMIN_BINARY="/usr/sbin/vdr-suite-backend-agent-admin"
COMMAND_ADMIN_BINARY="/usr/sbin/vdr-suite-backend-agent-command-admin"
START_TIME="$(date --iso-8601=seconds)"

[[ -n "$EXPECTED_BRANCH" ]] || fail expected_branch_required
[[ -n "$EXPECTED_HEAD" ]] || fail expected_head_required
[[ -n "$EVIDENCE_DIR" ]] || fail evidence_directory_required
[[ ! -e "$EVIDENCE_DIR" ]] || fail evidence_directory_already_exists
[[ -f "$CONFIG_PATH" ]] || fail agent_configuration_missing
[[ -f "$DATABASE" ]] || fail database_missing
for command in git make cmp python3 systemctl journalctl sha256sum mktemp stat; do
    require_command "$command"
done

CURRENT_BRANCH="$(git branch --show-current)" || fail branch_read_failed
CURRENT_HEAD="$(git rev-parse HEAD)" || fail head_read_failed
[[ "$CURRENT_BRANCH" == "$EXPECTED_BRANCH" ]] || fail unexpected_branch
[[ "$CURRENT_HEAD" == "$EXPECTED_HEAD" ]] || fail unexpected_head
[[ -z "$(git status --porcelain)" ]] || fail worktree_not_clean

make daemon backend-agent backend-agent-enrollment backend-agent-admin backend-agent-command-admin >/dev/null || fail candidate_binary_build_failed
for pair in \
    ".build/vdr-suite-daemon:$DAEMON_BINARY" \
    ".build/vdr-suite-backend-agent:$AGENT_BINARY" \
    ".build/vdr-suite-backend-agent-enroll:$ENROLL_BINARY" \
    ".build/vdr-suite-backend-agent-admin:$ADMIN_BINARY" \
    ".build/vdr-suite-backend-agent-command-admin:$COMMAND_ADMIN_BINARY"; do
    candidate="${pair%%:*}"
    installed="${pair#*:}"
    [[ -x "$candidate" && -x "$installed" ]] || fail candidate_binary_missing
    cmp -s "$candidate" "$installed" || fail "installed_candidate_mismatch_$(basename "$installed")"
done

[[ "$(systemctl is-active "$VDR_SERVICE" 2>/dev/null || true)" == active ]] || fail vdr_not_active_before_acceptance
[[ "$(systemctl is-active "$DAEMON_SERVICE" 2>/dev/null || true)" == active ]] || fail daemon_not_active_before_acceptance
[[ "$(systemctl is-active "$AGENT_SERVICE" 2>/dev/null || true)" == active ]] || fail agent_not_active_before_acceptance
AGENT_WAS_ACTIVE=1

mkdir -m 0700 "$EVIDENCE_DIR" || fail evidence_directory_create_failed
printf '%s\n' "$CURRENT_HEAD" >"$EVIDENCE_DIR/HEAD" || fail evidence_head_write_failed
sha256sum "$DAEMON_BINARY" "$AGENT_BINARY" "$ENROLL_BINARY" "$ADMIN_BINARY" "$COMMAND_ADMIN_BINARY" >"$EVIDENCE_DIR/installed-binaries.sha256" || fail evidence_binary_hash_failed

ORIGINAL_STATUS="$(agent_status)" || fail initial_agent_status_failed
printf '%s\n' "$ORIGINAL_STATUS" >"$EVIDENCE_DIR/status.original.json" || fail initial_agent_status_write_failed
ORIGINAL_METADATA="$(runtime_paths)" || fail runtime_paths_invalid
IFS=$'\t' read -r IDENTITY_PATH COMMAND_STATE_PATH ORIGINAL_COMMAND_TYPES <<<"$ORIGINAL_METADATA"
[[ -z "$ORIGINAL_COMMAND_TYPES" ]] || fail command_runtime_already_enabled
ORIGINAL_IDENTITY="$(protected_identity_metadata)" || fail protected_identity_required
IFS=$'\t' read -r ORIGINAL_AGENT_ID ORIGINAL_CREDENTIAL_ID ORIGINAL_CREDENTIAL_GENERATION <<<"$ORIGINAL_IDENTITY"
ONLINE_STATUS="$(wait_for_agent_online "$ORIGINAL_AGENT_ID" 60)" || fail existing_agent_not_online
printf '%s\n' "$ONLINE_STATUS" >"$EVIDENCE_DIR/status.before.json" || fail status_before_write_failed
ORIGINAL_BACKEND_GENERATION="$(printf '%s\n' "$ONLINE_STATUS" | python3 -c 'import json,sys;print(int(json.load(sys.stdin)["backendGeneration"]))')" || fail backend_generation_read_failed
vdr_fingerprint "$EVIDENCE_DIR/vdr-state.before" || fail vdr_fingerprint_before_failed
command_status >"$EVIDENCE_DIR/command-status.before.json" || fail command_status_before_failed

CONFIG_BACKUP="$(mktemp /root/vdr-suite-phase63-command-config.XXXXXX)" || fail config_backup_create_failed
cp -a "$CONFIG_PATH" "$CONFIG_BACKUP" || fail config_backup_failed
if [[ -e "$COMMAND_STATE_PATH" ]]; then
    [[ -f "$COMMAND_STATE_PATH" && ! -L "$COMMAND_STATE_PATH" ]] || fail unsafe_existing_command_state
    STATE_BACKUP="$(mktemp /root/vdr-suite-phase63-command-state.XXXXXX)" || fail state_backup_create_failed
    cp -a "$COMMAND_STATE_PATH" "$STATE_BACKUP" || fail state_backup_failed
    STATE_EXISTED=1
fi

systemctl stop "$AGENT_SERVICE" || fail agent_stop_failed
configure_command_runtime || fail command_runtime_configuration_failed
rm -f "$COMMAND_STATE_PATH" || fail command_state_reset_failed
systemctl start "$AGENT_SERVICE" || fail agent_start_failed
wait_for_service_active "$AGENT_SERVICE" 60 || fail agent_not_active_after_enablement
ENABLED_STATUS="$(wait_for_agent_online "$ORIGINAL_AGENT_ID" 90)" || fail agent_not_online_after_enablement
printf '%s\n' "$ENABLED_STATUS" >"$EVIDENCE_DIR/status.enabled.json" || fail enabled_status_write_failed
ENABLED_IDENTITY="$(protected_identity_metadata)" || fail enabled_identity_read_failed
[[ "$ENABLED_IDENTITY" == "$ORIGINAL_IDENTITY" ]] || fail agent_identity_or_credential_changed_after_enablement

BASELINE_ASSIGNMENT="$(enqueue_probe 120)" || fail command_capability_or_assignment_not_available
printf '%s\n' "$BASELINE_ASSIGNMENT" >"$EVIDENCE_DIR/command.baseline.assignment.json" || fail baseline_assignment_write_failed
BASELINE_ID="$(printf '%s\n' "$BASELINE_ASSIGNMENT" | command_id_from_json)" || fail baseline_command_id_failed
BASELINE_STATUS="$(wait_for_command "$BASELINE_ID" 1 0 0 180)" || fail baseline_command_not_completed
printf '%s\n' "$BASELINE_STATUS" >"$EVIDENCE_DIR/command.baseline.completed.json" || fail baseline_status_write_failed
IFS=$'\t' read -r _ BASELINE_DELIVERY BASELINE_RECEIPT_REPLAY BASELINE_RESULT_REPLAY <<<"$(printf '%s\n' "$BASELINE_STATUS" | command_counts)"

"$COMMAND_ADMIN_BINARY" --database "$DATABASE" --backend "$BACKEND_ID" --replay "$BASELINE_ID" >"$EVIDENCE_DIR/command.replay.request.json" || fail explicit_replay_request_failed
REPLAY_STATUS="$(wait_for_command "$BASELINE_ID" "$((BASELINE_DELIVERY+1))" "$((BASELINE_RECEIPT_REPLAY+1))" "$((BASELINE_RESULT_REPLAY+1))" 180)" || fail equivalent_command_replay_not_observed
printf '%s\n' "$REPLAY_STATUS" >"$EVIDENCE_DIR/command.replay.completed.json" || fail replay_status_write_failed

"$COMMAND_ADMIN_BINARY" --database "$DATABASE" --backend "$BACKEND_ID" --arm-lost-receipt-response >"$EVIDENCE_DIR/fault.lost-receipt.json" || fail lost_receipt_fault_arm_failed
RECEIPT_ASSIGNMENT="$(enqueue_probe 120)" || fail lost_receipt_assignment_failed
printf '%s\n' "$RECEIPT_ASSIGNMENT" >"$EVIDENCE_DIR/command.lost-receipt.assignment.json" || fail lost_receipt_assignment_write_failed
RECEIPT_ID="$(printf '%s\n' "$RECEIPT_ASSIGNMENT" | command_id_from_json)" || fail lost_receipt_command_id_failed
RECEIPT_STATUS="$(wait_for_command "$RECEIPT_ID" 1 1 0 240)" || fail lost_receipt_response_not_recovered
printf '%s\n' "$RECEIPT_STATUS" >"$EVIDENCE_DIR/command.lost-receipt.completed.json" || fail lost_receipt_status_write_failed

"$COMMAND_ADMIN_BINARY" --database "$DATABASE" --backend "$BACKEND_ID" --arm-lost-result-response >"$EVIDENCE_DIR/fault.lost-result.json" || fail lost_result_fault_arm_failed
RESULT_ASSIGNMENT="$(enqueue_probe 120)" || fail lost_result_assignment_failed
printf '%s\n' "$RESULT_ASSIGNMENT" >"$EVIDENCE_DIR/command.lost-result.assignment.json" || fail lost_result_assignment_write_failed
RESULT_ID="$(printf '%s\n' "$RESULT_ASSIGNMENT" | command_id_from_json)" || fail lost_result_command_id_failed
RESULT_STATUS="$(wait_for_command "$RESULT_ID" 1 0 1 240)" || fail lost_result_response_not_recovered
printf '%s\n' "$RESULT_STATUS" >"$EVIDENCE_DIR/command.lost-result.completed.json" || fail lost_result_status_write_failed
IFS=$'\t' read -r _ RESULT_DELIVERY RESULT_RECEIPT_REPLAY RESULT_RESULT_REPLAY <<<"$(printf '%s\n' "$RESULT_STATUS" | command_counts)"

systemctl restart "$DAEMON_SERVICE" || fail daemon_restart_failed
wait_for_service_active "$DAEMON_SERVICE" 60 || fail daemon_not_active_after_restart
DAEMON_RESTART_STATUS="$(wait_for_command "$RESULT_ID" "$RESULT_DELIVERY" "$RESULT_RECEIPT_REPLAY" "$RESULT_RESULT_REPLAY" 60)" || fail command_state_not_persisted_across_daemon_restart
printf '%s\n' "$DAEMON_RESTART_STATUS" >"$EVIDENCE_DIR/command.after-daemon-restart.json" || fail daemon_restart_status_write_failed

PRE_AGENT_RESTART_STATUS="$(command_status)" || fail pre_agent_restart_command_status_failed
IFS=$'\t' read -r PRE_RESTART_ID PRE_RESTART_DELIVERY PRE_RESTART_RECEIPT PRE_RESTART_RESULT <<<"$(printf '%s\n' "$PRE_AGENT_RESTART_STATUS" | command_counts)"
systemctl restart "$AGENT_SERVICE" || fail agent_restart_failed
wait_for_service_active "$AGENT_SERVICE" 60 || fail agent_not_active_after_restart
RESTARTED_STATUS="$(wait_for_agent_online "$ORIGINAL_AGENT_ID" 120)" || fail agent_not_online_after_restart
printf '%s\n' "$RESTARTED_STATUS" >"$EVIDENCE_DIR/status.after-agent-restart.json" || fail restarted_status_write_failed
RESTARTED_IDENTITY="$(protected_identity_metadata)" || fail restarted_identity_read_failed
[[ "$RESTARTED_IDENTITY" == "$ORIGINAL_IDENTITY" ]] || fail agent_identity_or_credential_changed_after_restart
RESTARTED_BACKEND_GENERATION="$(printf '%s\n' "$RESTARTED_STATUS" | python3 -c 'import json,sys;print(int(json.load(sys.stdin)["backendGeneration"]))')" || fail restarted_backend_generation_read_failed
[[ "$RESTARTED_BACKEND_GENERATION" -gt "$ORIGINAL_BACKEND_GENERATION" ]] || fail backend_generation_not_advanced_after_agent_restart
for ((attempt=0; attempt<120; ++attempt)); do
    [[ ! -e "$COMMAND_STATE_PATH" ]] && break
    sleep 1
done
[[ ! -e "$COMMAND_STATE_PATH" ]] || fail acknowledged_stale_command_state_not_retired
POST_RESTART_OLD_STATUS="$(wait_for_command "$PRE_RESTART_ID" "$PRE_RESTART_DELIVERY" "$PRE_RESTART_RECEIPT" "$PRE_RESTART_RESULT" 60)" || fail stale_generation_command_state_changed
printf '%s\n' "$POST_RESTART_OLD_STATUS" >"$EVIDENCE_DIR/command.old-generation-after-restart.json" || fail old_generation_status_write_failed

RESTART_ASSIGNMENT="$(enqueue_probe 120)" || fail post_restart_assignment_failed
printf '%s\n' "$RESTART_ASSIGNMENT" >"$EVIDENCE_DIR/command.after-restart.assignment.json" || fail restart_assignment_write_failed
RESTART_ID="$(printf '%s\n' "$RESTART_ASSIGNMENT" | command_id_from_json)" || fail restart_command_id_failed
RESTART_COMMAND_STATUS="$(wait_for_command "$RESTART_ID" 1 0 0 180)" || fail post_restart_command_not_completed
printf '%s\n' "$RESTART_COMMAND_STATUS" >"$EVIDENCE_DIR/command.after-restart.completed.json" || fail restart_command_status_write_failed

vdr_fingerprint "$EVIDENCE_DIR/vdr-state.after" || fail vdr_fingerprint_after_failed
cmp -s "$EVIDENCE_DIR/vdr-state.before" "$EVIDENCE_DIR/vdr-state.after" || fail vdr_native_state_changed

journalctl -u "$AGENT_SERVICE" --since "$START_TIME" --no-pager >"$EVIDENCE_DIR/backend-agent.journal.log" || fail agent_journal_capture_failed
journalctl -u "$DAEMON_SERVICE" --since "$START_TIME" --no-pager >"$EVIDENCE_DIR/daemon.journal.log" || fail daemon_journal_capture_failed
python3 tools/check_phase63_runtime_evidence_secrets.py "$EVIDENCE_DIR" >"$EVIDENCE_DIR/evidence-secret-scan.txt" || fail secret_like_material_found_in_evidence_logs

restore_runtime strict || fail original_runtime_restoration_failed
wait_for_service_active "$AGENT_SERVICE" 60 || fail agent_not_active_after_restoration
RESTORED_STATUS="$(wait_for_agent_online "$ORIGINAL_AGENT_ID" 120)" || fail agent_not_online_after_restoration
printf '%s\n' "$RESTORED_STATUS" >"$EVIDENCE_DIR/status.restored.json" || fail restored_status_write_failed
RESTORED_IDENTITY="$(protected_identity_metadata)" || fail restored_identity_read_failed
[[ "$RESTORED_IDENTITY" == "$ORIGINAL_IDENTITY" ]] || fail agent_identity_or_credential_changed_after_restoration
[[ "$(systemctl is-active "$VDR_SERVICE" 2>/dev/null || true)" == active ]] || fail vdr_not_active_after_acceptance
[[ "$(systemctl is-active "$DAEMON_SERVICE" 2>/dev/null || true)" == active ]] || fail daemon_not_active_after_acceptance
[[ "$(systemctl is-active "$AGENT_SERVICE" 2>/dev/null || true)" == active ]] || fail agent_not_active_after_acceptance

SUCCESS=1
printf 'PHASE_63_COMMAND_DELIVERY_UPGRADE_ACCEPTANCE=PASS\n'
printf 'HEAD=%s\n' "$CURRENT_HEAD"
printf 'AGENT_ID=%s\n' "$ORIGINAL_AGENT_ID"
printf 'BASELINE_COMMAND_COMPLETED=yes\n'
printf 'COMMAND_REPLAY=yes\n'
printf 'LOST_RECEIPT_RESPONSE_RECOVERED=yes\n'
printf 'LOST_RESULT_RESPONSE_RECOVERED=yes\n'
printf 'DAEMON_RESTART_PERSISTED=yes\n'
printf 'AGENT_RESTART_RECOVERED=yes\n'
printf 'STALE_GENERATION_COMMAND_NOT_REPLAYED=yes\n'
printf 'EXISTING_AGENT_IDENTITY_PRESERVED=yes\n'
printf 'CREDENTIAL_GENERATION_PRESERVED=yes\n'
printf 'VDR_NATIVE_STATE_UNCHANGED=yes\n'
printf 'ORIGINAL_CONFIGURATION_RESTORED=yes\n'
printf 'VDR_ACTIVE=yes\n'
printf 'DAEMON_ACTIVE=yes\n'
printf 'AGENT_ACTIVE=yes\n'
printf 'EVIDENCE=%s\n' "$EVIDENCE_DIR"
