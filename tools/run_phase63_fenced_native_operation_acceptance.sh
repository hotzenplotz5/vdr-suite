#!/usr/bin/env bash
umask 077

SUCCESS=0
RESTORED=0
PROXY_PID=""
VDR_WAS_ACTIVE=0
DAEMON_WAS_ACTIVE=0
AGENT_WAS_ACTIVE=0
DROPIN_EXISTED=0
STATE_EXISTED=0

fail() {
    printf 'PHASE_63_FENCED_NATIVE_OPERATION_ACCEPTANCE=FAIL\n' >&2
    printf 'REASON=%s\n' "$1" >&2
    if [[ -n "${EVIDENCE_DIR:-}" ]]; then
        printf 'EVIDENCE=%s\n' "$EVIDENCE_DIR" >&2
    fi
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || fail "missing_command_$1"
}

wait_active() {
    local service="$1"
    local attempts="$2"
    local index
    for ((index=0; index<attempts; ++index)); do
        if [[ "$(systemctl is-active "$service" 2>/dev/null || true)" == active ]]; then
            return 0
        fi
        sleep 1
    done
    return 1
}

stop_proxy() {
    if [[ -n "$PROXY_PID" ]]; then
        printf 'stop\n' >"$PROXY_STOP" 2>/dev/null || true
        kill "$PROXY_PID" >/dev/null 2>&1 || true
        wait "$PROXY_PID" >/dev/null 2>&1 || true
        PROXY_PID=""
    fi
}

restore_runtime() {
    local mode="${1:-best-effort}"
    if [[ "$RESTORED" -eq 1 ]]; then
        return 0
    fi
    stop_proxy
    systemctl stop "$AGENT_SERVICE" >/dev/null 2>&1 || [[ "$mode" != strict ]] || return 1
    systemctl stop "$DAEMON_SERVICE" >/dev/null 2>&1 || [[ "$mode" != strict ]] || return 1
    systemctl stop "$VDR_SERVICE" >/dev/null 2>&1 || [[ "$mode" != strict ]] || return 1

    if [[ "$DROPIN_EXISTED" -eq 1 ]]; then
        cp -a "$BACKUP_DIR/agent-dropin.conf" "$DROPIN_PATH" >/dev/null 2>&1 || [[ "$mode" != strict ]] || return 1
        cmp -s "$BACKUP_DIR/agent-dropin.conf" "$DROPIN_PATH" || [[ "$mode" != strict ]] || return 1
    else
        rm -f "$DROPIN_PATH" >/dev/null 2>&1 || [[ "$mode" != strict ]] || return 1
    fi
    systemctl daemon-reload >/dev/null 2>&1 || [[ "$mode" != strict ]] || return 1

    local installed
    for installed in "$DAEMON_BINARY" "$AGENT_BINARY" "$ENROLL_BINARY" "$ADMIN_BINARY" "$COMMAND_ADMIN_BINARY" "$PLUGIN_INSTALLED"; do
        if [[ -f "$BACKUP_DIR$(printf '%s' "$installed")" ]]; then
            cp -a "$BACKUP_DIR$(printf '%s' "$installed")" "$installed" >/dev/null 2>&1 || [[ "$mode" != strict ]] || return 1
            cmp -s "$BACKUP_DIR$(printf '%s' "$installed")" "$installed" || [[ "$mode" != strict ]] || return 1
        elif [[ "$mode" == strict ]]; then
            return 1
        fi
    done

    if [[ "$STATE_EXISTED" -eq 1 ]]; then
        cp -a "$BACKUP_DIR/command-state" "$COMMAND_STATE_PATH" >/dev/null 2>&1 || [[ "$mode" != strict ]] || return 1
        cmp -s "$BACKUP_DIR/command-state" "$COMMAND_STATE_PATH" || [[ "$mode" != strict ]] || return 1
    else
        rm -f "$COMMAND_STATE_PATH" >/dev/null 2>&1 || [[ "$mode" != strict ]] || return 1
    fi

    if [[ "$VDR_WAS_ACTIVE" -eq 1 ]]; then
        systemctl start "$VDR_SERVICE" >/dev/null 2>&1 || [[ "$mode" != strict ]] || return 1
        wait_active "$VDR_SERVICE" 30 || [[ "$mode" != strict ]] || return 1
    fi
    if [[ "$DAEMON_WAS_ACTIVE" -eq 1 ]]; then
        systemctl start "$DAEMON_SERVICE" >/dev/null 2>&1 || [[ "$mode" != strict ]] || return 1
        wait_active "$DAEMON_SERVICE" 30 || [[ "$mode" != strict ]] || return 1
    fi
    if [[ "$AGENT_WAS_ACTIVE" -eq 1 ]]; then
        systemctl start "$AGENT_SERVICE" >/dev/null 2>&1 || [[ "$mode" != strict ]] || return 1
        wait_active "$AGENT_SERVICE" 30 || [[ "$mode" != strict ]] || return 1
    fi
    RESTORED=1
    return 0
}
trap restore_runtime EXIT INT TERM

protected_config_value() {
    python3 - "$1" "$2" <<'PY_CONFIG_VALUE' || return 1
from pathlib import Path
import stat
import sys
path=Path(sys.argv[1])
key=sys.argv[2]
metadata=path.lstat()
if stat.S_ISLNK(metadata.st_mode) or not stat.S_ISREG(metadata.st_mode):
    raise SystemExit(1)
values={}
for raw in path.read_text(encoding="utf-8").splitlines():
    line=raw.strip()
    if not line or line.startswith("#"):
        continue
    name,separator,value=line.partition("=")
    if not separator or not name or name in values:
        raise SystemExit(1)
    values[name]=value
print(values.get(key,""))
PY_CONFIG_VALUE
}

identity_metadata() {
    python3 - "$IDENTITY_PATH" <<'PY_IDENTITY' || return 1
from pathlib import Path
import stat
import sys
path=Path(sys.argv[1])
metadata=path.lstat()
if stat.S_ISLNK(metadata.st_mode) or not stat.S_ISREG(metadata.st_mode):
    raise SystemExit(1)
if metadata.st_mode & (stat.S_IRWXG | stat.S_IRWXO):
    raise SystemExit(1)
values={}
raw=path.read_bytes()
for line in raw.decode("utf-8").splitlines():
    if not line or line.startswith("#"):
        continue
    key,separator,value=line.partition("=")
    if not separator or not key or key in values:
        raise SystemExit(1)
    values[key]=value
for key in ("agent_id","credential_id","credential_generation"):
    if not values.get(key):
        raise SystemExit(1)
print("\t".join([
    values["agent_id"],values["credential_id"],
    values["credential_generation"]]))
PY_IDENTITY
}

vdr_fingerprint() {
    python3 - "$VDR_VIDEO_DIR" "$1" <<'PY_VDR_FINGERPRINT' || return 1
from pathlib import Path
import hashlib
import os
import sys
video=Path(sys.argv[1])
output=Path(sys.argv[2])
paths=[
 Path("/var/lib/vdr/channels.conf"),Path("/var/lib/vdr/timers.conf"),
 Path("/var/lib/vdr/setup.conf"),Path("/var/lib/vdr/remote.conf"),
 Path("/var/lib/vdr/plugins/epgsearch/epgsearch.conf"),
]
lines=[]
for path in paths:
    if path.is_file():
        lines.append(f"file\t{path}\t{hashlib.sha256(path.read_bytes()).hexdigest()}")
if video.is_dir():
    entries=[]
    for path in video.rglob("*"):
        try:
            relative=str(path.relative_to(video))
            metadata=path.stat(follow_symlinks=False)
        except OSError:
            raise SystemExit(1)
        if path.is_dir() and path.name.endswith(".rec"):
            entries.append(f"recording\t{relative}\t{metadata.st_mode}\t{metadata.st_size}")
        elif path.is_file() and path.name in {"info","resume","marks"}:
            entries.append(f"control\t{relative}\t{metadata.st_size}\t{hashlib.sha256(path.read_bytes()).hexdigest()}")
    lines.append("video-tree\t"+hashlib.sha256("\n".join(sorted(entries)).encode()).hexdigest())
output.write_text("\n".join(lines)+"\n",encoding="utf-8")
PY_VDR_FINGERPRINT
}

command_status() {
    "$COMMAND_ADMIN_BINARY" --database "$DATABASE" --backend "$BACKEND_ID" --status
}

agent_status() {
    "$ADMIN_BINARY" --database "$DATABASE" --backend "$BACKEND_ID" --status
}

wait_agent_online() {
    local expected_id="$1"
    local attempts="$2"
    local index status
    for ((index=0; index<attempts; ++index)); do
        status="$(agent_status 2>/dev/null || true)"
        if printf '%s\n' "$status" | python3 -c '
import json,sys
expected=sys.argv[1]
try: value=json.load(sys.stdin)
except Exception: raise SystemExit(1)
raise SystemExit(0 if value.get("present") is True and value.get("state")=="online" and value.get("agentId")==expected and value.get("readOnly") is True else 1)
' "$expected_id"; then
            printf '%s\n' "$status"
            return 0
        fi
        sleep 1
    done
    return 1
}

enqueue_native_probe() {
    local attempts="$1"
    local index output
    for ((index=0; index<attempts; ++index)); do
        output="$("$COMMAND_ADMIN_BINARY" --database "$DATABASE" --backend "$BACKEND_ID" --enqueue-native-probe --deadline-seconds 600 2>/dev/null || true)"
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

json_field() {
    python3 -c 'import json,sys; print(json.load(sys.stdin)[sys.argv[1]])' "$1"
}

wait_command_state() {
    local expected_id="$1"
    local expected_state="$2"
    local expected_result="$3"
    local expected_verification="$4"
    local minimum_delivery="$5"
    local attempts="$6"
    local index status
    for ((index=0; index<attempts; ++index)); do
        status="$(command_status 2>/dev/null || true)"
        if printf '%s\n' "$status" | python3 -c '
import json,sys
try: value=json.load(sys.stdin)
except Exception: raise SystemExit(1)
raise SystemExit(0 if (
 value.get("present") is True
 and value.get("commandId")==sys.argv[1]
 and value.get("commandType")=="vdr.native.probe"
 and value.get("state")==sys.argv[2]
 and value.get("resultCategory")==sys.argv[3]
 and value.get("verificationState")==sys.argv[4]
 and int(value.get("deliveryCount",0))>=int(sys.argv[5])
) else 1)
' "$expected_id" "$expected_state" "$expected_result" "$expected_verification" "$minimum_delivery"; then
            printf '%s\n' "$status"
            return 0
        fi
        sleep 1
    done
    return 1
}

state_value() {
    python3 - "$COMMAND_STATE_PATH" "$1" <<'PY_STATE_VALUE' || return 1
from pathlib import Path
import stat
import sys
path=Path(sys.argv[1])
key=sys.argv[2]
metadata=path.lstat()
if stat.S_ISLNK(metadata.st_mode) or not stat.S_ISREG(metadata.st_mode):
    raise SystemExit(1)
if metadata.st_mode & (stat.S_IRWXG | stat.S_IRWXO):
    raise SystemExit(1)
values={}
for line in path.read_text(encoding="utf-8").splitlines():
    name,separator,value=line.partition("=")
    if not separator or not name or name in values:
        raise SystemExit(1)
    values[name]=value
if key not in values:
    raise SystemExit(1)
print(values[key])
PY_STATE_VALUE
}

verify_separate_evidence() {
    python3 - "$1" "$2" "$3" <<'PY_EVIDENCE' || return 1
import json
import sys
receipt=json.loads(sys.argv[1])
result=json.loads(sys.argv[2])
readback=json.loads(sys.argv[3])
if "receiptCategory" not in receipt or "resultCategory" in receipt:
    raise SystemExit(1)
if "resultCategory" not in result or "receiptCategory" in result:
    raise SystemExit(1)
required={"commandId","requestFingerprint","nativeOperation","nativeOperationSchema","pluginInstanceEpoch","nativeExecutionSequence","vdrActive","mutationsState","sideEffectObserved","readbackCategory","duplicateDisposition"}
if set(readback) != required:
    raise SystemExit(1)
if readback["nativeOperation"]!="vdr.native.probe" or readback["nativeOperationSchema"]!=1 or readback["vdrActive"] is not True or readback["mutationsState"]!="disabled" or readback["sideEffectObserved"] is not False or readback["readbackCategory"]!="verified" or readback["duplicateDisposition"]!="exact_replay":
    raise SystemExit(1)
if receipt["nativeExecutionSequence"] != result["nativeExecutionSequence"] or result["nativeExecutionSequence"] != readback["nativeExecutionSequence"]:
    raise SystemExit(1)
PY_EVIDENCE
}

proxy_exec_summary() {
    python3 - "$PROXY_LOG" "$1" <<'PY_PROXY_SUMMARY' || return 1
import json
from pathlib import Path
import re
import sys
command=sys.argv[2]
records=[]
for raw in Path(sys.argv[1]).read_text(encoding="utf-8").splitlines():
    value=json.loads(raw)
    request=value.get("request","")
    if "NPROBE EXEC" not in request:
        continue
    fields=request.split()
    if len(fields)>8 and fields[7]==command:
        response=value.get("response","")
        payload=response[4:] if len(response)>=4 else ""
        try: evidence=json.loads(payload)
        except Exception: evidence={}
        records.append((value.get("event"),evidence.get("receiptCategory"),evidence.get("nativeExecutionSequence")))
print(json.dumps(records,separators=(",",":")))
PY_PROXY_SUMMARY
}

wait_proxy_drop() {
    local command_id="$1"
    local attempts="$2"
    local index summary
    for ((index=0; index<attempts; ++index)); do
        summary="$(proxy_exec_summary "$command_id" 2>/dev/null || true)"
        if printf '%s\n' "$summary" | python3 -c 'import json,sys; values=json.load(sys.stdin); raise SystemExit(0 if any(item[0]=="drop" for item in values) else 1)'; then
            return 0
        fi
        sleep 1
    done
    return 1
}

[[ "${EUID:-$(id -u)}" -eq 0 ]] || fail root_required

EXPECTED_BRANCH="${PHASE63_EXPECTED_BRANCH:-agent/phase63-fenced-native-operation-runtime}"
EXPECTED_HEAD="${PHASE63_EXPECTED_HEAD:-}"
BACKEND_ID="${PHASE63_BACKEND_ID:-default}"
DATABASE="${PHASE63_DATABASE:-/var/lib/vdr-suite/vdr-suite.db}"
CONFIG_PATH="${PHASE63_AGENT_CONFIG_PATH:-/etc/vdr-suite/backend-agent.conf}"
VDR_VIDEO_DIR="${PHASE63_VDR_VIDEO_DIR:-/srv/vdr/video.00}"
VDR_SERVICE="${PHASE63_VDR_SERVICE:-vdr}"
DAEMON_SERVICE="${PHASE63_DAEMON_SERVICE:-vdr-suite-daemon}"
AGENT_SERVICE="${PHASE63_AGENT_SERVICE:-vdr-suite-backend-agent}"
SVDRP_PORT="${PHASE63_SVDRP_PORT:-6419}"
PROXY_PORT="${PHASE63_PROXY_PORT:-16419}"
DAEMON_BINARY="/usr/sbin/vdr-suite-daemon"
AGENT_BINARY="/usr/sbin/vdr-suite-backend-agent"
ENROLL_BINARY="/usr/sbin/vdr-suite-backend-agent-enroll"
ADMIN_BINARY="/usr/sbin/vdr-suite-backend-agent-admin"
COMMAND_ADMIN_BINARY="/usr/sbin/vdr-suite-backend-agent-command-admin"
DROPIN_DIR="/etc/systemd/system/${AGENT_SERVICE}.d"
DROPIN_PATH="$DROPIN_DIR/phase63-native-probe.conf"

[[ -n "$EXPECTED_HEAD" ]] || fail expected_head_required
for command in git make g++ python3 pkg-config systemctl cmp install sha256sum; do
    require_command "$command"
done

CURRENT_BRANCH="$(git branch --show-current)" || fail branch_read_failed
CURRENT_HEAD="$(git rev-parse HEAD)" || fail head_read_failed
[[ "$CURRENT_BRANCH" == "$EXPECTED_BRANCH" ]] || fail unexpected_branch
[[ "$CURRENT_HEAD" == "$EXPECTED_HEAD" ]] || fail unexpected_head
[[ -z "$(git status --porcelain)" ]] || fail worktree_not_clean
SHORT_HEAD="$(printf '%s' "$CURRENT_HEAD" | cut -c1-8)" || fail short_head_failed
EVIDENCE_DIR="${PHASE63_EVIDENCE_DIR:-/root/vdr-suite-phase63-fenced-native-operation-acceptance-${SHORT_HEAD}}"
[[ ! -e "$EVIDENCE_DIR" ]] || fail evidence_directory_already_exists
mkdir -m 0700 "$EVIDENCE_DIR" || fail evidence_directory_create_failed
BACKUP_DIR="$EVIDENCE_DIR/backups"
mkdir -m 0700 "$BACKUP_DIR" || fail backup_directory_create_failed
PROXY_LOG="$EVIDENCE_DIR/native-proxy.jsonl"
PROXY_DROP="$EVIDENCE_DIR/native-proxy.drop"
PROXY_READY="$EVIDENCE_DIR/native-proxy.ready"
PROXY_STOP="$EVIDENCE_DIR/native-proxy.stop"

IDENTITY_PATH="$(protected_config_value "$CONFIG_PATH" IDENTITY_PATH)" || fail identity_path_read_failed
COMMAND_STATE_PATH="$(protected_config_value "$CONFIG_PATH" COMMAND_STATE_PATH)" || fail command_state_path_read_failed
CONFIGURED_TYPES="$(protected_config_value "$CONFIG_PATH" COMMAND_TYPES)" || fail command_types_read_failed
[[ -n "$IDENTITY_PATH" && "$IDENTITY_PATH" == /* ]] || fail invalid_identity_path
[[ -n "$COMMAND_STATE_PATH" && "$COMMAND_STATE_PATH" == /* ]] || fail invalid_command_state_path
[[ -z "$CONFIGURED_TYPES" ]] || fail packaged_command_types_must_be_disabled
[[ -f "$IDENTITY_PATH" ]] || fail identity_missing
[[ -f "$DATABASE" ]] || fail database_missing

VDR_LIBDIR="$(pkg-config --variable=libdir vdr)" || fail vdr_libdir_failed
VDR_APIVERSION="$(pkg-config --variable=apiversion vdr)" || fail vdr_apiversion_failed
[[ -n "$VDR_LIBDIR" && -n "$VDR_APIVERSION" ]] || fail vdr_pkgconfig_incomplete
PLUGIN_INSTALLED="$VDR_LIBDIR/libvdr-suitebridge.so.$VDR_APIVERSION"
PLUGIN_CANDIDATE="vdr-plugin-suite-bridge/libvdr-suitebridge.so"

[[ "$(systemctl is-active "$VDR_SERVICE" 2>/dev/null || true)" == active ]] || fail vdr_not_active_before_acceptance
[[ "$(systemctl is-active "$DAEMON_SERVICE" 2>/dev/null || true)" == active ]] || fail daemon_not_active_before_acceptance
[[ "$(systemctl is-active "$AGENT_SERVICE" 2>/dev/null || true)" == active ]] || fail agent_not_active_before_acceptance
VDR_WAS_ACTIVE=1
DAEMON_WAS_ACTIVE=1
AGENT_WAS_ACTIVE=1

ORIGINAL_IDENTITY="$(identity_metadata)" || fail original_identity_read_failed
printf '%s\n' "$ORIGINAL_IDENTITY" >"$EVIDENCE_DIR/identity.before.tsv" || fail identity_evidence_write_failed
vdr_fingerprint "$EVIDENCE_DIR/vdr.before.tsv" || fail vdr_fingerprint_before_failed
cp -a "$CONFIG_PATH" "$BACKUP_DIR/agent-config" || fail config_backup_failed
if [[ -f "$COMMAND_STATE_PATH" ]]; then
    STATE_EXISTED=1
    cp -a "$COMMAND_STATE_PATH" "$BACKUP_DIR/command-state" || fail command_state_backup_failed
fi
if [[ -f "$DROPIN_PATH" ]]; then
    DROPIN_EXISTED=1
    cp -a "$DROPIN_PATH" "$BACKUP_DIR/agent-dropin.conf" || fail dropin_backup_failed
fi

make daemon backend-agent backend-agent-enrollment backend-agent-admin backend-agent-command-admin || fail candidate_binary_build_failed
make -C vdr-plugin-suite-bridge clean all || fail candidate_plugin_build_failed
[[ -z "$(git status --porcelain)" ]] || fail build_changed_worktree

CANDIDATES=(
    ".build/vdr-suite-daemon:$DAEMON_BINARY"
    ".build/vdr-suite-backend-agent:$AGENT_BINARY"
    ".build/vdr-suite-backend-agent-enroll:$ENROLL_BINARY"
    ".build/vdr-suite-backend-agent-admin:$ADMIN_BINARY"
    ".build/vdr-suite-backend-agent-command-admin:$COMMAND_ADMIN_BINARY"
    "$PLUGIN_CANDIDATE:$PLUGIN_INSTALLED"
)
for pair in "${CANDIDATES[@]}"; do
    candidate="${pair%%:*}"
    installed="${pair#*:}"
    [[ -f "$candidate" && -f "$installed" ]] || fail "candidate_or_installed_missing_$(basename "$installed")"
    mkdir -p "$BACKUP_DIR$(dirname "$installed")" || fail backup_parent_create_failed
    cp -a "$installed" "$BACKUP_DIR$installed" || fail "binary_backup_failed_$(basename "$installed")"
done

printf '%s\n' "$CURRENT_HEAD" >"$EVIDENCE_DIR/HEAD" || fail evidence_head_write_failed
sha256sum "${CANDIDATES[@]%%:*}" >"$EVIDENCE_DIR/candidates.sha256" || fail candidate_hash_failed

systemctl stop "$AGENT_SERVICE" || fail agent_stop_failed
systemctl stop "$DAEMON_SERVICE" || fail daemon_stop_failed
systemctl stop "$VDR_SERVICE" || fail vdr_stop_failed

for pair in "${CANDIDATES[@]}"; do
    candidate="${pair%%:*}"
    installed="${pair#*:}"
    install -m 0755 "$candidate" "$installed" || fail "candidate_install_failed_$(basename "$installed")"
    cmp -s "$candidate" "$installed" || fail "installed_candidate_mismatch_$(basename "$installed")"
done

mkdir -p "$DROPIN_DIR" || fail dropin_directory_create_failed
cat >"$DROPIN_PATH" <<EOF_DROPIN
[Service]
ExecStart=
ExecStart=$AGENT_BINARY --config $CONFIG_PATH --native-probe --suitebridge-host 127.0.0.1 --suitebridge-port $PROXY_PORT
EOF_DROPIN
systemctl daemon-reload || fail daemon_reload_failed

python3 tools/phase63-runtime-acceptance/native-probe-proxy.py \
    --listen-host 127.0.0.1 --listen-port "$PROXY_PORT" \
    --upstream-host 127.0.0.1 --upstream-port "$SVDRP_PORT" \
    --drop-file "$PROXY_DROP" --log-file "$PROXY_LOG" \
    --ready-file "$PROXY_READY" --stop-file "$PROXY_STOP" \
    >"$EVIDENCE_DIR/native-proxy.stdout" 2>"$EVIDENCE_DIR/native-proxy.stderr" &
PROXY_PID=$!
for ((index=0; index<50; ++index)); do
    [[ -f "$PROXY_READY" ]] && break
    sleep 0.1
 done
[[ -f "$PROXY_READY" ]] || fail proxy_start_failed

systemctl start "$VDR_SERVICE" || fail vdr_candidate_start_failed
wait_active "$VDR_SERVICE" 30 || fail vdr_candidate_not_active
systemctl start "$DAEMON_SERVICE" || fail daemon_candidate_start_failed
wait_active "$DAEMON_SERVICE" 30 || fail daemon_candidate_not_active
systemctl start "$AGENT_SERVICE" || fail agent_candidate_start_failed
wait_active "$AGENT_SERVICE" 30 || fail agent_candidate_not_active
ORIGINAL_AGENT_ID="$(printf '%s' "$ORIGINAL_IDENTITY" | cut -f1)" || fail agent_id_extract_failed
wait_agent_online "$ORIGINAL_AGENT_ID" 60 >"$EVIDENCE_DIR/agent.candidate.json" || fail candidate_agent_not_online

BASELINE_ASSIGNMENT="$(enqueue_native_probe 60)" || fail baseline_native_probe_enqueue_failed
printf '%s\n' "$BASELINE_ASSIGNMENT" >"$EVIDENCE_DIR/baseline.assignment.json" || fail baseline_assignment_write_failed
BASELINE_ID="$(printf '%s\n' "$BASELINE_ASSIGNMENT" | json_field commandId)" || fail baseline_command_id_failed
BASELINE_STATUS="$(wait_command_state "$BASELINE_ID" completed succeeded verified 1 90)" || fail baseline_native_probe_failed
printf '%s\n' "$BASELINE_STATUS" >"$EVIDENCE_DIR/baseline.status.json" || fail baseline_status_write_failed
BASELINE_SEQUENCE="$(state_value native_execution_sequence)" || fail baseline_sequence_missing
BASELINE_RECEIPT="$(state_value native_receipt_evidence)" || fail baseline_receipt_missing
BASELINE_RESULT="$(state_value native_result_evidence)" || fail baseline_result_missing
BASELINE_READBACK="$(state_value native_readback_evidence)" || fail baseline_readback_missing
verify_separate_evidence "$BASELINE_RECEIPT" "$BASELINE_RESULT" "$BASELINE_READBACK" || fail native_evidence_not_separate

systemctl restart "$DAEMON_SERVICE" || fail daemon_restart_failed
wait_active "$DAEMON_SERVICE" 30 || fail daemon_restart_not_active
systemctl restart "$AGENT_SERVICE" || fail agent_restart_failed
wait_active "$AGENT_SERVICE" 30 || fail agent_restart_not_active
wait_agent_online "$ORIGINAL_AGENT_ID" 60 >"$EVIDENCE_DIR/agent.after-restart.json" || fail agent_restart_not_online
BASELINE_EXEC_BEFORE="$(proxy_exec_summary "$BASELINE_ID")" || fail baseline_proxy_summary_failed
"$COMMAND_ADMIN_BINARY" --database "$DATABASE" --backend "$BACKEND_ID" --replay "$BASELINE_ID" >"$EVIDENCE_DIR/baseline.replay.json" || fail baseline_replay_request_failed
BASELINE_DELIVERY="$(printf '%s\n' "$BASELINE_STATUS" | json_field deliveryCount)" || fail baseline_delivery_count_failed
REPLAY_STATUS="$(wait_command_state "$BASELINE_ID" completed succeeded verified "$((BASELINE_DELIVERY+1))" 90)" || fail baseline_replay_failed
printf '%s\n' "$REPLAY_STATUS" >"$EVIDENCE_DIR/baseline.replay-status.json" || fail baseline_replay_status_write_failed
[[ "$(state_value native_execution_sequence)" == "$BASELINE_SEQUENCE" ]] || fail replay_sequence_changed
BASELINE_EXEC_AFTER="$(proxy_exec_summary "$BASELINE_ID")" || fail replay_proxy_summary_failed
[[ "$BASELINE_EXEC_BEFORE" == "$BASELINE_EXEC_AFTER" ]] || fail control_plane_replay_reexecuted_native_probe

printf '1\n' >"$PROXY_DROP" || fail proxy_drop_arm_failed
RECOVERY_ASSIGNMENT="$(enqueue_native_probe 60)" || fail recovery_native_probe_enqueue_failed
printf '%s\n' "$RECOVERY_ASSIGNMENT" >"$EVIDENCE_DIR/recovery.assignment.json" || fail recovery_assignment_write_failed
RECOVERY_ID="$(printf '%s\n' "$RECOVERY_ASSIGNMENT" | json_field commandId)" || fail recovery_command_id_failed
RECOVERY_STATUS="$(wait_command_state "$RECOVERY_ID" completed succeeded verified 1 120)" || fail lost_response_recovery_failed
printf '%s\n' "$RECOVERY_STATUS" >"$EVIDENCE_DIR/recovery.status.json" || fail recovery_status_write_failed
RECOVERY_SUMMARY="$(proxy_exec_summary "$RECOVERY_ID")" || fail recovery_proxy_summary_failed
printf '%s\n' "$RECOVERY_SUMMARY" >"$EVIDENCE_DIR/recovery.proxy-summary.json" || fail recovery_summary_write_failed
printf '%s\n' "$RECOVERY_SUMMARY" | python3 -c '
import json,sys
values=json.load(sys.stdin)
raise SystemExit(0 if len(values)==2 and values[0][0]=="drop" and values[0][1]=="accepted" and values[1][0]=="relay" and values[1][1]=="duplicate" and values[0][2]==values[1][2] else 1)
' || fail lost_response_not_exact_replay

printf '1\n' >"$PROXY_DROP" || fail epoch_drop_arm_failed
EPOCH_ASSIGNMENT="$(enqueue_native_probe 60)" || fail epoch_native_probe_enqueue_failed
printf '%s\n' "$EPOCH_ASSIGNMENT" >"$EVIDENCE_DIR/epoch.assignment.json" || fail epoch_assignment_write_failed
EPOCH_ID="$(printf '%s\n' "$EPOCH_ASSIGNMENT" | json_field commandId)" || fail epoch_command_id_failed
wait_proxy_drop "$EPOCH_ID" 90 || fail epoch_drop_not_observed
systemctl stop "$AGENT_SERVICE" || fail agent_stop_for_epoch_failed
OLD_PLUGIN_EPOCH="$(state_value plugin_instance_epoch)" || fail old_plugin_epoch_missing
systemctl restart "$VDR_SERVICE" || fail vdr_epoch_restart_failed
wait_active "$VDR_SERVICE" 30 || fail vdr_epoch_restart_not_active
systemctl start "$AGENT_SERVICE" || fail agent_epoch_restart_failed
wait_active "$AGENT_SERVICE" 30 || fail agent_epoch_restart_not_active
EPOCH_STATUS="$(wait_command_state "$EPOCH_ID" waiting_reconciliation outcome_unknown outcome_unknown 1 90)" || fail plugin_epoch_fence_failed
printf '%s\n' "$EPOCH_STATUS" >"$EVIDENCE_DIR/epoch.status.json" || fail epoch_status_write_failed
EPOCH_SUMMARY="$(proxy_exec_summary "$EPOCH_ID")" || fail epoch_proxy_summary_failed
printf '%s\n' "$EPOCH_SUMMARY" | python3 -c 'import json,sys; values=json.load(sys.stdin); raise SystemExit(0 if len(values)==1 and values[0][0]=="drop" else 1)' || fail old_epoch_command_replayed
[[ "$(state_value plugin_instance_epoch)" == "$OLD_PLUGIN_EPOCH" ]] || fail old_epoch_evidence_rewritten

FINAL_ASSIGNMENT="$(enqueue_native_probe 60)" || fail post_epoch_native_probe_enqueue_failed
printf '%s\n' "$FINAL_ASSIGNMENT" >"$EVIDENCE_DIR/post-epoch.assignment.json" || fail post_epoch_assignment_write_failed
FINAL_ID="$(printf '%s\n' "$FINAL_ASSIGNMENT" | json_field commandId)" || fail post_epoch_command_id_failed
FINAL_STATUS="$(wait_command_state "$FINAL_ID" completed succeeded verified 1 90)" || fail post_epoch_native_probe_failed
printf '%s\n' "$FINAL_STATUS" >"$EVIDENCE_DIR/post-epoch.status.json" || fail post_epoch_status_write_failed

restore_runtime strict || fail runtime_restore_failed
cmp -s "$BACKUP_DIR/agent-config" "$CONFIG_PATH" || fail original_configuration_not_restored
FINAL_IDENTITY="$(identity_metadata)" || fail final_identity_read_failed
printf '%s\n' "$FINAL_IDENTITY" >"$EVIDENCE_DIR/identity.after.tsv" || fail final_identity_write_failed
[[ "$FINAL_IDENTITY" == "$ORIGINAL_IDENTITY" ]] || fail agent_identity_or_credential_changed
vdr_fingerprint "$EVIDENCE_DIR/vdr.after.tsv" || fail vdr_fingerprint_after_failed
cmp -s "$EVIDENCE_DIR/vdr.before.tsv" "$EVIDENCE_DIR/vdr.after.tsv" || fail vdr_native_state_changed
[[ "$(systemctl is-active "$VDR_SERVICE" 2>/dev/null || true)" == active ]] || fail vdr_not_active_after_restore
[[ "$(systemctl is-active "$DAEMON_SERVICE" 2>/dev/null || true)" == active ]] || fail daemon_not_active_after_restore
[[ "$(systemctl is-active "$AGENT_SERVICE" 2>/dev/null || true)" == active ]] || fail agent_not_active_after_restore

SUCCESS=1
printf 'PHASE_63_FENCED_NATIVE_OPERATION_ACCEPTANCE=PASS\n'
printf 'BASELINE_NATIVE_PROBE_COMPLETED=yes\n'
printf 'CONTROL_PLANE_REPLAY_NO_NATIVE_REEXECUTION=yes\n'
printf 'LOST_LOCAL_RESPONSE_RECOVERED=yes\n'
printf 'PLUGIN_EPOCH_REPLAY_FENCED=yes\n'
printf 'NATIVE_RECEIPT_EVIDENCE_SEPARATE=yes\n'
printf 'NATIVE_RESULT_EVIDENCE_SEPARATE=yes\n'
printf 'AUTHORITATIVE_READBACK_VERIFIED=yes\n'
printf 'DAEMON_RESTART_PERSISTED=yes\n'
printf 'AGENT_RESTART_RECOVERED=yes\n'
printf 'VDR_NATIVE_STATE_UNCHANGED=yes\n'
printf 'EXISTING_AGENT_IDENTITY_PRESERVED=yes\n'
printf 'CREDENTIAL_GENERATION_PRESERVED=yes\n'
printf 'ORIGINAL_CONFIGURATION_RESTORED=yes\n'
printf 'VDR_ACTIVE=yes\n'
printf 'DAEMON_ACTIVE=yes\n'
printf 'AGENT_ACTIVE=yes\n'
printf 'EVIDENCE=%s\n' "$EVIDENCE_DIR"
exit 0
