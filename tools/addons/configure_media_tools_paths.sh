#!/usr/bin/env bash
# Root-only, offline Media Tools directory configuration after first install.
# The Backend Agent must be stopped by the administrator before running this.
set -Eeuo pipefail
stop() { printf 'MEDIA_TOOLS_CONFIG=STOP %s\n' "$*" >&2; exit 1; }

if [ "$#" -eq 1 ] && [ "$1" = --help ]; then
  echo 'Usage: configure_media_tools_paths.sh [SOURCE_ROOT VIDEO_ROOT] [--default-name NAME] [--add-root ROOT NAME]...'
  echo 'The first root is the default; at most 16 additional roots.'
  echo '--check validates the proposed configuration read-only, with no service stop.'
  echo 'Default SOURCE_ROOT=/srv/video/Filme VIDEO_ROOT=/srv/vdr/video.00'
  echo 'Stop the Backend Agent before using; restart it only after reviewing the result.'
  exit 0
fi
[ "$(id -u)" -eq 0 ] || stop 'root required'
for cmd in realpath stat sudo flock mktemp python3 systemctl find grep id getent; do
  command -v "$cmd" >/dev/null || stop "missing tool $cmd"
done
SOURCE=/srv/video/Filme
VIDEO=/srv/vdr/video.00
DEFAULT_NAME=
DEFAULT_NAME_SET=0
CHECK_ONLY=0
EXTRA_ROOTS=()
EXTRA_NAMES=()
# Preserve the original two-positional-argument interface exactly.
if [ "$#" -gt 0 ] && [ "${1:0:1}" != '-' ]; then
  [ "$#" -ge 2 ] && [ "${2:0:1}" != '-' ] ||
    stop 'source and video root must be supplied together'
  SOURCE=$1
  VIDEO=$2
  shift 2
fi
while [ "$#" -gt 0 ]; do
  case "$1" in
    --check)
      [ "$CHECK_ONLY" -eq 0 ] || stop 'duplicate --check'
      CHECK_ONLY=1
      shift ;;
    --default-name)
      [ "$#" -ge 2 ] && [ "$DEFAULT_NAME_SET" -eq 0 ] ||
        stop 'expected one default source name'
      DEFAULT_NAME=$2
      DEFAULT_NAME_SET=1
      shift 2 ;;
    --add-root)
      [ "$#" -ge 3 ] && [ "${#EXTRA_ROOTS[@]}" -lt 16 ] ||
        stop 'at most 16 named extra source roots'
      EXTRA_ROOTS+=("$2")
      EXTRA_NAMES+=("$3")
      shift 3 ;;
    *) stop "unrecognized argument: $1" ;;
  esac
done
# Named roots are descriptive only and must not smuggle TSV syntax or paths.
valid_name() {
  local LC_ALL=C name=$1
  [ -n "$name" ] && [ "${#name}" -le 64 ] &&
    [[ "$name" != *[[:cntrl:]]* &&
       "$name" != *'/'* && "$name" != *'\'* ]] ||
    stop 'invalid source label: max 64 bytes, no controls or separators'
}
if [ "${#EXTRA_ROOTS[@]}" -gt 0 ] && [ -z "$DEFAULT_NAME" ]; then
  DEFAULT_NAME=Filme
fi
if [ "$DEFAULT_NAME_SET" -eq 1 ] || [ -n "$DEFAULT_NAME" ]; then
  valid_name "$DEFAULT_NAME"
fi
for name in "${EXTRA_NAMES[@]}"; do valid_name "$name"; done
BACKEND=$(sed -n 's/^BACKEND_ID=//p' /etc/vdr-suite/backend-agent.conf | head -n 1)
[[ "$BACKEND" =~ ^[A-Za-z0-9_.:-]{1,128}$ ]] &&
  [ "$BACKEND" != . ] && [ "$BACKEND" != .. ] || stop 'backend not configured'

# Do not interrupt an active/authorized Agent job: operator must stop it safely.
if [ "$CHECK_ONLY" -eq 0 ]; then
  systemctl is-active --quiet vdr-suite-backend-agent &&
    stop 'Backend Agent running; stop it safely before reconfiguration'
fi
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

ALL_ROOTS=("$SOURCE" "${EXTRA_ROOTS[@]}")
for dir in "$VIDEO" "${ALL_ROOTS[@]}"; do
  [[ "$dir" = /* && "$dir" != *$'\t'* && "$dir" != *$'\n'* &&
     "$dir" != *$'\r'* ]] ||
    stop 'unsafe media path'
  [ -d "$dir" ] && [ "$(realpath -e "$dir")" = "$dir" ] ||
    stop "directory absent or noncanonical: $dir"
done
for ((i=0; i<${#ALL_ROOTS[@]}; i++)); do
  dir=${ALL_ROOTS[i]}
  [ "$dir" != "$VIDEO" ] || stop 'source equals video root'
  [[ "$dir/" != "$VIDEO/"* && "$VIDEO/" != "$dir/"* ]] ||
    stop 'source and VDR video root must not overlap'
  for ((j=i+1; j<${#ALL_ROOTS[@]}; j++)); do
    other=${ALL_ROOTS[j]}
    [[ "$dir" != "$other" &&
       "$dir/" != "$other/"* && "$other/" != "$dir/"* ]] ||
      stop 'duplicate or nested import roots'
  done
done
[[ "$VIDEO" =~ ^/[a-zA-Z0-9_./-]+$ ]] ||
  stop 'video root incompatible with systemd ReadWritePaths'
for dir in "${ALL_ROOTS[@]}"; do
  case "$dir" in /home/*|/root/*|/run/user/*)
    stop 'source hidden by Backend Agent ProtectHome sandbox' ;;
  esac
  sudo -u "$AGENT_ACCOUNT" test -r "$dir" ||
    stop "Agent cannot read source: $dir"
  sudo -u "$AGENT_ACCOUNT" test -x "$dir" ||
    stop "Agent cannot traverse source: $dir"
done
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
  [ "${#entries[@]}" -ge 1 ] && [ "${#entries[@]}" -le 17 ] &&
    [[ "${entries[0]}" = "$BACKEND"$'\t'local-media-import$'\t'* ]] ||
    stop 'custom source inventory; refuse to overwrite'
  for ((i=1; i<${#entries[@]}; i++)); do
    printf -v expected_id 'local-media-root-%04d' "$i"
    [[ "${entries[i]}" = "$BACKEND"$'\t'"$expected_id"$'\t'* ]] ||
      stop 'custom source inventory; refuse to overwrite'
  done
fi
[ -f "$OVERRIDE" ] && grep -Fxq \
  'ExecStart=/usr/sbin/vdr-suite-backend-agent --config /etc/vdr-suite/backend-agent.conf --enable-media-import --enable-media-shrink --media-shrink-state-dir /var/lib/vdr-suite/backend-agent/media-shrink' \
  "$OVERRIDE" || stop 'unrecognized Backend Agent override'

if [ "$CHECK_ONLY" -eq 1 ]; then
  echo "MEDIA_TOOLS_CONFIG_CHECK=PASS"
  echo "BACKEND=$BACKEND"
  echo "DEFAULT_SOURCE=$SOURCE"
  echo "DEFAULT_NAME=${DEFAULT_NAME:-<legacy>}"
  for ((i=0; i<${#EXTRA_ROOTS[@]}; i++)); do
    printf 'EXTRA_SOURCE_%02d=%s NAME=%s\n' "$((i+1))" \
      "${EXTRA_ROOTS[i]}" "${EXTRA_NAMES[i]}"
  done
  echo "VIDEO_ROOT=$VIDEO"
  echo 'FILES_CHANGED=NO'
  echo 'SERVICES_RESTARTED=NO'
  exit 0
fi

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
if [ -z "$DEFAULT_NAME" ]; then
  printf '%s\tlocal-media-import\t%s\t%s\t*\n' \
    "$BACKEND" "$SOURCE" "$VIDEO" > "$TEMP_LEDGER"
else
  printf '%s\tlocal-media-import\t%s\t%s\t*\t%s\n' \
    "$BACKEND" "$SOURCE" "$VIDEO" "$DEFAULT_NAME" > "$TEMP_LEDGER"
fi
for ((i=0; i<${#EXTRA_ROOTS[@]}; i++)); do
  printf -v source_id 'local-media-root-%04d' "$((i+1))"
  printf '%s\t%s\t%s\t%s\t*\t%s\n' \
    "$BACKEND" "$source_id" "${EXTRA_ROOTS[i]}" "$VIDEO" \
    "${EXTRA_NAMES[i]}" >> "$TEMP_LEDGER"
done
printf '%s\n' \
  '[Service]' \
  'ExecStart=' \
  'ExecStart=/usr/sbin/vdr-suite-backend-agent --config /etc/vdr-suite/backend-agent.conf --enable-media-import --enable-media-shrink --media-shrink-state-dir /var/lib/vdr-suite/backend-agent/media-shrink' \
  "ReadWritePaths=$VIDEO" > "$TEMP_OVERRIDE"
chmod 0644 "$TEMP_LEDGER" "$TEMP_OVERRIDE"
chown root:root "$TEMP_LEDGER" "$TEMP_OVERRIDE"

# Agent stays stopped across both changes; the administrator restarts it.
mv -T -- "$TEMP_LEDGER" "$LEDGER" || stop 'cannot publish source inventory'
mv -T -- "$TEMP_OVERRIDE" "$OVERRIDE" || stop 'cannot publish service override'
systemctl daemon-reload || stop 'systemd reload failed (Agent remains stopped)'

echo "MEDIA_TOOLS_SOURCE=$SOURCE"
echo "MEDIA_TOOLS_DEFAULT_NAME=${DEFAULT_NAME:-<legacy>}"
echo "MEDIA_TOOLS_ADDITIONAL_ROOTS=${#EXTRA_ROOTS[@]}"
echo "MEDIA_TOOLS_VIDEO=$VIDEO"
echo 'MEDIA_TOOLS_CONFIG=PASS'
echo 'BACKEND_AGENT_RESTART=MANUAL_AFTER_REVIEW'
