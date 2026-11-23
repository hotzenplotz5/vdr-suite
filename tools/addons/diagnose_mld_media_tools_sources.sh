#!/bin/sh
# VDR-Suite Media-Tools / MLD: read-only source-settings diagnostic.
# Never prints configured paths, backend identifiers or agent credentials.
LC_ALL=C
export LC_ALL

printf '%s\n' '=== VDR-SUITE MLD MEDIA-TOOLS DIAG V1 ==='
printf 'TIME: '; date '+%Y-%m-%d %H:%M:%S %z'
printf 'USER: '; if [ "$(id -u)" = 0 ]; then echo ROOT; else echo NONROOT; fi

printf '\n%s\n' '=== SERVICES ==='
if command -v systemctl >/dev/null 2>&1; then
    for service in vdr-suite-daemon vdr-suite-backend-agent; do
        state=$(systemctl is-active "$service" 2>/dev/null)
        [ -n "$state" ] || state=unknown
        printf '%s: %s\n' "$service" "$state"
    done
    pid=$(systemctl show vdr-suite-daemon -p MainPID --value 2>/dev/null)
    case "$pid" in
        ''|0|*[!0-9]*) echo 'DAEMON_EUID: unavailable' ;;
        *)
            if [ -r "/proc/$pid/status" ]; then
                awk '/^Uid:/ {print "DAEMON_EUID: " $3; found=1} END {if (!found) print "DAEMON_EUID: unavailable"}' "/proc/$pid/status"
            else
                echo 'DAEMON_EUID: unavailable'
            fi
            ;;
    esac
else
    echo 'SYSTEMCTL: unavailable'
fi

printf '\n%s\n' '=== CONFIG METADATA (NO CONTENTS) ==='
for path in \
    /etc/vdr-suite \
    /etc/vdr-suite/addon-sources \
    /etc/vdr-suite/addon-approvals \
    /etc/vdr-suite/backend-agent.conf \
    /etc/vdr-suite/addon-sources/media-import.tsv; do
    if [ -e "$path" ] || [ -L "$path" ]; then
        metadata=$(stat -c 'uid=%u mode=%a links=%h bytes=%s type=%F' "$path" 2>/dev/null)
        [ -n "$metadata" ] || metadata=STAT_FAILED
        printf '%s: %s\n' "$path" "$metadata"
    else
        printf '%s: MISSING\n' "$path"
    fi
done

conf=/etc/vdr-suite/backend-agent.conf
ledger=/etc/vdr-suite/addon-sources/media-import.tsv
backend=''
if [ -r "$conf" ]; then
    backend=$(awk '/^BACKEND_ID=/{count++; value=substr($0,12)} END{if(count==1)printf "%s",value}' "$conf" 2>/dev/null)
fi
if [ -n "$backend" ]; then
    echo 'BACKEND_ID: ONE_NONEMPTY_VALUE'
else
    echo 'BACKEND_ID: MISSING_EMPTY_OR_DUPLICATE'
fi

printf '\n%s\n' '=== SOURCE INVENTORY (NO PATHS) ==='
if [ ! -r "$ledger" ]; then
    echo 'LEDGER: MISSING_OR_UNREADABLE'
else
    size=$(stat -c %s "$ledger" 2>/dev/null)
    case "$size" in
        ''|*[!0-9]*) echo 'LEDGER_SIZE: UNKNOWN' ;;
        *) if [ "$size" -ge 32768 ]; then echo 'LEDGER_SIZE: TOO_LARGE'; else echo 'LEDGER_SIZE: OK'; fi ;;
    esac
    lastbyte=$(tail -c 1 "$ledger" 2>/dev/null | od -An -tu1 | tr -d '[:space:]')
    if [ "$lastbyte" = 10 ]; then
        echo 'TRAILING_NEWLINE: PASS'
    else
        echo 'TRAILING_NEWLINE: FAIL'
    fi
    case "$size" in
        ''|*[!0-9]*) echo 'LEDGER_FORMAT: NOT_CHECKED' ;;
        *) if [ "$size" -ge 32768 ]; then
               echo 'LEDGER_FORMAT: NOT_CHECKED'
           else
               awk -F '\t' -v backend="$backend" '
                   BEGIN {valid=1; matchid=1; root=""; n=0}
                   {
                       n++
                       if (NF != 5 && NF != 6) valid=0
                       if (backend == "" || $1 != backend) matchid=0
                       expected=(n==1 ? "local-media-import" : sprintf("local-media-root-%04d",n-1))
                       if ($2 != expected || $5 != "*") valid=0
                       if ($3 !~ /^\// || $4 !~ /^\// || $3 ~ /\/$/ || $4 ~ /\/$/ || $3 ~ /\/\// || $4 ~ /\/\//) valid=0
                       if (NF==6 && ($6=="" || $6 ~ /[\/\\\"]+/)) valid=0
                       if (n==1) root=$4
                       else if ($4 != root) valid=0
                   }
                   END {
                       printf "ENTRY_COUNT: %d\n",n
                       printf "LEDGER_FORMAT: %s\n",(valid && n>=1 && n<=17 ? "BASIC_PASS" : "FAIL")
                       printf "BACKEND_MATCH: %s\n",(matchid && n>=1 ? "PASS" : "FAIL")
                   }
               ' "$ledger"
           fi ;;
    esac
fi

printf '\n%s\n' '=== WEB FRONTEND ==='
frontend=/usr/share/vdr-suite/web/frontend/settings-addons.js
if [ -r "$frontend" ]; then
    if grep -Fq '/api/v1/addons/rectools/sources' "$frontend"; then
        echo 'SOURCE_EDITOR: PRESENT'
    else
        echo 'SOURCE_EDITOR: ENDPOINT_NOT_FOUND'
    fi
else
    echo 'SOURCE_EDITOR: FILE_MISSING_OR_UNREADABLE'
fi

printf '\n%s\n' '=== AGENT LOG SUMMARY ==='
if command -v journalctl >/dev/null 2>&1; then
    journalctl -u vdr-suite-backend-agent --since '24 hours ago' -n 250 -o cat --no-pager 2>/dev/null |
        awk '
            /BACKEND_AGENT_CURL_DIAG=PERFORM_FAILED code=7/ {connection++}
            /Backend Agent synchronization failed: protected_transport_failed/ {transport++}
            /Backend Agent synchronized/ {synced++}
            END {
                printf "CURL_CONNECT_FAILURES_IN_SAMPLE: %d\n",connection
                printf "TRANSPORT_FAILURES_IN_SAMPLE: %d\n",transport
                printf "SYNC_SUCCESSES_IN_SAMPLE: %d\n",synced
            }
        '
else
    echo 'JOURNALCTL: UNAVAILABLE'
fi
printf '\n%s\n' '=== END OF READ-ONLY DIAG ==='
