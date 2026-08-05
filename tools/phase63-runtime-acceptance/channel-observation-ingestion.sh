#!/usr/bin/env bash
umask 077

SUCCESS=0
CONFIG_RESTORED=0
AGENT_WAS_ACTIVE=0
CONFIG_BACKUP=""

fail() {
    printf 'PHASE_63_CHANNEL_OBSERVATION_UPGRADE_ACCEPTANCE=FAIL\n' >&2
    printf 'REASON=%s\n' "$1" >&2
    if [[ -n "${EVIDENCE_DIR:-}" ]]; then
        printf 'EVIDENCE=%s\n' "$EVIDENCE_DIR" >&2
    fi
    exit 1
}

restore_runtime_configuration() {
    local mode="${1:-best-effort}"
    if [[ "$CONFIG_RESTORED" -eq 1 ]]; then
        return 0
    fi
    if [[ "$mode" == strict ]]; then
        [[ -n "${AGENT_SERVICE:-}" ]] || return 1
        [[ -n "$CONFIG_BACKUP" && -f "$CONFIG_BACKUP" ]] || return 1
        [[ -n "${CONFIG_PATH:-}" ]] || return 1
        systemctl stop "$AGENT_SERVICE" >/dev/null 2>&1 || return 1
        cp -a "$CONFIG_BACKUP" "$CONFIG_PATH" >/dev/null 2>&1 || return 1
        cmp -s "$CONFIG_BACKUP" "$CONFIG_PATH" || return 1
        if [[ -n "${FIXTURE_PATH:-}" ]]; then
            rm -f "$FIXTURE_PATH" >/dev/null 2>&1 || return 1
        fi
        if [[ "$AGENT_WAS_ACTIVE" -eq 1 ]]; then
            systemctl start "$AGENT_SERVICE" >/dev/null 2>&1 || return 1
        fi
        rm -f "$CONFIG_BACKUP" >/dev/null 2>&1 || return 1
        CONFIG_RESTORED=1
        return 0
    fi
    if [[ -n "${AGENT_SERVICE:-}" ]]; then
        systemctl stop "$AGENT_SERVICE" >/dev/null 2>&1 || true
    fi
    if [[ -n "$CONFIG_BACKUP" && -f "$CONFIG_BACKUP" && -n "${CONFIG_PATH:-}" ]]; then
        cp -a "$CONFIG_BACKUP" "$CONFIG_PATH" >/dev/null 2>&1 || true
    fi
    if [[ -n "${FIXTURE_PATH:-}" ]]; then
        rm -f "$FIXTURE_PATH" >/dev/null 2>&1 || true
    fi
    if [[ "$AGENT_WAS_ACTIVE" -eq 1 && -n "${AGENT_SERVICE:-}" ]]; then
        systemctl start "$AGENT_SERVICE" >/dev/null 2>&1 || true
    fi
    if [[ -n "$CONFIG_BACKUP" ]]; then
        rm -f "$CONFIG_BACKUP" >/dev/null 2>&1 || true
    fi
}
trap restore_runtime_configuration EXIT INT TERM

require_command() {
    command -v "$1" >/dev/null 2>&1 || fail "missing_command_$1"
}

agent_status() {
    "$ADMIN_BINARY" --database "$DATABASE" --backend "$BACKEND_ID" --status
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
    fail "service_not_active_${service}"
}

wait_for_initial_channel_observation() {
    local expected_agent_id="$1"
    local attempts="$2"
    local attempt status
    for ((attempt=0; attempt<attempts; ++attempt)); do
        status="$(agent_status 2>/dev/null || true)"
        if printf '%s\n' "$status" | python3 -c '
import json,sys
expected=sys.argv[1]
try:
    value=json.load(sys.stdin)
except Exception:
    raise SystemExit(1)
observation=value.get("channelObservation", {})
raise SystemExit(0 if (
    value.get("present") is True
    and value.get("state") == "online"
    and value.get("agentId") == expected
    and value.get("readOnly") is True
    and "channels-conf" in value.get("adapters", [])
    and "channels" in value.get("observationDomains", [])
    and observation.get("present") is True
    and int(observation.get("backendGeneration", 0)) == int(value.get("backendGeneration", 0))
    and int(observation.get("snapshotGeneration", 0)) > 0
    and int(observation.get("producerSequence", 0)) == 1
    and int(observation.get("factCount", 0)) > 0
) else 1)
' "$expected_agent_id"; then
            printf '%s\n' "$status"
            return 0
        fi
        sleep 1
    done
    return 1
}

wait_for_changed_channel_snapshot() {
    local expected_agent_id="$1"
    local previous_snapshot="$2"
    local previous_revision="$3"
    local expected_fact_count="$4"
    local attempts="$5"
    local attempt status
    for ((attempt=0; attempt<attempts; ++attempt)); do
        status="$(agent_status 2>/dev/null || true)"
        if printf '%s\n' "$status" | python3 -c '
import json,sys
expected_agent=sys.argv[1]
previous_snapshot=int(sys.argv[2])
previous_revision=sys.argv[3]
expected_count=int(sys.argv[4])
try:
    value=json.load(sys.stdin)
except Exception:
    raise SystemExit(1)
observation=value.get("channelObservation", {})
raise SystemExit(0 if (
    value.get("state") == "online"
    and value.get("agentId") == expected_agent
    and observation.get("present") is True
    and int(observation.get("backendGeneration", 0)) == int(value.get("backendGeneration", 0))
    and int(observation.get("snapshotGeneration", 0)) > previous_snapshot
    and int(observation.get("producerSequence", 0)) == 1
    and observation.get("resourceRevision") != previous_revision
    and int(observation.get("factCount", -1)) == expected_count
) else 1)
' "$expected_agent_id" "$previous_snapshot" "$previous_revision" "$expected_fact_count"; then
            printf '%s\n' "$status"
            return 0
        fi
        sleep 1
    done
    return 1
}

wait_for_new_channel_lineage() {
    local expected_agent_id="$1"
    local previous_backend_generation="$2"
    local previous_snapshot_generation="$3"
    local expected_fact_count="$4"
    local attempts="$5"
    local attempt status
    for ((attempt=0; attempt<attempts; ++attempt)); do
        status="$(agent_status 2>/dev/null || true)"
        if printf '%s\n' "$status" | python3 -c '
import json,sys
expected_agent=sys.argv[1]
previous_backend=int(sys.argv[2])
previous_snapshot=int(sys.argv[3])
expected_count=int(sys.argv[4])
try:
    value=json.load(sys.stdin)
except Exception:
    raise SystemExit(1)
observation=value.get("channelObservation", {})
raise SystemExit(0 if (
    value.get("state") == "online"
    and value.get("agentId") == expected_agent
    and int(value.get("backendGeneration", 0)) > previous_backend
    and observation.get("present") is True
    and int(observation.get("backendGeneration", 0)) == int(value.get("backendGeneration", 0))
    and int(observation.get("snapshotGeneration", 0)) > previous_snapshot
    and int(observation.get("producerSequence", 0)) == 1
    and int(observation.get("factCount", -1)) == expected_count
) else 1)
' "$expected_agent_id" "$previous_backend_generation" "$previous_snapshot_generation" "$expected_fact_count"; then
            printf '%s\n' "$status"
            return 0
        fi
        sleep 1
    done
    return 1
}

vdr_fingerprint() {
    local output="$1"
    python3 - "$VDR_VIDEO_DIR" "$SOURCE_CHANNELS_CONF" "$output" <<'PY_FINGERPRINT' || return 1
from pathlib import Path
import hashlib
import sys

video = Path(sys.argv[1])
channels = Path(sys.argv[2])
output = Path(sys.argv[3])
files = [
    channels,
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
PY_FINGERPRINT
}

configure_channel_observation() {
    python3 - "$CONFIG_PATH" "$FIXTURE_PATH" <<'PY_CONFIG' || return 1
from pathlib import Path
import os
import stat
import sys

path = Path(sys.argv[1])
fixture = sys.argv[2]
metadata = path.lstat()
if stat.S_ISLNK(metadata.st_mode) or not stat.S_ISREG(metadata.st_mode):
    raise SystemExit("unsafe Backend Agent config")
lines = path.read_text(encoding="utf-8").splitlines()
values = {}
for line in lines:
    if not line or line.startswith("#") or "=" not in line:
        continue
    key, value = line.split("=", 1)
    if key in values:
        raise SystemExit("duplicate Backend Agent config key")
    values[key] = value

def with_value(existing: str, required: str) -> str:
    items = [item for item in existing.split(",") if item]
    if required not in items:
        items.append(required)
    return ",".join(items)

replacements = {
    "ADAPTERS": with_value(values.get("ADAPTERS", ""), "channels-conf"),
    "OBSERVATION_DOMAINS": with_value(
        with_value(values.get("OBSERVATION_DOMAINS", ""), "backend-health"),
        "channels",
    ),
    "CHANNELS_CONF_PATH": fixture,
}
seen = set()
rendered = []
for line in lines:
    key = line.split("=", 1)[0] if "=" in line and not line.startswith("#") else ""
    if key in replacements:
        rendered.append(key + "=" + replacements[key])
        seen.add(key)
    else:
        rendered.append(line)
for key in ("ADAPTERS", "OBSERVATION_DOMAINS", "CHANNELS_CONF_PATH"):
    if key not in seen:
        rendered.append(key + "=" + replacements[key])
temporary = path.with_name(path.name + ".phase63-channel.tmp")
temporary.write_text("\n".join(rendered) + "\n", encoding="utf-8")
os.chmod(temporary, metadata.st_mode & 0o777)
os.chown(temporary, metadata.st_uid, metadata.st_gid)
os.replace(temporary, path)
PY_CONFIG
}

mutate_channel_fixture() {
    python3 - "$FIXTURE_PATH" <<'PY_MUTATE' || return 1
from pathlib import Path
import os
import stat
import sys

path = Path(sys.argv[1])
metadata = path.lstat()
if stat.S_ISLNK(metadata.st_mode) or not stat.S_ISREG(metadata.st_mode):
    raise SystemExit("unsafe Channel fixture")
lines = path.read_text(encoding="utf-8").splitlines()
changed = False
for index, line in enumerate(lines):
    if not line or line.startswith("#") or line.startswith(":"):
        continue
    fields = line.split(":")
    if len(fields) != 13:
        continue
    name_field = fields[0]
    name, separator, provider = name_field.partition(";")
    suffix = "-phase63-probe"
    if name.endswith(suffix):
        raise SystemExit("Channel fixture already mutated")
    maximum = max(1, 255 - len(suffix.encode("utf-8")))
    encoded = name.encode("utf-8")
    while len(encoded) > maximum:
        name = name[:-1]
        encoded = name.encode("utf-8")
    fields[0] = name + suffix + (separator + provider if separator else "")
    lines[index] = ":".join(fields)
    changed = True
    break
if not changed:
    raise SystemExit("no mutable Channel record found")
temporary = path.with_name(path.name + ".tmp")
temporary.write_text("\n".join(lines) + "\n", encoding="utf-8")
os.chmod(temporary, metadata.st_mode & 0o777)
os.chown(temporary, metadata.st_uid, metadata.st_gid)
os.replace(temporary, path)
PY_MUTATE
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
IDENTITY_PATH="${PHASE63_AGENT_IDENTITY_PATH:-$STATE_DIR/identity}"
SOURCE_CHANNELS_CONF="${PHASE63_CHANNELS_CONF_SOURCE:-/var/lib/vdr/channels.conf}"
FIXTURE_PATH="${PHASE63_CHANNEL_FIXTURE_PATH:-$STATE_DIR/channel-acceptance.conf}"
DAEMON_BINARY="/usr/sbin/vdr-suite-daemon"
AGENT_BINARY="/usr/sbin/vdr-suite-backend-agent"
ENROLL_BINARY="/usr/sbin/vdr-suite-backend-agent-enroll"
ADMIN_BINARY="/usr/sbin/vdr-suite-backend-agent-admin"
SECRET_SCANNER="tools/check_phase63_runtime_evidence_secrets.py"
CHANNEL_EXERCISER="tools/phase63-runtime-acceptance/exercise_channel_observation.py"
READONLY_REGRESSION=".build/vdr_suite_real_readonly_regression"

[[ -n "$EXPECTED_BRANCH" ]] || fail "expected_branch_required"
[[ -n "$EXPECTED_HEAD" ]] || fail "expected_head_required"
[[ -n "$CONTROL_PLANE_URL" ]] || fail "control_plane_url_required"
[[ "$CONTROL_PLANE_URL" == https://* ]] || fail "control_plane_url_must_be_https"
[[ "$CONTROL_PLANE_URL" != */ ]] || fail "control_plane_url_must_not_end_with_slash"
[[ -n "$EVIDENCE_DIR" ]] || fail "evidence_directory_required"
[[ ! -e "$EVIDENCE_DIR" ]] || fail "evidence_directory_already_exists"
[[ -z "$CA_CERTIFICATE_PATH" || -f "$CA_CERTIFICATE_PATH" ]] || fail "ca_certificate_missing"
[[ -f "$CONFIG_PATH" && ! -L "$CONFIG_PATH" ]] || fail "backend_agent_config_not_regular"
[[ -f "$SOURCE_CHANNELS_CONF" && ! -L "$SOURCE_CHANNELS_CONF" ]] || fail "channels_conf_source_not_regular"
[[ "$FIXTURE_PATH" != "$SOURCE_CHANNELS_CONF" ]] || fail "fixture_must_not_be_native_channels_conf"
[[ ! -e "$FIXTURE_PATH" ]] || fail "channel_fixture_already_exists"

for command in git systemctl curl python3 sha256sum cmp journalctl grep install mktemp make; do
    require_command "$command"
done
[[ -f "$SECRET_SCANNER" ]] || fail "evidence_secret_scanner_missing"
[[ -f "$CHANNEL_EXERCISER" ]] || fail "channel_acceptance_helper_missing"

CURRENT_BRANCH="$(git branch --show-current)" || fail "branch_read_failed"
CURRENT_HEAD="$(git rev-parse HEAD)" || fail "head_read_failed"
[[ "$CURRENT_BRANCH" == "$EXPECTED_BRANCH" ]] || fail "branch_mismatch"
[[ "$CURRENT_HEAD" == "$EXPECTED_HEAD" ]] || fail "head_mismatch"
[[ -z "$(git status --porcelain)" ]] || fail "worktree_not_clean"

make daemon backend-agent backend-agent-enrollment backend-agent-admin >/dev/null ||
    fail "candidate_binary_build_failed"

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
    cmp -s "$source_binary" "$installed_binary" || fail "installed_candidate_mismatch_$(basename "$installed_binary")"
done

[[ -f "$DATABASE" ]] || fail "production_database_missing"
[[ -f "$IDENTITY_PATH" && ! -L "$IDENTITY_PATH" ]] || fail "existing_agent_identity_required"
[[ "$(systemctl is-active "$VDR_SERVICE" 2>/dev/null || true)" == active ]] || fail "vdr_not_active_before_acceptance"
[[ "$(systemctl is-active "$DAEMON_SERVICE" 2>/dev/null || true)" == active ]] || fail "daemon_not_active_before_acceptance"
[[ "$(systemctl is-active "$AGENT_SERVICE" 2>/dev/null || true)" == active ]] || fail "agent_not_active_before_acceptance"
AGENT_WAS_ACTIVE=1

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

make real-vdr-readonly-regression-helper >/dev/null || fail "real_vdr_readonly_helper_build_failed"
[[ -x "$READONLY_REGRESSION" ]] || fail "real_vdr_readonly_helper_missing"

mkdir -m 0700 "$EVIDENCE_DIR" || fail "evidence_directory_create_failed"
printf '%s\n' "$CURRENT_HEAD" > "$EVIDENCE_DIR/HEAD" || fail "evidence_head_write_failed"
sha256sum "$DAEMON_BINARY" "$AGENT_BINARY" "$ENROLL_BINARY" "$ADMIN_BINARY" \
    > "$EVIDENCE_DIR/installed-binaries.sha256" || fail "installed_binary_hash_failed"
vdr_fingerprint "$EVIDENCE_DIR/vdr-state.before" || fail "vdr_fingerprint_before_failed"
"$READONLY_REGRESSION" --run > "$EVIDENCE_DIR/vdr-readonly.before.log" 2>&1 || fail "vdr_readonly_regression_before_failed"

ORIGINAL_STATUS="$(agent_status)" || fail "initial_agent_status_failed"
printf '%s\n' "$ORIGINAL_STATUS" > "$EVIDENCE_DIR/status.original.json" || fail "initial_status_write_failed"
AGENT_ID="$(printf '%s\n' "$ORIGINAL_STATUS" | python3 -c 'import json,sys; value=json.load(sys.stdin); assert value.get("present") is True and value.get("state") == "online"; print(value.get("agentId", ""))')" || fail "existing_agent_status_invalid"
[[ -n "$AGENT_ID" ]] || fail "existing_agent_id_missing"
INITIAL_IDENTITY_METADATA="$(protected_identity_metadata)" || fail "protected_identity_metadata_invalid"
[[ "${INITIAL_IDENTITY_METADATA%%$'\t'*}" == "$AGENT_ID" ]] || fail "protected_identity_agent_mismatch"

CONFIG_BACKUP="$(mktemp /run/vdr-suite-phase63-channel-config.XXXXXX)" || fail "config_backup_create_failed"
cp -a "$CONFIG_PATH" "$CONFIG_BACKUP" || fail "config_backup_failed"
install -m 0600 -o vdr -g vdr "$SOURCE_CHANNELS_CONF" "$FIXTURE_PATH" || fail "channel_fixture_create_failed"
configure_channel_observation || fail "channel_observation_config_failed"

systemctl restart "$AGENT_SERVICE" || fail "agent_restart_for_channel_opt_in_failed"
wait_for_service_active "$AGENT_SERVICE" 30
INITIAL_STATUS="$(wait_for_initial_channel_observation "$AGENT_ID" 90)" || fail "initial_channel_observation_not_observed"
printf '%s\n' "$INITIAL_STATUS" > "$EVIDENCE_DIR/status.channel-initial.json" || fail "channel_initial_status_write_failed"
INITIAL_FACT_COUNT="$(printf '%s\n' "$INITIAL_STATUS" | python3 -c 'import json,sys; print(json.load(sys.stdin)["channelObservation"]["factCount"])')" || fail "initial_fact_count_read_failed"
INITIAL_SNAPSHOT="$(printf '%s\n' "$INITIAL_STATUS" | python3 -c 'import json,sys; print(json.load(sys.stdin)["channelObservation"]["snapshotGeneration"])')" || fail "initial_snapshot_read_failed"
INITIAL_REVISION="$(printf '%s\n' "$INITIAL_STATUS" | python3 -c 'import json,sys; print(json.load(sys.stdin)["channelObservation"]["resourceRevision"])')" || fail "initial_revision_read_failed"

mutate_channel_fixture || fail "channel_fixture_mutation_failed"
CHANGED_STATUS="$(wait_for_changed_channel_snapshot "$AGENT_ID" "$INITIAL_SNAPSHOT" "$INITIAL_REVISION" "$INITIAL_FACT_COUNT" 90)" || fail "changed_channel_snapshot_not_observed"
printf '%s\n' "$CHANGED_STATUS" > "$EVIDENCE_DIR/status.channel-changed.json" || fail "channel_changed_status_write_failed"
CHANGED_BACKEND_GENERATION="$(printf '%s\n' "$CHANGED_STATUS" | python3 -c 'import json,sys; print(json.load(sys.stdin)["backendGeneration"])')" || fail "changed_backend_generation_read_failed"
CHANGED_SNAPSHOT="$(printf '%s\n' "$CHANGED_STATUS" | python3 -c 'import json,sys; print(json.load(sys.stdin)["channelObservation"]["snapshotGeneration"])')" || fail "changed_snapshot_read_failed"

systemctl stop "$AGENT_SERVICE" || fail "agent_stop_before_daemon_restart_failed"
BEFORE_DAEMON_RESTART="$(agent_status)" || fail "status_before_daemon_restart_failed"
printf '%s\n' "$BEFORE_DAEMON_RESTART" > "$EVIDENCE_DIR/status.before-daemon-restart.json" || fail "status_before_daemon_restart_write_failed"
systemctl restart "$DAEMON_SERVICE" || fail "daemon_restart_failed"
wait_for_service_active "$DAEMON_SERVICE" 30
AFTER_DAEMON_RESTART="$(agent_status)" || fail "status_after_daemon_restart_failed"
printf '%s\n' "$AFTER_DAEMON_RESTART" > "$EVIDENCE_DIR/status.after-daemon-restart.json" || fail "status_after_daemon_restart_write_failed"
printf '%s\n%s\n' "$BEFORE_DAEMON_RESTART" "$AFTER_DAEMON_RESTART" | python3 -c '
import json,sys
before=json.loads(sys.stdin.readline())
after=json.loads(sys.stdin.readline())
assert before.get("agentId") == after.get("agentId")
assert before.get("channelObservation") == after.get("channelObservation")
' || fail "channel_cursor_not_persisted_across_daemon_restart"

systemctl start "$AGENT_SERVICE" || fail "agent_start_after_daemon_restart_failed"
POST_RESTART_STATUS="$(wait_for_new_channel_lineage "$AGENT_ID" "$CHANGED_BACKEND_GENERATION" "$CHANGED_SNAPSHOT" "$INITIAL_FACT_COUNT" 90)" || fail "new_channel_observation_lineage_not_observed"
printf '%s\n' "$POST_RESTART_STATUS" > "$EVIDENCE_DIR/status.after-agent-reconnect.json" || fail "post_restart_status_write_failed"
POST_RESTART_BACKEND_GENERATION="$(printf '%s\n' "$POST_RESTART_STATUS" | python3 -c 'import json,sys; print(json.load(sys.stdin)["backendGeneration"])')" || fail "post_restart_backend_generation_read_failed"
POST_RESTART_SNAPSHOT="$(printf '%s\n' "$POST_RESTART_STATUS" | python3 -c 'import json,sys; print(json.load(sys.stdin)["channelObservation"]["snapshotGeneration"])')" || fail "post_restart_snapshot_read_failed"

systemctl stop "$AGENT_SERVICE" || fail "agent_stop_before_replay_gap_failed"
BEFORE_REPLAY_GAP="$(agent_status)" || fail "status_before_replay_gap_failed"
printf '%s\n' "$BEFORE_REPLAY_GAP" > "$EVIDENCE_DIR/status.before-replay-gap.json" || fail "status_before_replay_gap_write_failed"
python3 "$CHANNEL_EXERCISER" \
    --database "$DATABASE" \
    --identity "$IDENTITY_PATH" \
    --backend "$BACKEND_ID" \
    --control-plane-url "$CONTROL_PLANE_URL" \
    --ca-certificate-path "$CA_CERTIFICATE_PATH" \
    > "$EVIDENCE_DIR/channel-replay-gap.log" 2>&1 || fail "channel_replay_gap_exercise_failed"
grep -qx 'CHANNEL_OBSERVATION_REPLAY=PASS' "$EVIDENCE_DIR/channel-replay-gap.log" || fail "channel_replay_not_proven"
grep -qx 'CHANNEL_OBSERVATION_GAP_RESYNC=PASS' "$EVIDENCE_DIR/channel-replay-gap.log" || fail "channel_gap_resync_not_proven"
grep -qx 'CHANNEL_OBSERVATION_FACTS_UNCHANGED=PASS' "$EVIDENCE_DIR/channel-replay-gap.log" || fail "channel_facts_unchanged_not_proven"
AFTER_REPLAY_GAP="$(agent_status)" || fail "status_after_replay_gap_failed"
printf '%s\n' "$AFTER_REPLAY_GAP" > "$EVIDENCE_DIR/status.after-replay-gap.json" || fail "status_after_replay_gap_write_failed"
printf '%s\n%s\n' "$BEFORE_REPLAY_GAP" "$AFTER_REPLAY_GAP" | python3 -c '
import json,sys
before=json.loads(sys.stdin.readline())
after=json.loads(sys.stdin.readline())
assert before.get("agentId") == after.get("agentId")
assert before.get("channelObservation") == after.get("channelObservation")
' || fail "channel_state_changed_after_replay_gap"

systemctl start "$AGENT_SERVICE" || fail "agent_start_after_replay_gap_failed"
RECOVERED_STATUS="$(wait_for_new_channel_lineage "$AGENT_ID" "$POST_RESTART_BACKEND_GENERATION" "$POST_RESTART_SNAPSHOT" "$INITIAL_FACT_COUNT" 90)" || fail "channel_recovery_after_resync_not_observed"
printf '%s\n' "$RECOVERED_STATUS" > "$EVIDENCE_DIR/status.channel-recovered.json" || fail "recovered_status_write_failed"

vdr_fingerprint "$EVIDENCE_DIR/vdr-state.after" || fail "vdr_fingerprint_after_failed"
cmp -s "$EVIDENCE_DIR/vdr-state.before" "$EVIDENCE_DIR/vdr-state.after" || fail "vdr_native_state_changed"
"$READONLY_REGRESSION" --run > "$EVIDENCE_DIR/vdr-readonly.after.log" 2>&1 || fail "vdr_readonly_regression_after_failed"

journalctl -u "$DAEMON_SERVICE" -u "$AGENT_SERVICE" --since "20 minutes ago" --no-pager \
    > "$EVIDENCE_DIR/runtime.journal.log" || fail "runtime_journal_capture_failed"
SECRET_SCAN_REPORT="$EVIDENCE_DIR/evidence-secret-scan.txt"
python3 "$SECRET_SCANNER" "$EVIDENCE_DIR" > "$SECRET_SCAN_REPORT"
secret_scan_status=$?
if [[ "$secret_scan_status" -ne 0 ]]; then
    if [[ "$secret_scan_status" -eq 1 ]]; then
        fail "secret_like_material_found_in_evidence_logs"
    fi
    fail "evidence_secret_scan_failed"
fi

restore_runtime_configuration strict || fail "original_configuration_restore_failed"
wait_for_service_active "$AGENT_SERVICE" 30
RESTORED_STATUS="$(agent_status)" || fail "restored_agent_status_failed"
printf '%s\n' "$RESTORED_STATUS" > "$EVIDENCE_DIR/status.configuration-restored.json" || fail "restored_status_write_failed"
printf '%s\n' "$RESTORED_STATUS" | python3 -c '
import json,sys
expected=sys.argv[1]
value=json.load(sys.stdin)
assert value.get("present") is True
assert value.get("state") == "online"
assert value.get("agentId") == expected
' "$AGENT_ID" || fail "agent_identity_or_service_not_restored"
FINAL_IDENTITY_METADATA="$(protected_identity_metadata)" || fail "restored_identity_metadata_invalid"
[[ "$FINAL_IDENTITY_METADATA" == "$INITIAL_IDENTITY_METADATA" ]] || fail "credential_identity_or_generation_changed"

[[ "$(systemctl is-active "$VDR_SERVICE" 2>/dev/null || true)" == active ]] || fail "vdr_not_active_after_acceptance"
[[ "$(systemctl is-active "$DAEMON_SERVICE" 2>/dev/null || true)" == active ]] || fail "daemon_not_active_after_acceptance"
[[ "$(systemctl is-active "$AGENT_SERVICE" 2>/dev/null || true)" == active ]] || fail "agent_not_active_after_acceptance"
systemctl --no-pager --full status "$VDR_SERVICE" "$DAEMON_SERVICE" "$AGENT_SERVICE" \
    > "$EVIDENCE_DIR/service-status.txt" || fail "service_status_capture_failed"
sha256sum "$EVIDENCE_DIR"/* > "$EVIDENCE_DIR/SHA256SUMS" || fail "evidence_hash_failed"

SUCCESS=1
printf 'PHASE_63_CHANNEL_OBSERVATION_UPGRADE_ACCEPTANCE=PASS\n'
printf 'HEAD=%s\n' "$CURRENT_HEAD"
printf 'AGENT_ID=%s\n' "$AGENT_ID"
printf 'INITIAL_CHANNEL_FACT_COUNT=%s\n' "$INITIAL_FACT_COUNT"
printf 'CHANNEL_BASELINE=yes\n'
printf 'CHANNEL_FIXTURE_TRANSITION=yes\n'
printf 'CHANNEL_OBSERVATION_REPLAY=yes\n'
printf 'CHANNEL_OBSERVATION_GAP_RESYNC=yes\n'
printf 'CHANNEL_CURSOR_RESTART_PERSISTED=yes\n'
printf 'CHANNEL_RECOVERY_AFTER_RESYNC=yes\n'
printf 'EXISTING_AGENT_IDENTITY_PRESERVED=yes\n'
printf 'CREDENTIAL_GENERATION_PRESERVED=yes\n'
printf 'VDR_NATIVE_STATE_UNCHANGED=yes\n'
printf 'VDR_READ_ONLY_REGRESSION=yes\n'
printf 'ORIGINAL_CONFIGURATION_RESTORED=yes\n'
printf 'VDR_ACTIVE=yes\n'
printf 'DAEMON_ACTIVE=yes\n'
printf 'AGENT_ACTIVE=yes\n'
printf 'EVIDENCE=%s\n' "$EVIDENCE_DIR"
