#!/usr/bin/env bash
set -euo pipefail
umask 077

SUCCESS=0
DAEMON_WAS_ACTIVE=0
AGENT_WAS_ACTIVE=0

restore_services() {
    [[ "$SUCCESS" -eq 1 ]] && return 0
    if [[ "$DAEMON_WAS_ACTIVE" -eq 1 ]]; then
        systemctl start "${DAEMON_SERVICE:-vdr-suite-daemon.service}" >/dev/null 2>&1 || true
    fi
    if [[ "$AGENT_WAS_ACTIVE" -eq 1 ]]; then
        systemctl start "${AGENT_SERVICE:-vdr-suite-backend-agent.service}" >/dev/null 2>&1 || true
    fi
}
trap restore_services EXIT

fail() {
    printf 'PHASE_63_BACKEND_HEALTH_INGESTION_UPGRADE_ACCEPTANCE=FAIL\n' >&2
    printf 'REASON=%s\n' "$1" >&2
    if [[ -n "${EVIDENCE_DIR:-}" ]]; then
        printf 'EVIDENCE=%s\n' "$EVIDENCE_DIR" >&2
    fi
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || fail "missing_command_$1"
}

json_field() {
    local field="$1"
    python3 -c 'import json,sys; print(json.load(sys.stdin).get(sys.argv[1], ""))' "$field"
}

agent_status() {
    "$ADMIN_BINARY" --database "$DATABASE" --backend "$BACKEND_ID" --status
}

wait_for_service_active() {
    local service="$1"
    local attempts="$2"
    for ((attempt=0; attempt<attempts; ++attempt)); do
        if [[ "$(systemctl is-active "$service" || true)" == active ]]; then
            return 0
        fi
        sleep 1
    done
    fail "service_not_active_${service}"
}

wait_for_online_observation() {
    local attempts="$1"
    local status=""
    for ((attempt=0; attempt<attempts; ++attempt)); do
        status="$(agent_status)"
        if printf '%s\n' "$status" | python3 -c '
import json,sys
value=json.load(sys.stdin)
observation=value.get("backendHealthObservation", {})
raise SystemExit(0 if (
    value.get("present") is True
    and value.get("state") == "online"
    and value.get("readOnly") is True
    and value.get("observationDomains") == ["backend-health"]
    and observation.get("present") is True
    and observation.get("backendGeneration") == value.get("backendGeneration")
    and int(observation.get("snapshotGeneration", 0)) > 0
    and int(observation.get("producerSequence", 0)) > 0
) else 1)
'; then
            printf '%s\n' "$status"
            return 0
        fi
        sleep 1
    done
    fail "online_backend_health_observation_not_observed"
}

wait_for_observation_advance() {
    local previous_sequence="$1"
    local attempts="$2"
    local status=""
    for ((attempt=0; attempt<attempts; ++attempt)); do
        status="$(agent_status)"
        if printf '%s\n' "$status" | python3 -c '
import json,sys
value=json.load(sys.stdin)
previous=int(sys.argv[1])
observation=value.get("backendHealthObservation", {})
raise SystemExit(0 if (
    value.get("state") == "online"
    and observation.get("present") is True
    and int(observation.get("producerSequence", 0)) > previous
) else 1)
' "$previous_sequence"; then
            printf '%s\n' "$status"
            return 0
        fi
        sleep 1
    done
    fail "backend_health_observation_did_not_advance"
}

wait_for_new_observation_lineage() {
    local previous_backend_generation="$1"
    local previous_snapshot_generation="$2"
    local attempts="$3"
    local status=""
    for ((attempt=0; attempt<attempts; ++attempt)); do
        status="$(agent_status)"
        if printf '%s\n' "$status" | python3 -c '
import json,sys
value=json.load(sys.stdin)
previous_backend=int(sys.argv[1])
previous_snapshot=int(sys.argv[2])
observation=value.get("backendHealthObservation", {})
raise SystemExit(0 if (
    value.get("state") == "online"
    and observation.get("present") is True
    and int(value.get("backendGeneration", 0)) > previous_backend
    and int(observation.get("backendGeneration", 0)) == int(value.get("backendGeneration", 0))
    and int(observation.get("snapshotGeneration", 0)) > previous_snapshot
    and int(observation.get("producerSequence", 0)) > 0
) else 1)
' "$previous_backend_generation" "$previous_snapshot_generation"; then
            printf '%s\n' "$status"
            return 0
        fi
        sleep 1
    done
    fail "new_backend_health_observation_lineage_not_observed"
}

vdr_fingerprint() {
    local output="$1"
    python3 - "$VDR_VIDEO_DIR" "$output" <<'PY'
from pathlib import Path
import hashlib
import sys

video = Path(sys.argv[1])
output = Path(sys.argv[2])
files = [
    Path("/var/lib/vdr/timers.conf"),
    Path("/var/lib/vdr/setup.conf"),
    Path("/var/lib/vdr/remote.conf"),
    Path("/var/lib/vdr/plugins/epgsearch/epgsearch.conf"),
]
lines = []
for path in files:
    if path.is_file():
        lines.append(f"file\t{path}\t{hashlib.sha256(path.read_bytes()).hexdigest()}")
if video.is_dir():
    recordings = sorted(
        str(path.relative_to(video))
        for path in video.rglob("*.rec")
        if path.is_dir()
    )
    digest = hashlib.sha256("\n".join(recordings).encode()).hexdigest()
    lines.append(f"recording-directories\t{video}\t{digest}\t{len(recordings)}")
output.write_text("\n".join(lines) + "\n", encoding="utf-8")
PY
}

[[ "${EUID}" -eq 0 ]] || fail "root_required"

EXPECTED_BRANCH="${PHASE63_EXPECTED_BRANCH:-}"
EXPECTED_HEAD="${PHASE63_EXPECTED_HEAD:-}"
CONTROL_PLANE_URL="${PHASE63_CONTROL_PLANE_URL:-}"
CA_CERTIFICATE_PATH="${PHASE63_CA_CERTIFICATE_PATH:-}"
EVIDENCE_DIR="${PHASE63_EVIDENCE_DIR:-}"
BACKEND_ID="${PHASE63_BACKEND_ID:-default}"
DATABASE="${PHASE63_DATABASE:-/var/lib/vdr-suite/vdr-suite.db}"
DAEMON_SERVICE="${PHASE63_DAEMON_SERVICE:-vdr-suite-daemon.service}"
AGENT_SERVICE="${PHASE63_AGENT_SERVICE:-vdr-suite-backend-agent.service}"
VDR_SERVICE="${PHASE63_VDR_SERVICE:-vdr.service}"
VDR_VIDEO_DIR="${PHASE63_VDR_VIDEO_DIR:-/srv/vdr/video.00}"
STATE_DIR="${PHASE63_AGENT_STATE_DIR:-/var/lib/vdr-suite/backend-agent}"
IDENTITY_PATH="${PHASE63_AGENT_IDENTITY_PATH:-$STATE_DIR/identity}"
DAEMON_BINARY="/usr/sbin/vdr-suite-daemon"
AGENT_BINARY="/usr/sbin/vdr-suite-backend-agent"
ENROLL_BINARY="/usr/sbin/vdr-suite-backend-agent-enroll"
ADMIN_BINARY="/usr/sbin/vdr-suite-backend-agent-admin"
SECRET_SCANNER="tools/check_phase63_runtime_evidence_secrets.py"
OBSERVATION_EXERCISER="tools/phase63-runtime-acceptance/exercise_backend_health_observation.py"

[[ -n "$EXPECTED_BRANCH" ]] || fail "expected_branch_required"
[[ -n "$EXPECTED_HEAD" ]] || fail "expected_head_required"
[[ -n "$CONTROL_PLANE_URL" ]] || fail "control_plane_url_required"
[[ "$CONTROL_PLANE_URL" == https://* ]] || fail "control_plane_url_must_be_https"
[[ "$CONTROL_PLANE_URL" != */ ]] || fail "control_plane_url_must_not_end_with_slash"
[[ -n "$EVIDENCE_DIR" ]] || fail "evidence_directory_required"
[[ ! -e "$EVIDENCE_DIR" ]] || fail "evidence_directory_already_exists"
[[ -z "$CA_CERTIFICATE_PATH" || -f "$CA_CERTIFICATE_PATH" ]] || fail "ca_certificate_missing"

for command in git systemctl curl python3 sha256sum cmp journalctl grep; do
    require_command "$command"
done
[[ -f "$SECRET_SCANNER" ]] || fail "evidence_secret_scanner_missing"
[[ -f "$OBSERVATION_EXERCISER" ]] || fail "observation_acceptance_helper_missing"

CURRENT_BRANCH="$(git branch --show-current)"
CURRENT_HEAD="$(git rev-parse HEAD)"
[[ "$CURRENT_BRANCH" == "$EXPECTED_BRANCH" ]] || fail "branch_mismatch"
[[ "$CURRENT_HEAD" == "$EXPECTED_HEAD" ]] || fail "head_mismatch"
[[ -z "$(git status --porcelain)" ]] || fail "worktree_not_clean"

for binary in "$DAEMON_BINARY" "$AGENT_BINARY" "$ENROLL_BINARY" "$ADMIN_BINARY"; do
    [[ -x "$binary" ]] || fail "installed_binary_missing_$(basename "$binary")"
done
for pair in \
    ".build/vdr-suite-daemon:$DAEMON_BINARY" \
    ".build/vdr-suite-backend-agent:$AGENT_BINARY" \
    ".build/vdr-suite-backend-agent-enroll:$ENROLL_BINARY" \
    ".build/vdr-suite-backend-agent-admin:$ADMIN_BINARY"; do
    source_binary="${pair%%:*}"
    installed_binary="${pair#*:}"
    [[ -x "$source_binary" ]] || fail "candidate_binary_missing_$(basename "$source_binary")"
    cmp -s "$source_binary" "$installed_binary" \
        || fail "installed_candidate_mismatch_$(basename "$installed_binary")"
done

[[ -f "$DATABASE" ]] || fail "production_database_missing"
[[ -f "$IDENTITY_PATH" ]] || fail "existing_agent_identity_required"
[[ "$(systemctl is-active "$VDR_SERVICE")" == active ]] || fail "vdr_not_active_before_acceptance"
[[ "$(systemctl is-active "$DAEMON_SERVICE")" == active ]] || fail "daemon_not_active_before_acceptance"
DAEMON_WAS_ACTIVE=1
[[ "$(systemctl is-active "$AGENT_SERVICE")" == active ]] || fail "agent_not_active_before_acceptance"
AGENT_WAS_ACTIVE=1

curl_arguments=(--silent --show-error --output /dev/null --max-time 15)
if [[ -n "$CA_CERTIFICATE_PATH" ]]; then
    curl_arguments+=(--cacert "$CA_CERTIFICATE_PATH")
fi
http_code="$(curl "${curl_arguments[@]}" --write-out '%{http_code}' "$CONTROL_PLANE_URL/api/backends")" \
    || fail "control_plane_tls_probe_failed"
case "$http_code" in
    200|401|403|405) ;;
    000) fail "control_plane_tls_probe_failed" ;;
    404) fail "control_plane_api_route_not_found" ;;
    *) fail "control_plane_api_route_unexpected_http_${http_code}" ;;
esac

mkdir -m 0700 "$EVIDENCE_DIR"
printf '%s\n' "$CURRENT_HEAD" > "$EVIDENCE_DIR/HEAD"
sha256sum "$DAEMON_BINARY" "$AGENT_BINARY" "$ENROLL_BINARY" "$ADMIN_BINARY" \
    > "$EVIDENCE_DIR/installed-binaries.sha256"
vdr_fingerprint "$EVIDENCE_DIR/vdr-state.before"

INITIAL_STATUS="$(wait_for_online_observation 60)"
printf '%s\n' "$INITIAL_STATUS" > "$EVIDENCE_DIR/status.initial.json"
AGENT_ID="$(printf '%s\n' "$INITIAL_STATUS" | json_field agentId)"
INITIAL_SEQUENCE="$(printf '%s\n' "$INITIAL_STATUS" | python3 -c '
import json,sys
print(json.load(sys.stdin)["backendHealthObservation"]["producerSequence"])
')"
[[ -n "$AGENT_ID" ]] || fail "agent_id_missing"

ADVANCED_STATUS="$(wait_for_observation_advance "$INITIAL_SEQUENCE" 75)"
printf '%s\n' "$ADVANCED_STATUS" > "$EVIDENCE_DIR/status.advanced.json"
ADVANCED_SEQUENCE="$(printf '%s\n' "$ADVANCED_STATUS" | python3 -c '
import json,sys
print(json.load(sys.stdin)["backendHealthObservation"]["producerSequence"])
')"
ADVANCED_BACKEND_GENERATION="$(printf '%s\n' "$ADVANCED_STATUS" | python3 -c '
import json,sys
print(json.load(sys.stdin)["backendGeneration"])
')"
ADVANCED_SNAPSHOT_GENERATION="$(printf '%s\n' "$ADVANCED_STATUS" | python3 -c '
import json,sys
print(json.load(sys.stdin)["backendHealthObservation"]["snapshotGeneration"])
')"

systemctl stop "$AGENT_SERVICE"
BEFORE_RESTART_STATUS="$(agent_status)"
printf '%s\n' "$BEFORE_RESTART_STATUS" > "$EVIDENCE_DIR/status.before-daemon-restart.json"
systemctl restart "$DAEMON_SERVICE"
wait_for_service_active "$DAEMON_SERVICE" 30
AFTER_RESTART_STATUS="$(agent_status)"
printf '%s\n' "$AFTER_RESTART_STATUS" > "$EVIDENCE_DIR/status.after-daemon-restart.json"
printf '%s\n%s\n' "$BEFORE_RESTART_STATUS" "$AFTER_RESTART_STATUS" | python3 -c '
import json,sys
before=json.loads(sys.stdin.readline())
after=json.loads(sys.stdin.readline())
assert before["agentId"] == after["agentId"]
assert before["backendHealthObservation"] == after["backendHealthObservation"]
'

systemctl start "$AGENT_SERVICE"
ONLINE_AFTER_RESTART="$(wait_for_new_observation_lineage "$ADVANCED_BACKEND_GENERATION" "$ADVANCED_SNAPSHOT_GENERATION" 75)"
printf '%s\n' "$ONLINE_AFTER_RESTART" > "$EVIDENCE_DIR/status.online-after-daemon-restart.json"
SYSTEM_AGENT_ID="$(printf '%s\n' "$ONLINE_AFTER_RESTART" | json_field agentId)"
[[ "$SYSTEM_AGENT_ID" == "$AGENT_ID" ]] || fail "agent_identity_changed_after_restart"
RESTART_BACKEND_GENERATION="$(printf '%s\n' "$ONLINE_AFTER_RESTART" | python3 -c '
import json,sys
print(json.load(sys.stdin)["backendGeneration"])
')"
RESTART_SNAPSHOT_GENERATION="$(printf '%s\n' "$ONLINE_AFTER_RESTART" | python3 -c '
import json,sys
print(json.load(sys.stdin)["backendHealthObservation"]["snapshotGeneration"])
')"

systemctl stop "$AGENT_SERVICE"
BEFORE_GAP_STATUS="$(agent_status)"
printf '%s\n' "$BEFORE_GAP_STATUS" > "$EVIDENCE_DIR/status.before-gap.json"
python3 "$OBSERVATION_EXERCISER" \
    --database "$DATABASE" \
    --identity "$IDENTITY_PATH" \
    --backend "$BACKEND_ID" \
    --control-plane-url "$CONTROL_PLANE_URL" \
    --ca-certificate-path "$CA_CERTIFICATE_PATH" \
    > "$EVIDENCE_DIR/observation-replay-gap.log" 2>&1
grep -qx 'BACKEND_HEALTH_OBSERVATION_REPLAY=PASS' \
    "$EVIDENCE_DIR/observation-replay-gap.log" \
    || fail "backend_health_observation_replay_not_proven"
grep -qx 'BACKEND_HEALTH_OBSERVATION_GAP_RESYNC=PASS' \
    "$EVIDENCE_DIR/observation-replay-gap.log" \
    || fail "backend_health_observation_gap_resync_not_proven"
AFTER_GAP_STATUS="$(agent_status)"
printf '%s\n' "$AFTER_GAP_STATUS" > "$EVIDENCE_DIR/status.after-gap.json"
printf '%s\n%s\n' "$BEFORE_GAP_STATUS" "$AFTER_GAP_STATUS" | python3 -c '
import json,sys
before=json.loads(sys.stdin.readline())
after=json.loads(sys.stdin.readline())
assert before["backendHealthObservation"] == after["backendHealthObservation"]
'

GAP_SEQUENCE="$(printf '%s\n' "$AFTER_GAP_STATUS" | python3 -c '
import json,sys
print(json.load(sys.stdin)["backendHealthObservation"]["producerSequence"])
')"
systemctl start "$AGENT_SERVICE"
RECOVERED_STATUS="$(wait_for_new_observation_lineage "$RESTART_BACKEND_GENERATION" "$RESTART_SNAPSHOT_GENERATION" 75)"
printf '%s\n' "$RECOVERED_STATUS" > "$EVIDENCE_DIR/status.recovered.json"
RECOVERED_AGENT_ID="$(printf '%s\n' "$RECOVERED_STATUS" | json_field agentId)"
[[ "$RECOVERED_AGENT_ID" == "$AGENT_ID" ]] || fail "agent_identity_changed_after_gap"

vdr_fingerprint "$EVIDENCE_DIR/vdr-state.after"
cmp -s "$EVIDENCE_DIR/vdr-state.before" "$EVIDENCE_DIR/vdr-state.after" \
    || fail "vdr_native_state_changed"
[[ "$(systemctl is-active "$VDR_SERVICE")" == active ]] || fail "vdr_not_active_after_acceptance"
[[ "$(systemctl is-active "$DAEMON_SERVICE")" == active ]] || fail "daemon_not_active_after_acceptance"
[[ "$(systemctl is-active "$AGENT_SERVICE")" == active ]] || fail "agent_not_active_after_acceptance"

journalctl -u "$DAEMON_SERVICE" -u "$AGENT_SERVICE" --since "15 minutes ago" --no-pager \
    > "$EVIDENCE_DIR/runtime.journal.log"
SECRET_SCAN_REPORT="$EVIDENCE_DIR/evidence-secret-scan.txt"
if python3 "$SECRET_SCANNER" "$EVIDENCE_DIR" > "$SECRET_SCAN_REPORT"; then
    secret_scan_status=0
else
    secret_scan_status=$?
fi
if [[ "$secret_scan_status" -ne 0 ]]; then
    cat "$SECRET_SCAN_REPORT" >&2
    if [[ "$secret_scan_status" -eq 1 ]]; then
        fail "secret_like_material_found_in_evidence_logs"
    fi
    fail "evidence_secret_scan_failed"
fi
systemctl --no-pager --full status "$DAEMON_SERVICE" "$AGENT_SERVICE" \
    > "$EVIDENCE_DIR/service-status.txt"
sha256sum "$EVIDENCE_DIR"/* > "$EVIDENCE_DIR/SHA256SUMS"

SUCCESS=1
printf 'PHASE_63_BACKEND_HEALTH_INGESTION_UPGRADE_ACCEPTANCE=PASS\n'
printf 'HEAD=%s\n' "$CURRENT_HEAD"
printf 'CONTROL_PLANE_URL=%s\n' "$CONTROL_PLANE_URL"
printf 'AGENT_ID=%s\n' "$AGENT_ID"
printf 'INITIAL_PRODUCER_SEQUENCE=%s\n' "$INITIAL_SEQUENCE"
printf 'ADVANCED_PRODUCER_SEQUENCE=%s\n' "$ADVANCED_SEQUENCE"
printf 'BACKEND_HEALTH_OBSERVATION_REPLAY=yes\n'
printf 'BACKEND_HEALTH_OBSERVATION_GAP_RESYNC=yes\n'
printf 'OBSERVATION_CURSOR_RESTART_PERSISTED=yes\n'
printf 'EXISTING_AGENT_IDENTITY_PRESERVED=yes\n'
printf 'VDR_NATIVE_STATE_UNCHANGED=yes\n'
printf 'DAEMON_ACTIVE=yes\n'
printf 'AGENT_ACTIVE=yes\n'
printf 'EVIDENCE=%s\n' "$EVIDENCE_DIR"
