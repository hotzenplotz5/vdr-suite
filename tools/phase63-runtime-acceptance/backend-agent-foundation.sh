#!/usr/bin/env bash
set -euo pipefail
umask 077

SUCCESS=0
ACCEPTANCE_STARTED=0

cleanup_failed_acceptance() {
    [[ "$SUCCESS" -eq 1 || "$ACCEPTANCE_STARTED" -eq 0 ]] && return 0
    systemctl stop "${AGENT_SERVICE:-vdr-suite-backend-agent.service}" >/dev/null 2>&1 || true
    if [[ -n "${DATABASE:-}" && -f "${DATABASE:-}" && -n "${BACKEND_ID:-}" ]]; then
        python3 - "$DATABASE" "$BACKEND_ID" <<'PY_CLEANUP' >/dev/null 2>&1 || true
import sqlite3, sys
connection = sqlite3.connect(sys.argv[1])
connection.execute("PRAGMA foreign_keys = ON")
backend = sys.argv[2]
rows = connection.execute(
    "SELECT actor_id, device_id, credential_id FROM backend_agents WHERE backend_id = ?",
    (backend,),
).fetchall()
credential_ids = [row[2] for row in rows]
device_ids = [row[1] for row in rows]
actor_ids = [row[0] for row in rows]
with connection:
    connection.execute(
        "DELETE FROM backend_agent_observation_receipts WHERE backend_id = ?",
        (backend,),
    )
    connection.execute(
        "DELETE FROM backend_agent_observation_cursors WHERE backend_id = ?",
        (backend,),
    )
    connection.execute(
        "DELETE FROM backend_agent_capabilities WHERE agent_id IN "
        "(SELECT agent_id FROM backend_agents WHERE backend_id = ?)",
        (backend,),
    )
    connection.execute(
        "DELETE FROM backend_agent_credential_rotations WHERE agent_id IN "
        "(SELECT agent_id FROM backend_agents WHERE backend_id = ?)",
        (backend,),
    )
    connection.execute("DELETE FROM backend_agents WHERE backend_id = ?", (backend,))
    connection.execute("DELETE FROM backend_agent_enrollments WHERE backend_id = ?", (backend,))
    for credential_id in credential_ids:
        connection.execute(
            "DELETE FROM security_basic_credential_verifiers WHERE credential_id = ?",
            (credential_id,),
        )
        connection.execute(
            "DELETE FROM security_credentials WHERE credential_id = ?",
            (credential_id,),
        )
    for device_id in device_ids:
        connection.execute("DELETE FROM security_devices WHERE device_id = ?", (device_id,))
    for actor_id in actor_ids:
        connection.execute("DELETE FROM security_actors WHERE actor_id = ?", (actor_id,))
connection.close()
PY_CLEANUP
    fi
    rm -f "${IDENTITY_PATH:-}" "${IDENTITY_PATH:-}.pending" \
        "${ENROLLMENT_PATH:-}" "${ENROLLMENT_PATH:-}.pending" 2>/dev/null || true
    if [[ -n "${CONFIG_PATH:-}" && -n "${EVIDENCE_DIR:-}" ]]; then
        if [[ -f "$EVIDENCE_DIR/backend-agent.conf.before" ]]; then
            cp -a "$EVIDENCE_DIR/backend-agent.conf.before" "$CONFIG_PATH" 2>/dev/null || true
        else
            rm -f "$CONFIG_PATH" 2>/dev/null || true
        fi
    fi
}
trap cleanup_failed_acceptance EXIT

fail() {
    printf 'PHASE_63_BACKEND_AGENT_RUNTIME_ACCEPTANCE=FAIL\n' >&2
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

agent_state() {
    agent_status | json_field state
}

wait_for_state() {
    local expected="$1"
    local attempts="$2"
    local current=""
    for ((attempt=0; attempt<attempts; ++attempt)); do
        current="$(agent_state)"
        if [[ "$current" == "$expected" ]]; then
            return 0
        fi
        sleep 1
    done
    fail "agent_state_${expected}_not_observed_last_${current}"
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

credential_generation() {
    python3 - "$DATABASE" "$BACKEND_ID" <<'PY'
import sqlite3, sys
connection = sqlite3.connect(sys.argv[1])
row = connection.execute(
    "SELECT credential_generation FROM backend_agents "
    "WHERE backend_id = ? AND revoked_at = 0 ORDER BY updated_at DESC LIMIT 1",
    (sys.argv[2],),
).fetchone()
connection.close()
if row is None:
    raise SystemExit(1)
print(row[0])
PY
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
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        lines.append(f"file\t{path}\t{digest}")
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
CONFIG_PATH="${PHASE63_AGENT_CONFIG:-/etc/vdr-suite/backend-agent.conf}"
STATE_DIR="${PHASE63_AGENT_STATE_DIR:-/var/lib/vdr-suite/backend-agent}"
IDENTITY_PATH="$STATE_DIR/identity"
ENROLLMENT_PATH="$STATE_DIR/enrollment"
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

for command in git systemctl curl python3 sha256sum runuser install cmp journalctl grep; do
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
[[ -f "$DATABASE" ]] || fail "production_database_missing"
[[ "$(systemctl is-active "$DAEMON_SERVICE")" == active ]] || fail "daemon_not_active_before_acceptance"
[[ "$(systemctl is-active "$VDR_SERVICE")" == active ]] || fail "vdr_not_active_before_acceptance"
if [[ "$(systemctl is-active "$AGENT_SERVICE" || true)" == active ]]; then
    fail "agent_service_already_active"
fi

for pair in \
    ".build/vdr-suite-daemon:$DAEMON_BINARY" \
    ".build/vdr-suite-backend-agent:$AGENT_BINARY" \
    ".build/vdr-suite-backend-agent-enroll:$ENROLL_BINARY" \
    ".build/vdr-suite-backend-agent-admin:$ADMIN_BINARY"; do
    source_binary="${pair%%:*}"
    installed_binary="${pair#*:}"
    [[ -x "$source_binary" ]] || fail "candidate_binary_missing_$(basename "$source_binary")"
    cmp -s "$source_binary" "$installed_binary" || fail "installed_candidate_mismatch_$(basename "$installed_binary")"
done

mkdir -m 0700 "$EVIDENCE_DIR"
printf '%s\n' "$CURRENT_HEAD" > "$EVIDENCE_DIR/HEAD"
sha256sum \
    "$DAEMON_BINARY" "$AGENT_BINARY" "$ENROLL_BINARY" "$ADMIN_BINARY" \
    > "$EVIDENCE_DIR/installed-binaries.sha256"
python3 - "$DATABASE" "$EVIDENCE_DIR/vdr-suite.db.before" <<'PY_BACKUP'
import sqlite3, sys
source = sqlite3.connect(f"file:{sys.argv[1]}?mode=ro", uri=True)
target = sqlite3.connect(sys.argv[2])
source.backup(target)
target.close()
source.close()
PY_BACKUP
[[ ! -e "$CONFIG_PATH" ]] || cp -a "$CONFIG_PATH" "$EVIDENCE_DIR/backend-agent.conf.before"
vdr_fingerprint "$EVIDENCE_DIR/vdr-state.before"

if agent_status | python3 -c 'import json,sys; raise SystemExit(0 if json.load(sys.stdin).get("present") else 1)'; then
    fail "backend_agent_history_already_present_for_backend"
fi
[[ ! -e "$IDENTITY_PATH" ]] || fail "agent_identity_already_present"
[[ ! -e "$ENROLLMENT_PATH" ]] || fail "agent_enrollment_already_present"

curl_arguments=(--silent --show-error --output /dev/null --max-time 15)
if [[ -n "$CA_CERTIFICATE_PATH" ]]; then
    curl_arguments+=(--cacert "$CA_CERTIFICATE_PATH")
fi
http_code="$(curl "${curl_arguments[@]}" --write-out '%{http_code}' "$CONTROL_PLANE_URL/api/backends")" || fail "control_plane_tls_probe_failed"
case "$http_code" in
    200|401|403|405) ;;
    000) fail "control_plane_tls_probe_failed" ;;
    404) fail "control_plane_api_route_not_found" ;;
    *) fail "control_plane_api_route_unexpected_http_${http_code}" ;;
esac

ACCEPTANCE_STARTED=1

install -d -m 0700 -o vdr -g vdr "$STATE_DIR"
install -d -m 0755 "$(dirname "$CONFIG_PATH")"
{
    printf 'CONTROL_PLANE_URL=%s\n' "$CONTROL_PLANE_URL"
    printf 'BACKEND_ID=%s\n' "$BACKEND_ID"
    printf 'IDENTITY_PATH=%s\n' "$IDENTITY_PATH"
    printf 'ENROLLMENT_PATH=%s\n' "$ENROLLMENT_PATH"
    printf 'CA_CERTIFICATE_PATH=%s\n' "$CA_CERTIFICATE_PATH"
    printf 'SOFTWARE_VERSION=vdr-suite-backend-agent/1\n'
    printf 'ADAPTERS=\n'
    printf 'OBSERVATION_DOMAINS=backend-health\n'
    printf 'HEARTBEAT_INTERVAL_SECONDS=30\n'
    printf 'RECONNECT_INITIAL_SECONDS=1\n'
    printf 'RECONNECT_MAXIMUM_SECONDS=30\n'
    printf 'CONNECT_TIMEOUT_MILLISECONDS=5000\n'
    printf 'REQUEST_TIMEOUT_MILLISECONDS=10000\n'
} > "$CONFIG_PATH"
chmod 0644 "$CONFIG_PATH"

"$ENROLL_BINARY" --database "$DATABASE" --backend "$BACKEND_ID" \
    --output "$ENROLLMENT_PATH" --ttl-seconds 900 \
    > "$EVIDENCE_DIR/enrollment-create.log" 2>&1
chown vdr:vdr "$ENROLLMENT_PATH"
chmod 0600 "$ENROLLMENT_PATH"

systemctl start "$AGENT_SERVICE"
wait_for_state online 45
FIRST_STATUS="$(agent_status)"
printf '%s\n' "$FIRST_STATUS" > "$EVIDENCE_DIR/status.initial.json"
printf '%s\n' "$FIRST_STATUS" | python3 -c '
import json,sys
value=json.load(sys.stdin)
assert value["state"] == "online"
assert value["backendGeneration"] > 0
assert value["heartbeatSequence"] > 0
assert value["capabilityRevision"] > 0
assert value["readOnly"] is True
assert value["adapters"] == []
assert value["observationDomains"] == ["backend-health"]
observation=value["backendHealthObservation"]
assert observation["present"] is True
assert observation["backendGeneration"] == value["backendGeneration"]
assert observation["snapshotGeneration"] > 0
assert observation["producerSequence"] > 0
assert observation["resourceRevision"].startswith("heartbeat-")
'
FIRST_OBSERVATION_SEQUENCE="$(printf '%s\n' "$FIRST_STATUS" | python3 -c '
import json,sys
print(json.load(sys.stdin)["backendHealthObservation"]["producerSequence"])
')"
FIRST_AGENT_ID="$(printf '%s\n' "$FIRST_STATUS" | json_field agentId)"
[[ -n "$FIRST_AGENT_ID" ]] || fail "first_agent_id_missing"

systemctl stop "$AGENT_SERVICE"
wait_for_state stale 125
agent_status > "$EVIDENCE_DIR/status.stale.json"
wait_for_state offline 40
agent_status > "$EVIDENCE_DIR/status.offline.json"

systemctl start "$AGENT_SERVICE"
wait_for_state online 45
RECONNECTED_STATUS="$(agent_status)"
printf '%s\n' "$RECONNECTED_STATUS" > "$EVIDENCE_DIR/status.reconnected.json"
printf '%s\n' "$RECONNECTED_STATUS" | python3 -c '
import json,sys
value=json.load(sys.stdin)
observation=value["backendHealthObservation"]
assert observation["present"] is True
assert observation["producerSequence"] > int(sys.argv[1])
' "$FIRST_OBSERVATION_SEQUENCE"

systemctl stop "$AGENT_SERVICE"
GENERATION_BEFORE="$(credential_generation)"
runuser -u vdr -- "$AGENT_BINARY" --config "$CONFIG_PATH" --rotate-credential \
    > "$EVIDENCE_DIR/credential-rotation.log" 2>&1
GENERATION_AFTER="$(credential_generation)"
[[ "$GENERATION_AFTER" -eq $((GENERATION_BEFORE + 1)) ]] || fail "credential_generation_not_advanced"
systemctl start "$AGENT_SERVICE"
wait_for_state online 45
ROTATED_STATUS="$(agent_status)"
printf '%s\n' "$ROTATED_STATUS" > "$EVIDENCE_DIR/status.rotated.json"

systemctl stop "$AGENT_SERVICE"
systemctl restart "$DAEMON_SERVICE"
wait_for_service_active "$DAEMON_SERVICE" 30
DAEMON_RESTARTED_STATUS="$(agent_status)"
printf '%s\n' "$DAEMON_RESTARTED_STATUS" \
    > "$EVIDENCE_DIR/status.daemon-restarted.json"
printf '%s\n%s\n' "$ROTATED_STATUS" "$DAEMON_RESTARTED_STATUS" | python3 -c '
import json,sys
before=json.loads(sys.stdin.readline())
after=json.loads(sys.stdin.readline())
assert before["agentId"] == after["agentId"]
assert before["backendHealthObservation"] == after["backendHealthObservation"]
'

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
printf '%s\n%s\n' "$DAEMON_RESTARTED_STATUS" "$AFTER_GAP_STATUS" | python3 -c '
import json,sys
before=json.loads(sys.stdin.readline())
after=json.loads(sys.stdin.readline())
assert before["backendHealthObservation"] == after["backendHealthObservation"]
'

systemctl start "$AGENT_SERVICE"
wait_for_state online 45
RECOVERED_STATUS="$(agent_status)"
printf '%s\n' "$RECOVERED_STATUS" > "$EVIDENCE_DIR/status.observation-recovered.json"
printf '%s\n%s\n' "$AFTER_GAP_STATUS" "$RECOVERED_STATUS" | python3 -c '
import json,sys
before=json.loads(sys.stdin.readline())
after=json.loads(sys.stdin.readline())
observation=after["backendHealthObservation"]
assert observation["present"] is True
assert observation["backendGeneration"] == after["backendGeneration"]
assert observation["producerSequence"] > 0
assert observation != before["backendHealthObservation"]
'

systemctl stop "$AGENT_SERVICE"
"$ADMIN_BINARY" --database "$DATABASE" --backend "$BACKEND_ID" \
    --revoke --reason runtime-acceptance-replacement \
    > "$EVIDENCE_DIR/revocation.log" 2>&1
wait_for_state revoked 5
agent_status > "$EVIDENCE_DIR/status.revoked.json"
if runuser -u vdr -- "$AGENT_BINARY" --config "$CONFIG_PATH" --once \
    > "$EVIDENCE_DIR/revoked-reconnect.log" 2>&1; then
    fail "revoked_agent_reconnected"
fi
rm -f "$IDENTITY_PATH" "$IDENTITY_PATH.pending" "$ENROLLMENT_PATH" "$ENROLLMENT_PATH.pending"

"$ENROLL_BINARY" --database "$DATABASE" --backend "$BACKEND_ID" \
    --output "$ENROLLMENT_PATH" --ttl-seconds 900 \
    > "$EVIDENCE_DIR/replacement-enrollment-create.log" 2>&1
chown vdr:vdr "$ENROLLMENT_PATH"
chmod 0600 "$ENROLLMENT_PATH"
systemctl start "$AGENT_SERVICE"
wait_for_state online 45
REPLACEMENT_STATUS="$(agent_status)"
printf '%s\n' "$REPLACEMENT_STATUS" > "$EVIDENCE_DIR/status.replacement.json"
REPLACEMENT_AGENT_ID="$(printf '%s\n' "$REPLACEMENT_STATUS" | json_field agentId)"
[[ -n "$REPLACEMENT_AGENT_ID" && "$REPLACEMENT_AGENT_ID" != "$FIRST_AGENT_ID" ]] || fail "replacement_agent_identity_not_distinct"
printf '%s\n' "$REPLACEMENT_STATUS" | python3 -c '
import json,sys
value=json.load(sys.stdin)
observation=value["backendHealthObservation"]
assert observation["present"] is True
assert observation["backendGeneration"] == value["backendGeneration"]
assert observation["snapshotGeneration"] > 0
assert observation["producerSequence"] > 0
'

python3 - "$DATABASE" "$BACKEND_ID" "$FIRST_AGENT_ID" "$REPLACEMENT_AGENT_ID" <<'PY'
import sqlite3, sys
connection = sqlite3.connect(sys.argv[1])
rows = connection.execute(
    "SELECT agent_id, revoked_at FROM backend_agents WHERE backend_id = ? ORDER BY created_at",
    (sys.argv[2],),
).fetchall()
receipt_agents = {
    row[0]
    for row in connection.execute(
        "SELECT DISTINCT agent_id FROM backend_agent_observation_receipts "
        "WHERE backend_id = ? AND observation_domain = 'backend-health' "
        "AND outcome = 'accepted'",
        (sys.argv[2],),
    ).fetchall()
}
cursor = connection.execute(
    "SELECT agent_id, producer_sequence FROM backend_agent_observation_cursors "
    "WHERE backend_id = ? AND observation_domain = 'backend-health'",
    (sys.argv[2],),
).fetchone()
connection.close()
by_id = {agent_id: revoked_at for agent_id, revoked_at in rows}
assert by_id.get(sys.argv[3], 0) > 0, rows
assert by_id.get(sys.argv[4], -1) == 0, rows
assert sum(1 for _, revoked_at in rows if revoked_at == 0) == 1, rows
assert sys.argv[3] in receipt_agents, receipt_agents
assert sys.argv[4] in receipt_agents, receipt_agents
assert cursor is not None and cursor[0] == sys.argv[4] and cursor[1] > 0, cursor
PY

vdr_fingerprint "$EVIDENCE_DIR/vdr-state.after"
cmp -s "$EVIDENCE_DIR/vdr-state.before" "$EVIDENCE_DIR/vdr-state.after" || fail "vdr_native_state_changed"
[[ "$(systemctl is-active "$VDR_SERVICE")" == active ]] || fail "vdr_not_active_after_acceptance"
[[ "$(systemctl is-active "$DAEMON_SERVICE")" == active ]] || fail "daemon_not_active_after_acceptance"
[[ "$(systemctl is-active "$AGENT_SERVICE")" == active ]] || fail "agent_not_active_after_acceptance"

journalctl -u "$AGENT_SERVICE" --since "10 minutes ago" --no-pager \
    > "$EVIDENCE_DIR/backend-agent.journal.log"
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
printf 'PHASE_63_BACKEND_AGENT_RUNTIME_ACCEPTANCE=PASS\n'
printf 'PHASE_63_BACKEND_HEALTH_INGESTION_RUNTIME_ACCEPTANCE=PASS\n'
printf 'HEAD=%s\n' "$CURRENT_HEAD"
printf 'CONTROL_PLANE_URL=%s\n' "$CONTROL_PLANE_URL"
printf 'FIRST_AGENT_ID=%s\n' "$FIRST_AGENT_ID"
printf 'REPLACEMENT_AGENT_ID=%s\n' "$REPLACEMENT_AGENT_ID"
printf 'CREDENTIAL_GENERATION=%s\n' "$GENERATION_AFTER"
printf 'BACKEND_HEALTH_OBSERVATION_INGESTED=yes\n'
printf 'BACKEND_HEALTH_OBSERVATION_REPLAY=yes\n'
printf 'BACKEND_HEALTH_OBSERVATION_GAP_RESYNC=yes\n'
printf 'OBSERVATION_CURSOR_RESTART_PERSISTED=yes\n'
printf 'OBSERVATION_REPLACEMENT_CURSOR=yes\n'
printf 'VDR_NATIVE_STATE_UNCHANGED=yes\n'
printf 'DAEMON_ACTIVE=yes\n'
printf 'AGENT_ACTIVE=yes\n'
printf 'EVIDENCE=%s\n' "$EVIDENCE_DIR"
