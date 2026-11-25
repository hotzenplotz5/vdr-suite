#!/bin/sh
# MLD 6.6 Media Import temporary systemd opt-in, no package changes.
# Usage: sh mld_media_tools_trial.sh check|prepare|restore
# 'prepare' only stages a /run drop-in; restarting the Agent is a separate
# explicit operator decision. Never installs, approves or starts a worker.
set +e
set +u


unit=vdr-suite-backend-agent.service
binary=/usr/sbin/vdr-suite-backend-agent
config=/etc/vdr-suite/backend-agent.conf
dir=/run/systemd/system/vdr-suite-backend-agent.service.d
dropin="$dir/90-vdr-suite-media-import-trial.conf"

fail() {
  printf 'MLD_MEDIA_IMPORT_TRIAL=STOP %s\n' "$1" >&2
  return 1
}

action=$1
case "$action" in check|prepare|restore) ;; *)
  echo 'Usage: sh mld_media_tools_trial.sh check|prepare|restore'
  exit 2 ;;
esac
if ! command -v systemctl >/dev/null 2>&1; then
  fail 'systemctl unavailable'
  exit 1
fi

if [ "$action" = check ]; then
  state=$(systemctl is-active "$unit" 2>/dev/null)
  [ -n "$state" ] || state=unknown
  printf 'AGENT_SERVICE=%s\n' "$state"
  pid=$(systemctl show "$unit" -p MainPID --value 2>/dev/null)
  case "$pid" in
    ''|0|*[!0-9]*) echo 'RUNNING_OPT_IN=UNKNOWN' ;;
    *)
      if [ -r "/proc/$pid/cmdline" ]; then
        if tr '\000' '\n' < "/proc/$pid/cmdline" |
            grep -Fqx -- '--enable-media-import'; then
          echo 'RUNNING_OPT_IN=YES'
        else
          echo 'RUNNING_OPT_IN=NO'
        fi
      else
        echo 'RUNNING_OPT_IN=UNKNOWN'
      fi ;;
  esac
  if [ -f "$dropin" ]; then
    echo 'RUNTIME_TRIAL_DROPIN=PRESENT'
  else
    echo 'RUNTIME_TRIAL_DROPIN=ABSENT'
  fi
  exit 0
fi

if [ "$(id -u)" != 0 ]; then
  fail 'root required'
  exit 1
fi

expected=$(printf '%s\n' \
  '[Service]' \
  'ExecStart=' \
  'ExecStart=/usr/sbin/vdr-suite-backend-agent --config /etc/vdr-suite/backend-agent.conf --enable-media-import')
if [ "$action" = restore ]; then
  if [ ! -e "$dropin" ] && [ ! -L "$dropin" ]; then
    echo 'MLD_MEDIA_IMPORT_TRIAL=ALREADY_ABSENT'
    exit 0
  fi
  if [ ! -f "$dropin" ] || [ -L "$dropin" ] ||
      [ "$(cat "$dropin" 2>/dev/null)" != "$expected" ]; then
    fail 'trial drop-in has different content; not removing'
    exit 1
  fi
  rm -f -- "$dropin" || { fail 'cannot remove trial drop-in'; exit 1; }
  systemctl daemon-reload || { fail 'daemon-reload after restore failed'; exit 1; }
  echo 'MLD_MEDIA_IMPORT_TRIAL=RESTORED_UNIT_ONLY'
  echo 'AGENT_RESTART=MANUAL_AFTER_REVIEW'
  exit 0
fi

# Only an exact known service layout can be overridden without silently
# losing distribution-specific arguments. For a different layout the owner
# must review systemctl cat and append the flag preserving all options.
if [ ! -x "$binary" ] || [ ! -r "$config" ]; then
  fail 'Agent binary or config absent'
  exit 1
fi
current=$(systemctl show "$unit" -p ExecStart --value 2>/dev/null)
case "$current" in
  *"argv[]=/usr/sbin/vdr-suite-backend-agent --config /etc/vdr-suite/backend-agent.conf ;"*) ;;
  *)
    fail 'nonstandard ExecStart; inspect systemctl cat and preserve existing arguments'
    exit 1 ;;
esac
if [ -e "$dropin" ] || [ -L "$dropin" ]; then
  fail 'trial drop-in already exists; no overwrite'
  exit 1
fi
if [ -e /etc/systemd/system/vdr-suite-backend-agent.service.d/90-vdr-suite-media-import-trial.conf ]; then
  fail 'persistent trial filename already exists'
  exit 1
fi
umask 022
mkdir -p -- "$dir" || { fail 'cannot create runtime drop-in directory'; exit 1; }
printf '%s\n' "$expected" > "$dropin" ||
  { fail 'cannot write trial drop-in'; exit 1; }
if ! systemctl daemon-reload; then
  rm -f -- "$dropin"
  fail 'daemon-reload failed; trial drop-in removed'
  exit 1
fi
echo 'MLD_MEDIA_IMPORT_TRIAL=PREPARED_RUNTIME_ONLY'
echo 'AGENT_RESTART=MANUAL_AFTER_REVIEW'
echo 'APPROVAL_FILES=NOT_MODIFIED'
echo 'PACKAGE_FILES=NOT_MODIFIED'
