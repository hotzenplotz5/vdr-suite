#!/usr/bin/env bash
# Root-only, offline Media Tools directory configuration after first install.
# The Backend Agent must be stopped by the administrator before running this.
set -Eeuo pipefail
stop() { printf 'MEDIA_TOOLS_CONFIG=STOP %s\n' "$*" >&2; exit 1; }

if [ "$#" -eq 1 ] && [ "$1" = --help ]; then
  echo 'Usage: configure_media_tools_paths.sh [SOURCE_ROOT VIDEO_ROOT]'
  echo 'Default SOURCE_ROOT=/srv/video/Filme VIDEO_ROOT=/srv/vdr/video.00'
  echo 'Stop the Backend Agent before using; restart it only after reviewing the result.'
  exit 0
fi
[ "$#" -eq 0 ] || [ "$#" -eq 2 ] || stop 'provide both paths or neither'
[ "$(id -u)" -eq 0 ] || stop 'root required'
for cmd in realpath stat sudo flock mktemp python3 systemctl find grep id getent; do
  command -v "$cmd" >/dev/null || stop "missing tool $cmd"
done
if [ "$#" -eq 0 ]; then
  SOURCE=/srv/video/Filme
  VIDEO=/srv/vdr/video.00
else
  SOURCE=$1
  VIDEO=$2
fi
BACKEND=$(sed -n 's/^BACKEND_ID=//p' /etc/vdr-suite/backend-agent.conf | head -n 1)
[[ "$BACKEND" =~ ^[A-Za-z0-9_.:-]{1,128}$ ]] &&
  [ "$BACKEND" != . ] && [ "$BACKEND" != .. ] || stop 'backend not configured'

# Do not interrupt an active/authorized Agent job: operator must stop it safely.
systemctl is-active --quiet vdr-suite-backend-agent &&
  stop 'Backend Agent running; stop it safely before reconfiguration'
# Installation evidence is Suite-owned; a Debian package is not required.
# Reuse the existing root-owned approval/handler verifier, for both source
# and packaged installations. Neither grants an execution capability.
python3 - "$(dirname "$0")" <<'PY' || stop 'approved Media Tools handler missing'
import pathlib, sys
sys.path.insert(0, sys.argv[1])
from admin_approval_evidence import examine
result = examine(pathlib.Path("/"), "rectools")
if not result.get("consistent"):
    raise SystemExit("HANDLER_EVIDENCE=DENIED " + result.get("reason", "unknown"))
print("HANDLER_EVIDENCE=PASS")
PY
AGENT_ACCOUNT=$(systemctl show -p User --value vdr-suite-backend-agent) ||
  stop 'cannot inspect Agent service account'
[[ "$AGENT_ACCOUNT" =~ ^[a-z_][a-z0-9_-]*[$]?$ ]] &&
  [ "$AGENT_ACCOUNT" != root ] &&
  id "$AGENT_ACCOUNT" >/dev/null 2>&1 ||
  stop 'unrecognized Backend Agent service account'

for dir in "$SOURCE" "$VIDEO"; do
  [[ "$dir" = /* && "$dir" != *$'\t'* && "$dir" != *$'\n'* ]] ||
    stop 'unsafe media path'
  [ -d "$dir" ] && [ "$(realpath -e "$dir")" = "$dir" ] ||
    stop "directory absent or noncanonical: $dir"
done
[ "$SOURCE" != "$VIDEO" ] || stop 'source equals video root'
[[ "$VIDEO" =~ ^/[a-zA-Z0-9_./-]+$ ]] ||
  stop 'video root incompatible with systemd ReadWritePaths'
case "$SOURCE" in /home/*|/root/*|/run/user/*)
  stop 'source hidden by Backend Agent ProtectHome sandbox' ;;
esac
sudo -u "$AGENT_ACCOUNT" test -r "$SOURCE" || stop 'Agent cannot read source'
sudo -u "$AGENT_ACCOUNT" test -w "$VIDEO" || stop 'Agent cannot write video root'

LEDGER_DIR=/etc/vdr-suite/addon-sources
OVERRIDE_DIR=/etc/systemd/system/vdr-suite-backend-agent.service.d
LEDGER=$LEDGER_DIR/media-import.tsv
OVERRIDE=$OVERRIDE_DIR/20-media-tools.conf
LOCK=/etc/vdr-suite/addon-approvals/rectools.import-running.lock

for dir in /etc/vdr-suite /etc/vdr-suite/addon-approvals "$LEDGER_DIR" "$OVERRIDE_DIR"; do
  [ -d "$dir" ] && [ ! -L "$dir" ] &&
    [ "$(stat -c %u "$dir")" = 0 ] &&
    [ -z "$(find "$dir" -maxdepth 0 -perm /022 -print)" ] ||
    stop "unsafe root-controlled directory: $dir"
done
[ -f "$LOCK" ] && [ ! -L "$LOCK" ] &&
  [ "$(stat -c %u "$LOCK")" = 0 ] || stop 'maintenance lock missing or untrusted'
exec 9<>"$LOCK"
flock -n -x 9 || stop 'Media Tools maintenance in progress'

for file in "$LEDGER" "$OVERRIDE"; do
  if [ -e "$file" ] || [ -L "$file" ]; then
    [ -f "$file" ] && [ ! -L "$file" ] &&
      [ "$(stat -c %u "$file")" = 0 ] &&
      [ "$(stat -c %h "$file")" = 1 ] &&
      [ -z "$(find "$file" -maxdepth 0 -perm /022 -print)" ] ||
      stop "unsafe existing configuration: $file"
  fi
done
if [ -f "$LEDGER" ]; then
  mapfile -t entries < "$LEDGER"
  [ "${#entries[@]}" -eq 1 ] &&
    [[ "${entries[0]}" = "$BACKEND"$'\t'local-media-import$'\t'* ]] ||
    stop 'custom source inventory; refuse to overwrite'
fi
[ -f "$OVERRIDE" ] && grep -Fxq \
  'ExecStart=/usr/sbin/vdr-suite-backend-agent --config /etc/vdr-suite/backend-agent.conf --enable-media-import --enable-media-shrink --media-shrink-state-dir /var/lib/vdr-suite/backend-agent/media-shrink' \
  "$OVERRIDE" || stop 'unrecognized Backend Agent override'

BACKUP=$(mktemp -d /root/vdr-suite-media-path-backup.XXXXXXXX) ||
  stop 'cannot create root backup'
[ ! -f "$LEDGER" ] || cp -p -- "$LEDGER" "$BACKUP/media-import.tsv"
cp -p -- "$OVERRIDE" "$BACKUP/20-media-tools.conf"
echo "CONFIG_BACKUP=$BACKUP"

TEMP_LEDGER=$(mktemp "$LEDGER_DIR/.media-import.tsv.XXXXXXXX") ||
  stop 'cannot stage source inventory'
TEMP_OVERRIDE=$(mktemp "$OVERRIDE_DIR/.20-media-tools.conf.XXXXXXXX") ||
  stop 'cannot stage service override'
trap 'rm -f -- "${TEMP_LEDGER:-}" "${TEMP_OVERRIDE:-}"' EXIT
printf '%s\tlocal-media-import\t%s\t%s\t*\n' \
  "$BACKEND" "$SOURCE" "$VIDEO" > "$TEMP_LEDGER"
printf '%s\n' \
  '[Service]' \
  'ExecStart=' \
  'ExecStart=/usr/sbin/vdr-suite-backend-agent --config /etc/vdr-suite/backend-agent.conf --enable-media-import --enable-media-shrink --media-shrink-state-dir /var/lib/vdr-suite/backend-agent/media-shrink' \
  "ReadWritePaths=$VIDEO" \
  "ReadWritePaths=$SOURCE" > "$TEMP_OVERRIDE"
chmod 0644 "$TEMP_LEDGER" "$TEMP_OVERRIDE"
chown root:root "$TEMP_LEDGER" "$TEMP_OVERRIDE"

# Agent stays stopped across both changes; the administrator restarts it.
mv -T -- "$TEMP_LEDGER" "$LEDGER" || stop 'cannot publish source inventory'
mv -T -- "$TEMP_OVERRIDE" "$OVERRIDE" || stop 'cannot publish service override'
systemctl daemon-reload || stop 'systemd reload failed (Agent remains stopped)'

echo "MEDIA_TOOLS_SOURCE=$SOURCE"
echo "MEDIA_TOOLS_VIDEO=$VIDEO"
echo 'MEDIA_TOOLS_CONFIG=PASS'
echo 'BACKEND_AGENT_RESTART=MANUAL_AFTER_REVIEW'
