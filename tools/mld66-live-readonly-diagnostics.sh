#!/bin/sh
# MLD.F1.4: Live-TV authority diagnostics, read-only and safe to share.
# sh tools/mld66-live-readonly-diagnostics.sh [--caps]
# No service changes, no SQL writes, no HTTP/API requests, no raw logs or secrets.
set -eu

case "$#" in
    0) caps=0 ;;
    1)
        if [ "$1" = "--caps" ]; then caps=1
        else echo "Usage: sh $0 [--caps]" >&2; exit 2
        fi ;;
    *) echo "Usage: sh $0 [--caps]" >&2; exit 2 ;;
esac

echo "MLD_LIVE_DIAG=READ_ONLY"
echo "LIVE_ERROR=live_backend_authority_required"
printf 'ARCH='
uname -m
if [ -r /etc/os-release ]; then
    sed -n '/^ID=/p; /^VERSION_ID=/p' /etc/os-release
fi

for cmd in vdr systemctl ps sqlite3 timeout svdrpsend.sh ffmpeg; do
    if command -v "$cmd" >/dev/null 2>&1; then
        printf 'COMMAND_%s=PRESENT\n' "$cmd"
    else
        printf 'COMMAND_%s=ABSENT\n' "$cmd"
    fi
done

if command -v systemctl >/dev/null 2>&1; then
    for unit in vdr.service vdr-suite-daemon.service vdr-suite-backend-agent.service; do
        printf 'UNIT=%s\n' "$unit"
        # Only these non-secret properties are emitted; never ExecStart/Environment.
        status=$(systemctl show "$unit" \
            -p LoadState -p ActiveState -p SubState -p UnitFileState \
            --no-pager 2>/dev/null) || status=""
        if [ -z "$status" ]; then
            echo "UNIT_STATUS=UNAVAILABLE"
        else
            printf '%s\n' "$status" | sed -n \
                '/^LoadState=/p; /^ActiveState=/p; /^SubState=/p; /^UnitFileState=/p'
        fi
    done
else
    echo "SERVICE_MANAGER=SYSTEMCTL_UNAVAILABLE"
fi

if command -v ps >/dev/null 2>&1; then
    names=$(ps -eo comm= 2>/dev/null) || names=""
    if [ -n "$names" ]; then
        # Process names only, never command-line arguments or environments.
        for match in vdr vdr-suite-daemon vdr-suite-backend-agent; do
            if printf '%s\n' "$names" | grep -Fx "$match" >/dev/null 2>&1; then
                printf 'PROCESS_%s=RUNNING\n' "$match"
            else
                printf 'PROCESS_%s=NOT_SEEN_OR_TRUNCATED\n' "$match"
            fi
        done
    else
        echo "PROCESS_NAMES=UNAVAILABLE"
    fi
fi

# A SQLite read-only query avoids the admin --status command: that command
# performs normal ensureSchema()/technical-identity initialization.
db=/var/lib/vdr-suite/vdr-suite.db
if [ ! -r "$db" ]; then
    echo "AGENT_DB=NOT_READABLE_OR_NOT_AT_DEFAULT_PATH"
elif ! command -v sqlite3 >/dev/null 2>&1; then
    echo "AGENT_DB=SQLITE3_UNAVAILABLE"
else
    # Output only backend IDs and derived boolean/health classifications:
    # never print agent IDs, enrollment material, credentials, or DB rows.
    query="PRAGMA query_only=ON;
SELECT 'BACKEND=' || backend_id ||
       ' AGENT=' ||
       CASE
         WHEN revoked_at != 0 THEN 'REVOKED'
         WHEN incompatible != 0 THEN 'INCOMPATIBLE'
         WHEN backend_generation = 0 OR agent_instance_id = '' THEN 'NOT_READY'
         WHEN lease_expires_at <= CAST(strftime('%s','now') AS INTEGER)
              THEN 'LEASE_EXPIRED'
         ELSE 'LEASE_CURRENT'
       END ||
       ' CHANNEL_CURSOR=' ||
       CASE WHEN EXISTS (
          SELECT 1 FROM backend_agent_observation_cursors c
          WHERE c.backend_id = a.backend_id
            AND c.observation_domain = 'channels'
            AND c.agent_instance_id = a.agent_instance_id
            AND c.backend_generation = a.backend_generation
            AND c.snapshot_generation > 0
            AND c.producer_sequence > 0
            AND c.resource_revision <> ''
       ) THEN 'PRESENT' ELSE 'MISSING_OR_STALE' END
FROM backend_agents a
WHERE revoked_at = 0
ORDER BY backend_id;"
    agent_status=$(sqlite3 -readonly -batch -noheader "$db" "$query" 2>/dev/null) ||
        agent_status="UNAVAILABLE_SCHEMA_OR_ACCESS"
    if [ -z "$agent_status" ]; then
        echo "AGENT_DB=NO_ACTIVE_AGENT_ROWS"
    else
        printf '%s\n' "$agent_status"
    fi
fi

# This is optional because it talks to the live VDR SVDRP interface.
# Only the existing read-only SuiteBridge CAPS command is used with a hard
# timeout. No stream, timer, recording or provider is opened.
if [ "$caps" -eq 1 ]; then
    if command -v timeout >/dev/null 2>&1 &&
        command -v svdrpsend.sh >/dev/null 2>&1; then
        echo "SUITEBRIDGE_CAPS=BEGIN"
        if caps_reply=$(timeout 5 svdrpsend.sh plug suitebridge CAPS 1 2>/dev/null); then
            printf '%s\n' "$caps_reply" | sed -n '1,12p'
            echo "SUITEBRIDGE_CAPS=RESPONSE_ABOVE"
        else
            echo "SUITEBRIDGE_CAPS=FAILED_OR_TIMED_OUT"
        fi
        echo "SUITEBRIDGE_CAPS=END"
    else
        echo "SUITEBRIDGE_CAPS=SKIPPED_MISSING_TIMEOUT_OR_MLD_SVDRP_TOOL"
    fi
else
    echo "SUITEBRIDGE_CAPS=NOT_REQUESTED"
fi

echo "MLD_LIVE_DIAG=END"
