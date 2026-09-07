#!/usr/bin/env bash
set +e
set +u
set +o pipefail
umask 077

fail() {
    printf 'RECORDING_CUT_ACCEPTANCE_PREFLIGHT=FAIL\n' >&2
    printf 'REASON=%s\n' "$1" >&2
    if [[ -n "${EVIDENCE_DIR:-}" ]]; then
        printf 'EVIDENCE=%s\n' "$EVIDENCE_DIR" >&2
    fi
    exit 1
}

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)" \
    || fail "repository_root_resolution_failed"
cd "$REPO_ROOT" || fail "repository_root_enter_failed"

require_command() {
    command -v "$1" >/dev/null 2>&1 || fail "missing_command_$1"
}

EXPECTED_BRANCH="${RECORDING_CUT_EXPECTED_BRANCH:-}"
EXPECTED_HEAD="${RECORDING_CUT_EXPECTED_HEAD:-}"
CONTROL_PLANE_URL="${RECORDING_CUT_CONTROL_PLANE_URL:-}"
CURL_CONFIG="${RECORDING_CUT_CURL_CONFIG:-}"
CA_CERTIFICATE_PATH="${RECORDING_CUT_CA_CERTIFICATE_PATH:-}"
EVIDENCE_DIR="${RECORDING_CUT_EVIDENCE_DIR:-}"
BACKEND_ID="${RECORDING_CUT_BACKEND_ID:-default}"
RECORDING_ID="${RECORDING_CUT_RECORDING_ID:-}"
DATABASE_PATH="${RECORDING_CUT_DATABASE_PATH:-/var/lib/vdr-suite/vdr-suite.db}"
DAEMON_SERVICE="${RECORDING_CUT_DAEMON_SERVICE:-vdr-suite-daemon.service}"
AGENT_SERVICE="${RECORDING_CUT_AGENT_SERVICE:-vdr-suite-backend-agent.service}"
VDR_SERVICE="${RECORDING_CUT_VDR_SERVICE:-vdr.service}"
SVDRP_PORT="${RECORDING_CUT_SVDRP_PORT:-6419}"
DAEMON_CANDIDATE=".build/vdr-suite-daemon"
AGENT_CANDIDATE=".build/vdr-suite-backend-agent"
ADMIN_CANDIDATE=".build/vdr-suite-backend-agent-command-admin"
PLUGIN_CANDIDATE="vdr-plugin-suite-bridge/libvdr-suitebridge.so"
DAEMON_INSTALLED="/usr/sbin/vdr-suite-daemon"
AGENT_INSTALLED="/usr/sbin/vdr-suite-backend-agent"
ADMIN_INSTALLED="/usr/sbin/vdr-suite-backend-agent-command-admin"

[[ "$EXPECTED_HEAD" =~ ^[0-9a-f]{40}$ ]] || fail "expected_head_missing_or_invalid"
[[ -n "$EXPECTED_BRANCH" ]] || fail "expected_branch_required"
[[ -n "$CONTROL_PLANE_URL" ]] || fail "control_plane_url_required"
[[ "$CONTROL_PLANE_URL" != */ ]] || fail "control_plane_url_must_not_end_with_slash"
case "$CONTROL_PLANE_URL" in
    https://*|http://127.0.0.1:*|http://localhost:*) ;;
    *) fail "control_plane_url_must_be_https_or_loopback_http" ;;
esac
[[ -n "$EVIDENCE_DIR" ]] || fail "evidence_directory_required"
[[ ! -e "$EVIDENCE_DIR" ]] || fail "evidence_directory_already_exists"
[[ -n "$RECORDING_ID" ]] || fail "recording_id_required"
[[ -f "$DATABASE_PATH" ]] || fail "database_missing"
[[ "$SVDRP_PORT" =~ ^[0-9]+$ ]] || fail "svdrp_port_invalid"
(( SVDRP_PORT > 0 && SVDRP_PORT <= 65535 )) || fail "svdrp_port_invalid"
[[ -z "$CA_CERTIFICATE_PATH" || -f "$CA_CERTIFICATE_PATH" ]] || fail "ca_certificate_missing"
if [[ -n "$CURL_CONFIG" ]]; then
    [[ -f "$CURL_CONFIG" ]] || fail "curl_config_missing"
    CURL_CONFIG_MODE="$(stat -c '%a' "$CURL_CONFIG")" \
        || fail "curl_config_stat_failed"
    [[ "${CURL_CONFIG_MODE: -2}" == "00" ]] || fail "curl_config_permissions_too_open"
fi

for command in git systemctl curl python3 sha256sum cmp svdrpsend grep stat pkg-config; do
    require_command "$command"
done

CURRENT_BRANCH="$(git branch --show-current)" || fail "branch_read_failed"
CURRENT_HEAD="$(git rev-parse HEAD)" || fail "head_read_failed"
WORKTREE_STATUS="$(git status --porcelain)" || fail "worktree_status_failed"
[[ "$CURRENT_BRANCH" == "$EXPECTED_BRANCH" ]] || fail "branch_mismatch"
[[ "$CURRENT_HEAD" == "$EXPECTED_HEAD" ]] || fail "head_mismatch"
[[ -z "$WORKTREE_STATUS" ]] || fail "worktree_not_clean"

VDR_LIBDIR="$(pkg-config --variable=libdir vdr)" || fail "vdr_libdir_failed"
VDR_APIVERSION="$(pkg-config --variable=apiversion vdr)" || fail "vdr_apiversion_failed"
[[ -n "$VDR_LIBDIR" && -n "$VDR_APIVERSION" ]] || fail "vdr_pkgconfig_incomplete"
PLUGIN_INSTALLED="$VDR_LIBDIR/libvdr-suitebridge.so.$VDR_APIVERSION"

for binary in \
    "$DAEMON_CANDIDATE" "$AGENT_CANDIDATE" "$ADMIN_CANDIDATE" \
    "$DAEMON_INSTALLED" "$AGENT_INSTALLED" "$ADMIN_INSTALLED"; do
    [[ -x "$binary" ]] || fail "binary_missing_$(basename "$binary")"
done
for plugin in "$PLUGIN_CANDIDATE" "$PLUGIN_INSTALLED"; do
    [[ -f "$plugin" ]] || fail "plugin_missing_$(basename "$plugin")"
done
cmp -s "$DAEMON_CANDIDATE" "$DAEMON_INSTALLED" || fail "installed_daemon_candidate_mismatch"
cmp -s "$AGENT_CANDIDATE" "$AGENT_INSTALLED" || fail "installed_agent_candidate_mismatch"
cmp -s "$ADMIN_CANDIDATE" "$ADMIN_INSTALLED" || fail "installed_command_admin_candidate_mismatch"
cmp -s "$PLUGIN_CANDIDATE" "$PLUGIN_INSTALLED" || fail "installed_suitebridge_candidate_mismatch"

VDR_STATE="$(systemctl is-active "$VDR_SERVICE" 2>/dev/null)"
DAEMON_STATE="$(systemctl is-active "$DAEMON_SERVICE" 2>/dev/null)"
AGENT_STATE="$(systemctl is-active "$AGENT_SERVICE" 2>/dev/null)"
[[ "$VDR_STATE" == active ]] || fail "vdr_not_active"
[[ "$DAEMON_STATE" == active ]] || fail "daemon_not_active"
[[ "$AGENT_STATE" == active ]] || fail "agent_not_active"

mkdir -m 0700 "$EVIDENCE_DIR" || fail "evidence_directory_create_failed"
printf '%s\n' "$CURRENT_BRANCH" > "$EVIDENCE_DIR/branch" \
    || fail "evidence_branch_write_failed"
printf '%s\n' "$CURRENT_HEAD" > "$EVIDENCE_DIR/HEAD" \
    || fail "evidence_head_write_failed"
printf 'vdr=%s\ndaemon=%s\nagent=%s\n' \
    "$VDR_STATE" "$DAEMON_STATE" "$AGENT_STATE" \
    > "$EVIDENCE_DIR/services.txt" \
    || fail "evidence_services_write_failed"
sha256sum \
    "$DAEMON_CANDIDATE" "$AGENT_CANDIDATE" "$ADMIN_CANDIDATE" "$PLUGIN_CANDIDATE" \
    "$DAEMON_INSTALLED" "$AGENT_INSTALLED" "$ADMIN_INSTALLED" "$PLUGIN_INSTALLED" \
    > "$EVIDENCE_DIR/binaries.sha256" \
    || fail "evidence_hashes_failed"

"$ADMIN_INSTALLED" \
    --database "$DATABASE_PATH" \
    --backend "$BACKEND_ID" \
    --recording-cut-provider-ownership-status \
    > "$EVIDENCE_DIR/recording-cut-provider-ownership.json" \
    || fail "recording_cut_provider_ownership_read_failed"

python3 - "$EVIDENCE_DIR/recording-cut-provider-ownership.json" <<'PY'
import json
import sys
from pathlib import Path

ownership = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
valid = (
    ownership.get("present") is True
    and ownership.get("active") is True
    and ownership.get("authorityDomain") == "vdr.recording.cut"
    and ownership.get("providerId") == "suitebridge:recording-cut"
    and ownership.get("providerKind") == "suitebridge"
    and ownership.get("allowedCapabilities") == ["vdr.recording.cut"]
    and int(ownership.get("ownershipGeneration", 0)) > 0
)
raise SystemExit(0 if valid else 1)
PY
[[ $? -eq 0 ]] || fail "recording_cut_provider_ownership_unavailable"

svdrpsend -p "$SVDRP_PORT" "PLUG suitebridge CAPS 1" \
    > "$EVIDENCE_DIR/suitebridge-caps.txt" \
    || fail "suitebridge_caps_read_failed"
grep -Fq '"id":"recording-cut-state","state":"available"' \
    "$EVIDENCE_DIR/suitebridge-caps.txt" \
    || fail "recording_cut_state_capability_unavailable"

svdrpsend -p "$SVDRP_PORT" "PLUG suitebridge NCUT CAP 1 start" \
    > "$EVIDENCE_DIR/suitebridge-ncut-cap.txt" \
    || fail "suitebridge_ncut_cap_read_failed"
grep -Fq 'vdr-suite-ncut-cap/1 vdr.recording.cut 1 recording-cut enabled suitebridge' \
    "$EVIDENCE_DIR/suitebridge-ncut-cap.txt" \
    || fail "recording_cut_mutation_provider_unavailable"

QUERY_STRING="$(python3 - "$BACKEND_ID" "$RECORDING_ID" <<'PY'
import sys
from urllib.parse import urlencode
print(urlencode({"backend": sys.argv[1], "recordingId": sys.argv[2]}))
PY
)" || fail "query_string_encode_failed"

curl_arguments=(--silent --show-error --fail-with-body --max-time 15 --output -)
if [[ -n "$CURL_CONFIG" ]]; then
    curl_arguments=(--config "$CURL_CONFIG" "${curl_arguments[@]}")
fi
if [[ -n "$CA_CERTIFICATE_PATH" ]]; then
    curl_arguments+=(--cacert "$CA_CERTIFICATE_PATH")
fi
curl_arguments+=(--get --request GET)

curl "${curl_arguments[@]}" \
    "$CONTROL_PLANE_URL/api/vdr/recordings/marks?$QUERY_STRING" \
    > "$EVIDENCE_DIR/marks.json" \
    || fail "marks_read_failed"
curl "${curl_arguments[@]}" \
    "$CONTROL_PLANE_URL/api/vdr/recordings/cut?$QUERY_STRING" \
    > "$EVIDENCE_DIR/cut-preview.json" \
    || fail "cut_preview_read_failed"

python3 - \
    "$BACKEND_ID" "$RECORDING_ID" \
    "$EVIDENCE_DIR/recording-cut-provider-ownership.json" \
    "$EVIDENCE_DIR/marks.json" "$EVIDENCE_DIR/cut-preview.json" \
    "$EVIDENCE_DIR/preflight.json" <<'PY'
import json
import re
import sys
from pathlib import Path

backend_id, recording_id, ownership_path, marks_path, cut_path, output_path = sys.argv[1:]
ownership = json.loads(Path(ownership_path).read_text(encoding="utf-8"))
marks = json.loads(Path(marks_path).read_text(encoding="utf-8"))
cut = json.loads(Path(cut_path).read_text(encoding="utf-8"))
revision = marks.get("marksRevision", "")

checks = {
    "local_provider_ownership_active": (
        ownership.get("present") is True
        and ownership.get("active") is True
        and ownership.get("authorityDomain") == "vdr.recording.cut"
        and ownership.get("providerId") == "suitebridge:recording-cut"
        and ownership.get("providerKind") == "suitebridge"
        and ownership.get("allowedCapabilities") == ["vdr.recording.cut"]
        and int(ownership.get("ownershipGeneration", 0)) > 0
    ),
    "marks_backend_matches": marks.get("backendId") == backend_id,
    "marks_recording_matches": marks.get("recordingId") == recording_id,
    "marks_available": marks.get("availability") == "available",
    "marks_present": marks.get("marksFilePresent") is True,
    "marks_revision_valid": re.fullmatch(r"[0-9a-f]{32}", revision) is not None,
    "marks_sequences_usable": int(marks.get("sequenceCount", 0)) > 0,
    "marks_recording_idle": marks.get("inUse") is False and int(marks.get("inUseFlags", -1)) == 0,
    "cut_backend_matches": cut.get("backendId") == backend_id,
    "cut_recording_matches": cut.get("recordingId") == recording_id,
    "cut_available": cut.get("availability") == "available",
    "cut_ready": cut.get("ready") is True and cut.get("reason") == "ready",
    "cut_marks_readable": cut.get("marksReadable") is True,
    "cut_revision_matches": cut.get("marksRevision") == revision,
    "cut_sequences_usable": int(cut.get("sequenceCount", 0)) > 0,
    "cut_recording_idle": cut.get("inUse") is False and int(cut.get("inUseFlags", -1)) == 0,
    "cut_handler_idle": int(cut.get("handlerUsage", -1)) == 0,
    "cut_destination_free": cut.get("editedDestinationExists") is False,
    "cut_result_absent": cut.get("editedRecordingFound") is False,
}
failed = [name for name, passed in checks.items() if not passed]
summary = {
    "backendId": backend_id,
    "recordingId": recording_id,
    "ownershipGeneration": int(ownership.get("ownershipGeneration", 0)),
    "marksRevision": revision,
    "markCount": int(cut.get("markCount", 0)),
    "sequenceCount": int(cut.get("sequenceCount", 0)),
    "checks": checks,
    "readyForControlledCutAcceptance": not failed,
}
Path(output_path).write_text(
    json.dumps(summary, indent=2, sort_keys=True) + "\n",
    encoding="utf-8",
)
if failed:
    print("RECORDING_CUT_ACCEPTANCE_PREFLIGHT=FAIL", file=sys.stderr)
    print("REASON=precondition_failed:" + ",".join(failed), file=sys.stderr)
    raise SystemExit(1)
print("RECORDING_CUT_ACCEPTANCE_PREFLIGHT=PASS")
print(f"HEAD={Path(output_path).parent.joinpath('HEAD').read_text(encoding='utf-8').strip()}")
print(f"BACKEND_ID={backend_id}")
print(f"RECORDING_ID={recording_id}")
print(f"OWNERSHIP_GENERATION={summary['ownershipGeneration']}")
print(f"MARKS_REVISION={revision}")
print(f"SEQUENCE_COUNT={summary['sequenceCount']}")
print("HTTP_MUTATION=not_executed")
print("NCUT_EXEC=not_executed")
print("SERVICE_MUTATION=not_executed")
PY
[[ $? -eq 0 ]] || fail "preflight_validation_failed"
