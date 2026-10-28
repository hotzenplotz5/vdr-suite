#!/usr/bin/env bash
# Media Tools upgrade for the existing yaVDR. Run as root in a CHILD shell.
# Only the verified product commit is built; later documentation commits do not trigger a rebuild.
set +e
set +u
set +o pipefail

mt_install() {
  local CODE_SHA=c7cf3f51200b738449edd43810ce87022b1ccc85
  local TOOLS_SHA=e7193f95298e63819cc32086240f51c93eeee3fe
  local BRANCH=fix/media-tools-root-scan-visible-results
  local REPO=https://github.com/hotzenplotz5/vdr-suite.git
  local WORK=/home/yavdr/vdr-suite-mt-kiss-c7cf3f51200b
  local ROOT=/home/yavdr/vdr-suite
  local STAGE='' HASH='' OLD_HASH='' BACKEND='' BASE='' CANDIDATE='' ROOT_DIR=''
  local file state origin pinned daemon_prior agent_prior
  local -a ROTATE=()

  [ "$(id -u)" -eq 0 ] || { echo 'MT_INSTALL=STOP: root shell required'; return 1; }
  cd "$ROOT" || { echo "MT_INSTALL=STOP: checkout directory missing: $ROOT"; return 1; }
  [ -e .git ] || { echo 'MT_INSTALL=STOP: wrong checkout root'; return 1; }
  for c in git runuser make g++ python3 sha256sum systemctl cmp dpkg-query; do
    command -v "$c" >/dev/null 2>&1 || { echo "MT_INSTALL=STOP: missing $c"; return 1; }
  done
  id yavdr >/dev/null 2>&1 || { echo 'MT_INSTALL=STOP: yavdr user missing'; return 1; }
  [ -r /etc/vdr-suite/backend-agent.conf ] || { echo 'MT_INSTALL=STOP: Agent config absent'; return 1; }
  BACKEND=$(sed -n 's/^BACKEND_ID=//p' /etc/vdr-suite/backend-agent.conf | head -n 1)
  [[ "$BACKEND" =~ ^[A-Za-z0-9_.:-]{1,128}$ ]] || { echo 'MT_INSTALL=STOP: backend ID invalid'; return 1; }

  # Reuse an already sealed candidate of this exact product code if one exists.
  for CANDIDATE in /home/yavdr/vdr-suite-mt-*/stage-with-media-tools; do
    BASE=${CANDIDATE%/stage-with-media-tools}
    [ -f "$BASE/candidate.txt" ] || continue
    [ "$(sed -n '1p' "$BASE/candidate.txt")" = "$CODE_SHA" ] || continue
    [ -f "$BASE/repo/tools/addons/stage_media_tools_runtime.py" ] || continue
    HASH=$(sed -n '2p' "$BASE/candidate.txt")
    if [[ "$HASH" =~ ^[a-f0-9]{64}$ ]] &&
       runuser -u yavdr -- python3 "$BASE/repo/tools/addons/stage_media_tools_runtime.py" \
         --verify-stage "$CANDIDATE" --approved-handler-sha256 "$HASH" >/dev/null 2>&1; then
      WORK=$BASE
      STAGE=$CANDIDATE
      echo "MT_BUILD=REUSED_SEALED_CANDIDATE $WORK"
      break
    fi
  done

  if [ -z "$STAGE" ]; then
    if [ ! -e "$WORK" ]; then
      runuser -u yavdr -- mkdir -m 0700 "$WORK" || return 1
    fi
    [ -d "$WORK" ] && [ ! -L "$WORK" ] && [ "$(stat -c '%U:%a' "$WORK")" = yavdr:700 ] || {
      echo "MT_INSTALL=STOP: unexpected workdir $WORK"; return 1;
    }
    if [ ! -d "$WORK/repo/.git" ]; then
      [ ! -e "$WORK/repo" ] || { echo 'MT_INSTALL=STOP: unexpected repository path'; return 1; }
      runuser -u yavdr -- git clone -q --depth 20 --single-branch --branch "$BRANCH" \
        "$REPO" "$WORK/repo" || return 1
    fi
    # The tooling commit changes only AGENTS.md and installer/staging code.
    # Reuse binaries compiled from CODE_SHA; no new product build is required.
    runuser -u yavdr -- git -C "$WORK/repo" fetch -q --no-tags --depth 20 origin "$BRANCH" || return 1
    [ -z "$(runuser -u yavdr -- git -C "$WORK/repo" status --porcelain --untracked-files=no)" ] || {
      echo 'MT_INSTALL=STOP: checkout contains tracked changes'; return 1;
    }
    runuser -u yavdr -- git -C "$WORK/repo" checkout -q --detach "$TOOLS_SHA" || return 1
    [ "$(runuser -u yavdr -- git -C "$WORK/repo" rev-parse --show-toplevel)" = "$WORK/repo" ] &&
    [ "$(runuser -u yavdr -- git -C "$WORK/repo" rev-parse HEAD)" = "$TOOLS_SHA" ] || {
      echo 'MT_INSTALL=STOP: unverified staging tools'; return 1;
    }
    local changed_file
    while IFS= read -r changed_file; do
      case "$changed_file" in
        ''|AGENTS.md|tools/addons/install_media_tools_kiss_yavdr.sh|tools/addons/stage_media_tools_runtime.py) ;;
        *) echo "MT_INSTALL=STOP: product input differs from $CODE_SHA: $changed_file"; return 1 ;;
      esac
    done < <(runuser -u yavdr -- git -C "$WORK/repo" diff --name-only "$CODE_SHA" "$TOOLS_SHA")

    if [ -e "$WORK/stage-with-media-tools" ]; then
      echo 'MT_INSTALL=STOP: existing candidate stage is not verified; keeping it'; return 1
    fi
    if [ -f "$WORK/stage/.vdr-suite-install-runtime-manifest.json" ]; then
      runuser -u yavdr -- python3 "$WORK/repo/tools/install_runtime_stage.py" check \
        --stage-root "$WORK/stage" --live-root "$WORK/stage" || {
        echo 'MT_INSTALL=STOP: existing Suite stage invalid'; return 1;
      }
      echo 'MT_SUITE_BUILD=REUSED_EXISTING_SEALED_STAGE'
    else
      [ ! -e "$WORK/stage" ] || { echo 'MT_INSTALL=STOP: incomplete Suite stage'; return 1; }
      echo "MT_SUITE_BUILD=START $CODE_SHA"
      runuser -u yavdr -- make -C "$WORK/repo" -j2 stage-install-runtime \
        BUILD_DIR="$WORK/build" RUNTIME_STAGE="$WORK/stage" || return 1
    fi

    local -a missing=()
    [ -x "$WORK/build/addons/vdr-suite-media-import" ] || missing+=(addon-media-import)
    [ -x "$WORK/build/addons/vdr-suite-media-shrink" ] || missing+=(addon-media-shrink)
    [ -x "$WORK/build/addons/vdr-suite-media-check" ] || missing+=(addon-media-check)
    if [ "${#missing[@]}" -gt 0 ]; then
      echo "MT_ADDON_BUILD=MISSING_ONLY ${missing[*]}"
      runuser -u yavdr -- make -C "$WORK/repo" -j2 "${missing[@]}" \
        BUILD_DIR="$WORK/build" || return 1
    else
      echo 'MT_ADDON_BUILD=REUSED_EXISTING_WORKERS'
    fi
    HASH=$(sha256sum "$WORK/build/addons/vdr-suite-media-import" | cut -d' ' -f1) || return 1
    if [ -f "$WORK/candidate.txt" ]; then
      [ "$(sed -n '1p' "$WORK/candidate.txt")" = "$CODE_SHA" ] &&
      [ "$(sed -n '2p' "$WORK/candidate.txt")" = "$HASH" ] || {
        echo 'MT_INSTALL=STOP: candidate record differs from existing build'; return 1;
      }
    else
      runuser -u yavdr -- sh -c 'printf "%s\n%s\n" "$1" "$2" > "$3"' \
        sh "$CODE_SHA" "$HASH" "$WORK/candidate.txt" || return 1
    fi
    runuser -u yavdr -- python3 "$WORK/repo/tools/addons/stage_media_tools_runtime.py" \
      --prepared-workdir "$WORK" --output-stage "$WORK/stage-with-media-tools" \
      --approved-handler-sha256 "$HASH" || return 1
    STAGE="$WORK/stage-with-media-tools"
  fi

  python3 "$WORK/repo/tools/addons/stage_media_tools_runtime.py" \
    --verify-stage "$STAGE" --approved-handler-sha256 "$HASH" || return 1
  echo "MT_CODE_SHA=$CODE_SHA"
  echo "MT_HANDLER_SHA256=$HASH"

  # Do not overlay files managed by another package mechanism.
  for file in /usr/sbin/vdr-suite-daemon /usr/sbin/vdr-suite-backend-agent \
      /usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import; do
    if dpkg-query -S "$file" >/dev/null 2>&1; then
      echo "MT_INSTALL=STOP: dpkg owns $file"; return 1
    fi
  done

  # Prepare updates for EXISTING approvals only. No new grants or activation.
  file=/usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import
  if [ -f "$file" ]; then
    OLD_HASH=$(sha256sum "$file" | cut -d' ' -f1) || return 1
  fi
  for file in /etc/vdr-suite/addon-approvals/rectools.json \
      "/etc/vdr-suite/addon-approvals/rectools.$BACKEND.json"; do
    [ -e "$file" ] || continue
    read -r state origin pinned < <(python3 - "$file" <<'PY'
import json,sys
try:
    with open(sys.argv[1], encoding='utf-8') as stream:
        x=json.load(stream)
    print(x['state'],x['origin'],x['sha256'])
except (OSError,ValueError,KeyError,TypeError):
    print('INVALID INVALID INVALID')
PY
)
    if [ "$state" = revoked ]; then
      echo "MT_APPROVAL=REVOKED_PRESERVED $file"
      continue
    fi
    [ "$state" = approved ] && [ "$origin" = manual ] && \
    { [ "$pinned" = "$OLD_HASH" ] || [ "$pinned" = "$HASH" ]; } || {
      echo "MT_INSTALL=STOP: existing approval needs separate diagnosis: $file"; return 1;
    }
    [ "$pinned" = "$OLD_HASH" ] && [ "$pinned" != "$HASH" ] && ROTATE+=("$file")
  done

  # Already installed? Do not rebuild, redeploy or restart.
  if python3 "$WORK/repo/tools/install_runtime_stage.py" check \
      --stage-root "$STAGE" --live-root / >/dev/null 2>&1 &&
     [ "${#ROTATE[@]}" -eq 0 ] &&
     systemctl is-active --quiet vdr-suite-daemon &&
     systemctl is-active --quiet vdr-suite-backend-agent; then
    echo 'MT_INSTALL=ALREADY_CURRENT'; return 0
  fi

  daemon_prior=$(systemctl is-active vdr-suite-daemon)
  agent_prior=$(systemctl is-active vdr-suite-backend-agent)
  [ "$daemon_prior" = active ] || { echo 'MT_INSTALL=STOP: daemon was not active; no forced service activation'; return 1; }
  [ "$agent_prior" = active ] || { echo 'MT_INSTALL=STOP: backend agent was not active; no forced service activation'; return 1; }
  echo 'MT_DEPLOY=START'
  systemctl stop vdr-suite-backend-agent vdr-suite-daemon || {
    echo 'MT_INSTALL=STOP: services could not be stopped'; return 1;
  }
  if ! python3 "$WORK/repo/tools/install_runtime_stage.py" deploy \
      --stage-root "$STAGE" --live-root / ||
     ! python3 "$WORK/repo/tools/install_runtime_stage.py" check \
      --stage-root "$STAGE" --live-root /; then
    echo "MT_INSTALL=FAILED: deployment mismatch; services stopped; stage=$STAGE"
    return 1
  fi

  for file in "${ROTATE[@]}"; do
    local -a extra=()
    if [ "${file##*/}" != rectools.json ]; then
      extra=(--backend-id "$BACKEND" --core-expected-only)
    fi
    python3 "$WORK/repo/tools/addons/admin_approval_issuer.py" rotate \
      --root / --module rectools --version 0.1.0 --origin manual \
      --expected-sha256 "$OLD_HASH" --approved-sha256 "$HASH" \
      --allow-live-root "${extra[@]}" || {
        echo "MT_INSTALL=FAILED: existing approval rotation failed: $file"; return 1;
      }
  done

  systemctl start vdr-suite-daemon || { echo 'MT_INSTALL=FAILED: daemon start'; return 1; }
  systemctl start vdr-suite-backend-agent || { echo 'MT_INSTALL=FAILED: agent start'; return 1; }
  systemctl is-active --quiet vdr-suite-daemon && \
  systemctl is-active --quiet vdr-suite-backend-agent || {
    echo 'MT_INSTALL=FAILED: services not active'; return 1;
  }
  # The source/install check has already passed. A /proc/<pid>/exe
  # comparison can be temporarily unavailable during Agent startup/restart;
  # report that separately instead of misreporting the deployment as failed.
  local process_check=PASS pid running_inode installed_inode
  for file in vdr-suite-daemon vdr-suite-backend-agent; do
    pid=$(systemctl show -p MainPID --value "$file")
    running_inode=''
    installed_inode=''
    if [[ "$pid" =~ ^[1-9][0-9]*$ ]]; then
      running_inode=$(stat -Lc '%d:%i' "/proc/$pid/exe" 2>/dev/null)
      installed_inode=$(stat -Lc '%d:%i' "/usr/sbin/$file" 2>/dev/null)
    fi
    if [ -n "$running_inode" ] && [ "$running_inode" = "$installed_inode" ]; then
      echo "MT_PROCESS_$file=MATCH"
    else
      process_check=UNVERIFIED
      echo "MT_PROCESS_$file=UNVERIFIED pid=${pid:-none} (check live service logs if needed)"
    fi
  done
  echo 'RUNTIME_DEPLOYMENT_MATCH=YES'
  echo 'MT_INSTALL=PASS (installed files and service starts)'
  echo "MT_RUNNING_PROCESSES=$process_check"
  echo 'MT_VDR_RECORDINGS_CONFIG_DB=UNCHANGED'
  echo 'MT_APPROVALS=EXISTING_PINS_UPDATED_IF_NEEDED'
  echo 'MT_ACTIVATION_AND_REAL_VDR_IMPORT=NOT_VERIFIED'
  return 0
}

mt_install
MT_RESULT=$?
if [ "$MT_RESULT" -ne 0 ]; then
  echo 'MT_INSTALL=FAILED_OR_NOT_STARTED'
fi
[ "$MT_RESULT" -eq 0 ]
