#!/usr/bin/env bash

fail()
{
    printf 'FAIL: %s\n' "$1" >&2
    exit 1
}

[[ "${EUID:-$(id -u)}" -eq 0 ]] || fail must_run_as_root

EXPECTED_BRANCH="${PHASE63_EXPECTED_BRANCH:-agent/phase63-local-provider-selection-runtime}"
EXPECTED_HEAD="${PHASE63_EXPECTED_HEAD:-}"
BACKEND_ID="${PHASE63_BACKEND_ID:-default}"
DATABASE="${PHASE63_DATABASE:-/var/lib/vdr-suite/vdr-suite.db}"
CURRENT_BRANCH="$(git branch --show-current)" || fail branch_read_failed
CURRENT_HEAD="$(git rev-parse HEAD)" || fail head_read_failed
[[ "$CURRENT_BRANCH" == "$EXPECTED_BRANCH" ]] || fail unexpected_branch
if [[ -n "$EXPECTED_HEAD" ]]; then
    [[ "$CURRENT_HEAD" == "$EXPECTED_HEAD" ]] || fail unexpected_head
fi
[[ -z "$(git status --porcelain)" ]] || fail worktree_not_clean
SHORT_HEAD="$(printf '%s' "$CURRENT_HEAD" | cut -c1-8)" || fail short_head_failed
EVIDENCE_DIR="${PHASE63_PROVIDER_EVIDENCE_DIR:-/root/vdr-suite-phase63-local-provider-selection-acceptance-${SHORT_HEAD}}"
[[ ! -e "$EVIDENCE_DIR" ]] || fail evidence_directory_already_exists
mkdir -m 0700 "$EVIDENCE_DIR" || fail evidence_directory_create_failed
NATIVE_EVIDENCE_DIR="$EVIDENCE_DIR/native-runtime"
ADMIN_BUILD=".build/vdr-suite-backend-agent-command-admin"
HOST_AGENT_CONFIG="${PHASE63_AGENT_CONFIG_PATH:-/etc/vdr-suite/backend-agent.conf}"
HOST_AGENT_CONFIG_BACKUP="$EVIDENCE_DIR/backend-agent.host.original.conf"
ACCEPTANCE_AGENT_CONFIG="${PHASE63_ACCEPTANCE_AGENT_CONFIG_PATH:-/run/vdr-suite-phase63-provider-acceptance-${SHORT_HEAD}.conf}"
ACCEPTANCE_CONFIG_CREATED=0
OWNER_TOUCHED=0
OWNER_RESTORED=0

build_admin()
{
    make backend-agent-command-admin >"$EVIDENCE_DIR/provider-admin-build.log" 2>&1 || return 1
    [[ -x "$ADMIN_BUILD" ]]
}

provider_status()
{
    "$ADMIN_BUILD" \
        --database "$DATABASE" \
        --backend "$BACKEND_ID" \
        --provider-ownership-status
}

provider_active()
{
    python3 - "$1" <<'PY_PROVIDER_ACTIVE'
import json
import sys
payload=json.loads(sys.argv[1])
raise SystemExit(0 if payload.get("active") is True else 1)
PY_PROVIDER_ACTIVE
}

provider_inactive()
{
    python3 - "$1" <<'PY_PROVIDER_INACTIVE'
import json
import sys
payload=json.loads(sys.argv[1])
raise SystemExit(0 if payload.get("active") is False else 1)
PY_PROVIDER_INACTIVE
}

prepare_acceptance_agent_config()
{
    [[ -f "$HOST_AGENT_CONFIG" && ! -L "$HOST_AGENT_CONFIG" ]] || return 1
    [[ "$ACCEPTANCE_AGENT_CONFIG" == /* ]] || return 1
    [[ ! -e "$ACCEPTANCE_AGENT_CONFIG" ]] || return 1
    cp -a "$HOST_AGENT_CONFIG" "$HOST_AGENT_CONFIG_BACKUP" || return 1
    install -o root -g root -m 0644 "$HOST_AGENT_CONFIG" "$ACCEPTANCE_AGENT_CONFIG" || return 1
    ACCEPTANCE_CONFIG_CREATED=1
    python3 - "$ACCEPTANCE_AGENT_CONFIG" <<'PY_ACCEPTANCE_CONFIG' || return 1
from pathlib import Path
import os
import sys

path = Path(sys.argv[1])
default_state_path = "/var/lib/vdr-suite/backend-agent/commands.state"
lines = path.read_text(encoding="utf-8").splitlines()
found = False
output = []
for raw in lines:
    stripped = raw.strip()
    if not stripped or stripped.startswith("#"):
        output.append(raw)
        continue
    key, separator, value = raw.partition("=")
    if not separator:
        raise SystemExit(1)
    if key.strip() == "COMMAND_STATE_PATH":
        if found:
            raise SystemExit(1)
        found = True
        output.append(raw if value else f"COMMAND_STATE_PATH={default_state_path}")
    else:
        output.append(raw)
if not found:
    if output and output[-1] != "":
        output.append("")
    output.append(f"COMMAND_STATE_PATH={default_state_path}")
path.write_text("\n".join(output) + "\n", encoding="utf-8")
os.chmod(path, 0o644)
PY_ACCEPTANCE_CONFIG
}

cleanup()
{
    local exit_code="$?"
    trap - EXIT INT TERM
    if [[ "$OWNER_TOUCHED" -eq 1 ]]; then
        if build_admin; then
            if "$ADMIN_BUILD" \
                --database "$DATABASE" \
                --backend "$BACKEND_ID" \
                --clear-native-probe-owner \
                >"$EVIDENCE_DIR/provider-owner-cleanup.json" 2>"$EVIDENCE_DIR/provider-owner-cleanup.err"; then
                local cleanup_status
                cleanup_status="$(provider_status 2>"$EVIDENCE_DIR/provider-owner-cleanup-status.err")"
                printf '%s\n' "$cleanup_status" >"$EVIDENCE_DIR/provider-owner-cleanup-status.json"
                if provider_inactive "$cleanup_status"; then
                    OWNER_RESTORED=1
                    OWNER_TOUCHED=0
                else
                    exit_code=1
                fi
            else
                exit_code=1
            fi
        else
            exit_code=1
        fi
    fi
    if [[ "$OWNER_RESTORED" -ne 1 && "$OWNER_TOUCHED" -eq 1 ]]; then
        printf 'FAIL: provider_ownership_restore_failed\n' >&2
    fi
    if [[ -f "$HOST_AGENT_CONFIG_BACKUP" ]] && ! cmp -s "$HOST_AGENT_CONFIG_BACKUP" "$HOST_AGENT_CONFIG"; then
        printf 'FAIL: host_agent_configuration_changed\n' >&2
        exit_code=1
    fi
    if [[ "$ACCEPTANCE_CONFIG_CREATED" -eq 1 ]]; then
        if rm -f "$ACCEPTANCE_AGENT_CONFIG" && [[ ! -e "$ACCEPTANCE_AGENT_CONFIG" ]]; then
            ACCEPTANCE_CONFIG_CREATED=0
        else
            printf 'FAIL: acceptance_agent_config_cleanup_failed\n' >&2
            exit_code=1
        fi
    fi
    exit "$exit_code"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

prepare_acceptance_agent_config || fail acceptance_agent_config_prepare_failed
build_admin || fail provider_admin_build_failed
ORIGINAL_STATUS="$(provider_status 2>"$EVIDENCE_DIR/provider-owner-original.err")" || fail provider_owner_status_failed
printf '%s\n' "$ORIGINAL_STATUS" >"$EVIDENCE_DIR/provider-owner-original.json"
if provider_active "$ORIGINAL_STATUS"; then
    fail preexisting_active_provider_ownership_not_supported
fi
provider_inactive "$ORIGINAL_STATUS" || fail invalid_original_provider_ownership_status

"$ADMIN_BUILD" \
    --database "$DATABASE" \
    --backend "$BACKEND_ID" \
    --set-native-probe-owner \
    >"$EVIDENCE_DIR/provider-owner-set.json" \
    2>"$EVIDENCE_DIR/provider-owner-set.err" || fail provider_owner_set_failed
OWNER_TOUCHED=1
SET_STATUS="$(provider_status 2>"$EVIDENCE_DIR/provider-owner-set-status.err")" || fail provider_owner_set_status_failed
printf '%s\n' "$SET_STATUS" >"$EVIDENCE_DIR/provider-owner-set-status.json"
provider_active "$SET_STATUS" || fail provider_owner_not_active
python3 - "$SET_STATUS" <<'PY_PROVIDER_OWNER'
import json
import sys
payload=json.loads(sys.argv[1])
assert payload.get("authorityDomain") == "vdr.native"
assert payload.get("providerId") == "suitebridge:local"
assert payload.get("providerKind") == "suitebridge"
assert payload.get("allowedCapabilities") == ["vdr.native.probe"]
assert isinstance(payload.get("ownershipGeneration"), int)
assert payload["ownershipGeneration"] > 0
PY_PROVIDER_OWNER
[[ "$?" -eq 0 ]] || fail provider_owner_contract_mismatch

PHASE63_EXPECTED_BRANCH="$CURRENT_BRANCH" \
PHASE63_EXPECTED_HEAD="$CURRENT_HEAD" \
PHASE63_EVIDENCE_DIR="$NATIVE_EVIDENCE_DIR" \
PHASE63_BACKEND_ID="$BACKEND_ID" \
PHASE63_DATABASE="$DATABASE" \
PHASE63_VDR_VIDEO_DIR="${PHASE63_VDR_VIDEO_DIR:-/srv/vdr/video.00}" \
PHASE63_AGENT_CONFIG_PATH="$ACCEPTANCE_AGENT_CONFIG" \
PHASE63_DAEMON_SERVICE="${PHASE63_DAEMON_SERVICE:-vdr-suite-daemon}" \
PHASE63_AGENT_SERVICE="${PHASE63_AGENT_SERVICE:-vdr-suite-backend-agent}" \
PHASE63_VDR_SERVICE="${PHASE63_VDR_SERVICE:-vdr}" \
PHASE63_SVDRP_PORT="${PHASE63_SVDRP_PORT:-6419}" \
PHASE63_PROXY_PORT="${PHASE63_PROXY_PORT:-16419}" \
bash tools/run_phase63_fenced_native_operation_acceptance.sh \
    >"$EVIDENCE_DIR/native-runtime.log" 2>&1
NATIVE_RC="$?"
cat "$EVIDENCE_DIR/native-runtime.log"
[[ "$NATIVE_RC" -eq 0 ]] || fail nested_native_acceptance_failed

for marker in \
    'PHASE_63_FENCED_NATIVE_OPERATION_ACCEPTANCE=PASS' \
    'CONTROL_PLANE_REPLAY_NO_NATIVE_REEXECUTION=yes' \
    'PLUGIN_EPOCH_REPLAY_FENCED=yes' \
    'NATIVE_RECEIPT_EVIDENCE_SEPARATE=yes' \
    'NATIVE_RESULT_EVIDENCE_SEPARATE=yes' \
    'AUTHORITATIVE_READBACK_VERIFIED=yes' \
    'VDR_NATIVE_STATE_UNCHANGED=yes' \
    'EXISTING_AGENT_IDENTITY_PRESERVED=yes' \
    'CREDENTIAL_GENERATION_PRESERVED=yes' \
    'ORIGINAL_CONFIGURATION_RESTORED=yes' \
    'VDR_ACTIVE=yes' \
    'DAEMON_ACTIVE=yes' \
    'AGENT_ACTIVE=yes'
do
    grep -Fqx "$marker" "$EVIDENCE_DIR/native-runtime.log" || fail "nested_marker_missing_${marker%%=*}"
done

cmp -s "$HOST_AGENT_CONFIG_BACKUP" "$HOST_AGENT_CONFIG" || fail host_agent_configuration_changed

python3 - "$DATABASE" "$BACKEND_ID" "$EVIDENCE_DIR/provider-selection-db.json" <<'PY_PROVIDER_DB'
import json
import sqlite3
import sys

database, backend_id, output = sys.argv[1:]
connection = sqlite3.connect(f"file:{database}?mode=ro", uri=True)
try:
    rows = connection.execute(
        """
        SELECT c.command_id,c.payload_version,s.authority_domain,s.provider_id,
               s.provider_kind,s.ownership_generation,s.provider_instance_epoch,
               s.provider_generation,s.capability_revision,s.required_capability,
               s.selection_identity
          FROM backend_agent_commands c
          JOIN backend_agent_command_provider_selections s
            ON s.command_id=c.command_id
         WHERE c.backend_id=? AND c.command_type='vdr.native.probe'
         ORDER BY c.assigned_at
        """,
        (backend_id,),
    ).fetchall()
finally:
    connection.close()
if not rows:
    raise SystemExit("no persisted provider selections")
for row in rows:
    assert row[1] == 2
    assert row[2] == "vdr.native"
    assert row[3] == "suitebridge:local"
    assert row[4] == "suitebridge"
    assert row[5] > 0
    assert row[6]
    assert row[7] == 1
    assert row[8] == 1
    assert row[9] == "vdr.native.probe"
    assert row[10].startswith("local-provider-selection/1|")
payload = {
    "backendId": backend_id,
    "selectionCount": len(rows),
    "allPayloadVersion2": all(row[1] == 2 for row in rows),
    "providerId": "suitebridge:local",
    "providerKind": "suitebridge",
    "authorityDomain": "vdr.native",
    "requiredCapability": "vdr.native.probe",
    "ownershipGenerations": sorted({row[5] for row in rows}),
    "pluginInstanceEpochs": sorted({row[6] for row in rows}),
}
with open(output, "w", encoding="utf-8") as handle:
    json.dump(payload, handle, sort_keys=True, indent=2)
    handle.write("\n")
PY_PROVIDER_DB
[[ "$?" -eq 0 ]] || fail provider_selection_database_verification_failed

build_admin || fail provider_admin_rebuild_failed
"$ADMIN_BUILD" \
    --database "$DATABASE" \
    --backend "$BACKEND_ID" \
    --clear-native-probe-owner \
    >"$EVIDENCE_DIR/provider-owner-clear.json" \
    2>"$EVIDENCE_DIR/provider-owner-clear.err" || fail provider_owner_clear_failed
CLEAR_STATUS="$(provider_status 2>"$EVIDENCE_DIR/provider-owner-clear-status.err")" || fail provider_owner_clear_status_failed
printf '%s\n' "$CLEAR_STATUS" >"$EVIDENCE_DIR/provider-owner-clear-status.json"
provider_inactive "$CLEAR_STATUS" || fail provider_owner_not_restored_inactive
OWNER_TOUCHED=0
OWNER_RESTORED=1

cmp -s "$HOST_AGENT_CONFIG_BACKUP" "$HOST_AGENT_CONFIG" || fail host_agent_configuration_changed
rm -f "$ACCEPTANCE_AGENT_CONFIG" || fail acceptance_agent_config_cleanup_failed
[[ ! -e "$ACCEPTANCE_AGENT_CONFIG" ]] || fail acceptance_agent_config_cleanup_failed
ACCEPTANCE_CONFIG_CREATED=0
[[ -z "$(git status --porcelain)" ]] || fail acceptance_changed_worktree

printf '%s\n' \
    'PHASE_63_LOCAL_PROVIDER_SELECTION_ACCEPTANCE=PASS' \
    "HEAD=$CURRENT_HEAD" \
    'PROVIDER_OWNERSHIP_EXPLICIT=yes' \
    'PROVIDER_AUTHORITY_DOMAIN=vdr.native' \
    'SELECTED_PROVIDER_ID=suitebridge:local' \
    'SELECTED_PROVIDER_KIND=suitebridge' \
    'SELECTED_CAPABILITY=vdr.native.probe' \
    'NATIVE_PROBE_PAYLOAD_V2_PERSISTED=yes' \
    'PROVIDER_SELECTION_SIDECAR_PERSISTED=yes' \
    'PROVIDER_OWNERSHIP_RESTORED=inactive' \
    'MUTATIONS_REMAIN_DISABLED=yes' \
    'VDR_NATIVE_STATE_UNCHANGED=yes' \
    'EXISTING_AGENT_IDENTITY_PRESERVED=yes' \
    'CREDENTIAL_GENERATION_PRESERVED=yes' \
    'ORIGINAL_CONFIGURATION_RESTORED=yes' \
    'HOST_AGENT_CONFIGURATION_UNCHANGED=yes' \
    'ACCEPTANCE_AGENT_CONFIG_REMOVED=yes' \
    'VDR_ACTIVE=yes' \
    'DAEMON_ACTIVE=yes' \
    'AGENT_ACTIVE=yes' \
    "EVIDENCE=$EVIDENCE_DIR"
