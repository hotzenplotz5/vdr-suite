#!/usr/bin/env bash
set -euo pipefail
umask 077

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

fail() {
    printf 'RECORDING_CUT_ACCEPTANCE_PREFLIGHT=FAIL\n' >&2
    printf 'REASON=%s\n' "$1" >&2
    if [[ -n "${EVIDENCE_DIR:-}" ]]; then
        printf 'EVIDENCE=%s\n' "$EVIDENCE_DIR" >&2
    fi
    exit 1
}

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
DAEMON_SERVICE="${RECORDING_CUT_DAEMON_SERVICE:-vdr-suite-daemon.service}"
AGENT_SERVICE="${RECORDING_CUT_AGENT_SERVICE:-vdr-suite-backend-agent.service}"
VDR_SERVICE="${RECORDING_CUT_VDR_SERVICE:-vdr.service}"
SVDRP_PORT="${RECORDING_CUT_SVDRP_PORT:-6419}"
DAEMON_CANDIDATE=".build/vdr-suite-daemon"
AGENT_CANDIDATE=".build/vdr-suite-backend-agent"
PLUGIN_CANDIDATE="vdr-plugin-suite-bridge/libvdr-suitebridge.so"
DAEMON_INSTALLED="/usr/sbin/vdr-suite-daemon"
AGENT_INSTALLED="/usr/sbin/vdr-suite-backend-agent"

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
[[ "$SVDRP_PORT" =~ ^[0-9]+$ ]] || fail "svdrp_port_invalid"
(( SVDRP_PORT > 0 && SVDRP_PORT <= 65535 )) || fail "svdrp_port_invalid"
[[ -z "$CA_CERTIFICATE_PATH" || -f "$CA_CERTIFICATE_PATH" ]] || fail "ca_certificate_missing"
if [[ -n "$CURL_CONFIG" ]]; then
    [[ -f "$CURL_CONFIG" ]] || fail "curl_config_missing"
    CURL_CONFIG_MODE="$(stat -c '%a' "$CURL_CONFIG")"
    [[ "${CURL_CONFIG_MODE: -2}" == "00" ]] || fail "curl_config_permissions_too_open"
fi

for command in git systemctl curl python3 sha256sum cmp svdrpsend grep stat pkg-config; do
    require_command "$command"
done

CURRENT_BRANCH="$(git branch --show-current)"
CURRENT_HEAD="$(git rev-parse HEAD)"
[[ "$CURRENT_BRANCH" == "$EXPECTED_BRANCH" ]] || fail "branch_mismatch"
[[ "$CURRENT_HEAD" == "$EXPECTED_HEAD" ]] || fail "head_mismatch"
[[ -z "$(git status --porcelain)" ]] || fail "worktree_not_clean"

VDR_LIBDIR="$(pkg-config --variable=libdir vdr)" || fail "vdr_libdir_failed"
VDR_APIVERSION="$(pkg-config --variable=apiversion vdr)" || fail "vdr_apiversion_failed"
[[ -n "$VDR_LIBDIR" && -n "$VDR_APIVERSION" ]] || fail "vdr_pkgconfig_incomplete"
PLUGIN_INSTALLED="$VDR_LIBDIR/libvdr-suitebridge.so.$VDR_APIVERSION"

for binary in "$DAEMON_CANDIDATE" "$AGENT_CANDIDATE" "$DAEMON_INSTALLED" "$AGENT_INSTALLED"; do
    [[ -x "$binary" ]] || fail "binary_missing_$(basename "$binary")"
done
for plugin in "$PLUGIN_CANDIDATE" "$PLUGIN_INSTALLED"; do
    [[ -f "$plugin" ]] || fail "plugin_missing_$(basename "$plugin")"
done
cmp -s "$DAEMON_CANDIDATE" "$DAEMON_INSTALLED" || fail "installed_daemon_candidate_mismatch"
cmp -s "$AGENT_CANDIDATE" "$AGENT_INSTALLED" || fail "installed_agent_candidate_mismatch"
cmp -s "$PLUGIN_CANDIDATE" "$PLUGIN_INSTALLED" || fail "installed_suitebridge_candidate_mismatch"

[[ "$(systemctl is-active "$VDR_SERVICE" || true)" == active ]] || fail "vdr_not_active"
[[ "$(systemctl is-active "$DAEMON_SERVICE" || true)" == active ]] || fail "daemon_not_active"
[[ "$(systemctl is-active "$AGENT_SERVICE" || true)" == active ]] || fail "agent_not_active"

mkdir -m 0700 "$EVIDENCE_DIR"
printf '%s\n' "$CURRENT_BRANCH" > "$EVIDENCE_DIR/branch"
printf '%s\n' "$CURRENT_HEAD" > "$EVIDENCE_DIR/HEAD"
printf 'vdr=%s\ndaemon=%s\nagent=%s\n' \
    "$(systemctl is-active "$VDR_SERVICE")" \
    "$(systemctl is-active "$DAEMON_SERVICE")" \
    "$(systemctl is-active "$AGENT_SERVICE")" \
    > "$EVIDENCE_DIR/services.txt"
sha256sum \
    "$DAEMON_CANDIDATE" "$AGENT_CANDIDATE" "$PLUGIN_CANDIDATE" \
    "$DAEMON_INSTALLED" "$AGENT_INSTALLED" "$PLUGIN_INSTALLED" \
    > "$EVIDENCE_DIR/binaries.sha256"

svdrpsend -p "$SVDRP_PORT" "PLUG suitebridge CAPS 1" \
    > "$EVIDENCE_DIR/suitebridge-caps.txt"
grep -Fq '"id":"recording-cut-state","state":"available"' \
    "$EVIDENCE_DIR/suitebridge-caps.txt" \
    || fail "recording_cut_state_capability_unavailable"

svdrpsend -p "$SVDRP_PORT" "PLUG suitebridge NCUT CAP 1 start" \
    > "$EVIDENCE_DIR/suitebridge-ncut-cap.txt"
grep -Fq 'vdr-suite-ncut-cap/1 vdr.recording.cut 1 recording-cut enabled suitebridge' \
    "$EVIDENCE_DIR/suitebridge-ncut-cap.txt" \
    || fail "recording_cut_mutation_provider_unavailable"

QUERY_STRING="$(python3 - "$BACKEND_ID" "$RECORDING_ID" <<'PY'
import sys
from urllib.parse import urlencode
print(urlencode({"backend": sys.argv[1], "recordingId": sys.argv[2]}))
PY
)"

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
    > "$EVIDENCE_DIR/marks.json"
curl "${curl_arguments[@]}" \
    "$CONTROL_PLANE_URL/api/vdr/recordings/cut?$QUERY_STRING" \
    > "$EVIDENCE_DIR/cut-preview.json"

python3 - \
    "$BACKEND_ID" "$RECORDING_ID" \
    "$EVIDENCE_DIR/marks.json" "$EVIDENCE_DIR/cut-preview.json" \
    "$EVIDENCE_DIR/preflight.json" <<'PY'
import json
import re
import sys
from pathlib import Path

backend_id, recording_id, marks_path, cut_path, output_path = sys.argv[1:]
marks = json.loads(Path(marks_path).read_text(encoding="utf-8"))
cut = json.loads(Path(cut_path).read_text(encoding="utf-8"))
revision = marks.get("marksRevision", "")

checks = {
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
print(f"MARKS_REVISION={revision}")
print(f"SEQUENCE_COUNT={summary['sequenceCount']}")
print("HTTP_MUTATION=not_executed")
print("NCUT_EXEC=not_executed")
print("SERVICE_MUTATION=not_executed")
PY
