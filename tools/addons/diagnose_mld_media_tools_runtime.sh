#!/bin/sh
# MLD Media Tools: read-only runtime wiring check, no paths/secrets printed.
LC_ALL=C
export LC_ALL
printf '%s\n' '=== VDR-SUITE MLD MEDIA-TOOLS RUNTIME DIAG V1 ==='
printf 'TIME: '; date '+%Y-%m-%d %H:%M:%S %z'
if [ "$(id -u)" = 0 ]; then echo 'USER: ROOT'; else echo 'USER: NONROOT'; fi

printf '\n%s\n' '=== AGENT OPT-IN ==='
if command -v systemctl >/dev/null 2>&1; then
  state=$(systemctl is-active vdr-suite-backend-agent 2>/dev/null)
  [ -n "$state" ] || state=unknown
  printf 'AGENT_SERVICE: %s\n' "$state"
  pid=$(systemctl show vdr-suite-backend-agent -p MainPID --value 2>/dev/null)
  case "$pid" in
    ''|0|*[!0-9]*) echo 'RUNNING_AGENT_OPT_IN: UNKNOWN' ;;
    *)
      if [ -r "/proc/$pid/cmdline" ]; then
        if tr '\000' '\n' < "/proc/$pid/cmdline" | grep -Fqx -- '--enable-media-import'; then
          echo 'RUNNING_AGENT_OPT_IN: YES'
        else
          echo 'RUNNING_AGENT_OPT_IN: NO'
        fi
      else
        echo 'RUNNING_AGENT_OPT_IN: UNKNOWN'
      fi ;;
  esac
  effective=$(systemctl show vdr-suite-backend-agent -p ExecStart --value 2>/dev/null)
  if [ -z "$effective" ]; then
    echo 'UNIT_MEDIA_IMPORT_OPT_IN: UNKNOWN'
  elif printf '%s\n' "$effective" | grep -Fq -- '--enable-media-import'; then
    echo 'UNIT_MEDIA_IMPORT_OPT_IN: YES'
  else
    echo 'UNIT_MEDIA_IMPORT_OPT_IN: NO'
  fi
else
  echo 'SYSTEMCTL: NOT_AVAILABLE'
fi

printf '\n%s\n' '=== ADD-ON FILES / APPROVAL PRESENCE ==='
for name in vdr-suite-media-import vdr-suite-media-check vdr-suite-media-shrink; do
  path="/usr/libexec/vdr-suite/addons/rectools/$name"
  if [ -f "$path" ] && [ -x "$path" ]; then
    printf '%s: EXECUTABLE_PRESENT\n' "$name"
  else
    printf '%s: MISSING_OR_NOT_EXECUTABLE\n' "$name"
  fi
done
for path in \
  /usr/libexec/vdr-suite/addons/rectools/addon.json \
  /usr/share/vdr-suite/addons/rectools/media-tools-install-inventory.json \
  /etc/vdr-suite/addon-approvals/rectools.json; do
  if [ -f "$path" ]; then
    printf '%s: PRESENT\n' "$path"
  else
    printf '%s: MISSING\n' "$path"
  fi
done
backend=''
conf=/etc/vdr-suite/backend-agent.conf
if [ -r "$conf" ]; then
  backend=$(awk '/^BACKEND_ID=/{n++;v=substr($0,12)} END{if(n==1) printf "%s",v}' "$conf")
fi
case "$backend" in
  ''|*[!a-zA-Z0-9_.:-]*)
    echo 'LOCAL_BACKEND_ID: INVALID_OR_MISSING'
    echo 'BACKEND_APPROVAL: NOT_CHECKED' ;;
  *)
    echo 'LOCAL_BACKEND_ID: PRESENT'
    if [ -f "/etc/vdr-suite/addon-approvals/rectools.$backend.json" ]; then
      echo 'BACKEND_APPROVAL: PRESENT'
    else
      echo 'BACKEND_APPROVAL: MISSING'
    fi ;;
esac

printf '\n%s\n' '=== SOURCE PATH EXISTENCE (NO PATHS) ==='
ledger=/etc/vdr-suite/addon-sources/media-import.tsv
if [ -r "$ledger" ]; then
  src=$(awk -F '\t' 'NR==1{print $3}' "$ledger")
  dst=$(awk -F '\t' 'NR==1{print $4}' "$ledger")
  if [ -n "$src" ] && [ -d "$src" ]; then echo 'IMPORT_DIRECTORY: EXISTS'; else echo 'IMPORT_DIRECTORY: MISSING'; fi
  if [ -n "$dst" ] && [ -d "$dst" ]; then echo 'RECORDING_DIRECTORY: EXISTS'; else echo 'RECORDING_DIRECTORY: MISSING'; fi
else
  echo 'SOURCE_LEDGER: MISSING_OR_UNREADABLE'
fi

printf '\n%s\n' '=== AGENT LOG COUNTS (LAST 24 HOURS) ==='
if command -v journalctl >/dev/null 2>&1; then
  journalctl -u vdr-suite-backend-agent --since '24 hours ago' \
    -n 1000 -o cat --no-pager 2>/dev/null |
    awk '
      /Media Import inventory observation unavailable/ {obs++}
      /MEDIA_IMPORT_HEARTBEAT_UNAVAILABLE=/ {heartbeat++}
      /Backend Agent synchronized/ {synced++}
      /Backend Agent heartbeat failed: accountability_unavailable/ {audit++}
      END {
        printf "SOURCE_OBSERVATION_FAILURES: %d\n",obs
        printf "IMPORT_HEARTBEAT_FAILURES: %d\n",heartbeat
        printf "SYNC_SUCCESSES: %d\n",synced
        printf "ACCOUNTABILITY_FAILURES: %d\n",audit
      }'
else
  echo 'JOURNALCTL: NOT_AVAILABLE'
fi
printf '\n%s\n' '=== DAEMON ACCOUNTABILITY SQLITE FAILURES (LAST 24 HOURS) ==='
if command -v journalctl >/dev/null 2>&1; then
  journalctl -u vdr-suite-daemon --since '24 hours ago' \
    -n 4000 -o cat --no-pager 2>/dev/null |
    awk '
      /accountability append .* failed: sqlite_rc=/ {
        code=$0
        sub(/^.*sqlite_rc=/, "", code)
        sub(/[^0-9].*$/, "", code)
        count++
        if (code == "5") busy++
        else if (code == "6") locked++
        else if (code == "19") constraint++
        else other++
      }
      /database is locked/ {lockedText++}
      END {
        printf "DAEMON_ACCOUNTABILITY_APPEND_FAILURES: %d\n",count
        printf "SQLITE_BUSY_RC5: %d\n",busy
        printf "SQLITE_LOCKED_RC6: %d\n",locked
        printf "SQLITE_CONSTRAINT_RC19: %d\n",constraint
        printf "SQLITE_OTHER_RC: %d\n",other
        printf "DAEMON_LOCKED_TEXT_LINES: %d\n",lockedText
      }'
else
  echo 'DAEMON_LOGS: NOT_AVAILABLE'
fi

printf '%s\n' '=== END OF READ-ONLY DIAG ==='
