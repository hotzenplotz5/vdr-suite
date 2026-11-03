#!/usr/bin/env bash
# Update the existing yaVDR Media Tools mixed runtime to the tested candidate.
# Reuse the existing addon worker bytes, build changed Suite code as yavdr,
# and deploy through the established observed-live backup/rollback helper.
set +e
set +u
set +o pipefail

mt_install() {
  local SHA=5057533b5e2862b50f886a70a41905b4aa77457a
  local ROOT=/home/yavdr/vdr-suite
  local OLD=/home/yavdr/vdr-suite-mt-live-c26429741f9a
  local WORK STAGE HASH BACKUP TOOL
  cd "$ROOT" || return 1
  [ "$(id -u)" -eq 0 ] || { echo 'INSTALL=STOP: Existing root shell required'; return 1; }
  [ -f "$OLD/stage-with-media-tools/.vdr-suite-install-runtime-manifest.json" ] || {
    echo 'INSTALL=STOP: Previous Media Tools stage missing'; return 1;
  }
  echo '=== BUILD NEW CANDIDATE (LIVE SERVICES UNCHANGED) ==='
  # An independent shallow clone avoids the existing checkout's object
  # database. In particular, a failed fetch with an unresolved delta must
  # not corrupt or hold up the productive installation.
  WORK=$(runuser -u yavdr -- mktemp -d /home/yavdr/vdr-suite-mt-update-XXXXXXXX) || return 1
  echo "MT_UPDATE_WORK=$WORK"
  runuser -u yavdr -- git clone --quiet --no-tags --depth 50 --single-branch \
    --branch fix/media-tools-root-scan-visible-results \
    https://github.com/hotzenplotz5/vdr-suite.git "$WORK/repo" || return 1
  runuser -u yavdr -- git -C "$WORK/repo" checkout --quiet --detach "$SHA" || return 1
  [ "$(runuser -u yavdr -- git -C "$WORK/repo" rev-parse HEAD)" = "$SHA" ] || return 1
  runuser -u yavdr -- cp -a "$OLD/build" "$WORK/build" || return 1
  runuser -u yavdr -- make -C "$WORK/repo" -j2 stage-install-runtime \
    BUILD_DIR="$WORK/build" RUNTIME_STAGE="$WORK/stage" || return 1
  STAGE="$WORK/stage-with-media-tools"
  runuser -u yavdr -- python3 - "$WORK/repo" "$WORK/stage" \
    "$OLD/stage-with-media-tools" "$STAGE" <<'PY' || return 1
import json, os, shutil, stat, sys
from pathlib import Path
repo, fresh, old, result = map(Path, sys.argv[1:])
sys.path.insert(0, str(repo / 'tools'))
sys.path.insert(0, str(repo / 'tools/addons'))
from stage_media_tools_runtime import payload_roles, INVENTORY_REL, verify_media_tools_stage
from install_runtime_stage import load_manifest, verify_sealed_stage, seal
verify_sealed_stage(fresh, load_manifest(fresh))
record = json.loads((old / INVENTORY_REL).read_text())
digest = record['approved_handler_sha256']
verify_media_tools_stage(old, digest)
shutil.copytree(fresh, result)
for name in list(payload_roles()) + [INVENTORY_REL.as_posix()]:
    origin = old / name
    dest = result / name
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(origin, dest)
    parent = dest.parent
    while parent != result and parent.is_relative_to(result):
        original_parent = old / parent.relative_to(result)
        if original_parent.is_dir():
            os.chmod(parent, stat.S_IMODE(original_parent.stat().st_mode))
        parent = parent.parent
seal(result)
verify_media_tools_stage(result, digest)
print('MT_NEW_STAGE=PASS')
print('MT_EXISTING_HANDLER_REUSED=' + digest)
PY
  HASH=$(python3 - "$STAGE/usr/share/vdr-suite/addons/rectools/media-tools-install-inventory.json" <<'PY'
import json,sys
print(json.load(open(sys.argv[1]))['approved_handler_sha256'])
PY
  ) || return 1
  [ "$(sha256sum /usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import | awk '{print $1}')" = "$HASH" ] || {
    echo 'INSTALL=STOP: Installed worker differs from existing approved stage'; return 1;
  }
  BACKUP="/root/vdr-suite-mt-upgrade-$(date +%Y%m%d%H%M%S)-$$"
  TOOL="$WORK/repo/tools/addons/media_tools_observed_live_update.py"
  echo "MT_NEW_BACKUP=$BACKUP"
  python3 "$TOOL" preflight --candidate-stage "$STAGE" \
    --approved-handler-sha256 "$HASH" --backup-dir "$BACKUP" \
    --confirm LIVE-OBSERVED-UPDATE-REVIEWED || return 1
  echo '=== BACKUP AND INSTALL EXISTING RUNTIME ==='
  if ! systemctl stop vdr-suite-backend-agent vdr-suite-daemon; then
    systemctl start vdr-suite-daemon vdr-suite-backend-agent
    return 1
  fi
  if ! python3 "$TOOL" live-update --candidate-stage "$STAGE" \
    --approved-handler-sha256 "$HASH" --backup-dir "$BACKUP" \
    --confirm LIVE-OBSERVED-UPDATE-REVIEWED; then
    if [ ! -f "$BACKUP/review.json" ] || \
       python3 "$WORK/repo/tools/install_runtime_stage.py" check \
         --stage-root "$BACKUP/observed-previous-stage" --live-root /; then
      systemctl start vdr-suite-daemon vdr-suite-backend-agent
    else
      echo 'MT_SERVICES=STOPPED_ROLLBACK_UNVERIFIED'
    fi
    return 1
  fi
  if ! python3 "$WORK/repo/tools/install_runtime_stage.py" check \
      --stage-root "$BACKUP/candidate-stage" --live-root / || \
     ! systemctl start vdr-suite-daemon vdr-suite-backend-agent || \
     ! systemctl is-active --quiet vdr-suite-daemon || \
     ! systemctl is-active --quiet vdr-suite-backend-agent || \
     ! cmp -s "/proc/$(systemctl show -p MainPID --value vdr-suite-daemon)/exe" /usr/sbin/vdr-suite-daemon || \
     ! cmp -s "/proc/$(systemctl show -p MainPID --value vdr-suite-backend-agent)/exe" /usr/sbin/vdr-suite-backend-agent; then
    echo 'MT_VERIFY=FAIL; ROLLING_BACK'
    systemctl stop vdr-suite-backend-agent vdr-suite-daemon
    if python3 "$TOOL" live-rollback --approved-handler-sha256 "$HASH" \
        --backup-dir "$BACKUP" --confirm LIVE-OBSERVED-ROLLBACK-REVIEWED; then
      systemctl start vdr-suite-daemon vdr-suite-backend-agent
      echo 'MT_ROLLBACK=PASS'
    else
      echo 'MT_ROLLBACK=FAIL; SERVICES_STOPPED_BACKUP_RETAINED'
    fi
    return 1
  fi
  echo 'MT_INSTALL=PASS'
  echo "MT_INSTALLED_SOURCE_SHA=$SHA"
  echo "MT_BACKUP=$BACKUP"
  echo 'RUNTIME_DEPLOYMENT_MATCH=YES'
  echo 'MT_SERVICES=ACTIVE'
  echo 'MT_RECORDINGS_AND_APPROVALS=UNCHANGED'
  df -hP /home/yavdr
}

if ! mt_install; then
  echo 'MT_INSTALL=FAILED_OR_NOT_STARTED'
fi
