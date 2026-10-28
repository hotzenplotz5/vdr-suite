#!/usr/bin/env bash
# yaVDR mixed-runtime update. Executes in a child shell, never changes the
# operator's shell, VDR recordings, access grants or existing configuration.
set -Eeuo pipefail
stop() { printf 'MT_INSTALL=STOP %s\n' "$*" >&2; exit 1; }
[ "$#" -eq 1 ] || stop 'full reviewed commit SHA argument required'
SHA=$1
[[ "$SHA" =~ ^[0-9a-f]{40}$ ]] || stop 'commit SHA invalid'
[ "$(id -u)" -eq 0 ] || stop 'existing root shell required'
id yavdr >/dev/null 2>&1 || stop 'yavdr account unavailable'
for cmd in git make sudo sha256sum python3 systemctl; do
  command -v "$cmd" >/dev/null || stop "missing command: $cmd"
done

cd /home/yavdr/vdr-suite || stop 'original checkout missing'
[ "$(sudo -u yavdr -H git rev-parse --show-toplevel 2>/dev/null)" = \
  /home/yavdr/vdr-suite ] || stop 'original checkout not verified'
SHORT=$(printf %s "$SHA" | cut -c 1-12)
WORK="/home/yavdr/vdr-suite-mt-live-$SHORT"
if [ -e "$WORK" ] || [ -L "$WORK" ]; then
  [ -d "$WORK" ] && [ ! -L "$WORK" ] &&
    [ "$(stat -c '%U:%a' "$WORK")" = 'yavdr:700' ] ||
    stop "existing workspace not owned by yavdr: $WORK"
else
  sudo -u yavdr -H mkdir -m 0700 -- "$WORK" ||
    stop 'could not prepare yavdr-owned workspace'
fi
echo "MT_WORKDIR=$WORK"

if [ ! -d "$WORK/repo" ]; then
  sudo -u yavdr -H git clone -q --depth 1 --single-branch \
    --branch fix/media-tools-root-scan-visible-results \
    https://github.com/hotzenplotz5/vdr-suite.git "$WORK/repo" ||
    stop 'public GitHub checkout failed'
fi
SOURCE_SHA=$(sudo -u yavdr -H git -C "$WORK/repo" rev-parse HEAD) ||
  stop 'cannot identify source commit'
[ "$SOURCE_SHA" = "$SHA" ] || stop "source SHA differs: $SOURCE_SHA"
[ -z "$(sudo -u yavdr -H git -C "$WORK/repo" status --porcelain --untracked-files=no)" ] ||
  stop 'source checkout has changes'

TOOL="$WORK/repo/tools/addons/media_tools_observed_live_update.py"
COMPOSE="$WORK/repo/tools/addons/stage_media_tools_runtime.py"
CHECK="$WORK/repo/tools/install_runtime_stage.py"
[ -f "$TOOL" ] && [ -f "$COMPOSE" ] && [ -f "$CHECK" ] ||
  stop 'reviewed installer components missing'

if [ -f "$WORK/candidate.txt" ] &&
   [ -f "$WORK/stage-with-media-tools/.vdr-suite-install-runtime-manifest.json" ]; then
  [ "$(head -n 1 "$WORK/candidate.txt")" = "$SHA" ] ||
    stop 'prepared candidate commit differs'
  HASH=$(sed -n '2p' "$WORK/candidate.txt")
  python3 "$COMPOSE" --verify-stage "$WORK/stage-with-media-tools" \
    --approved-handler-sha256 "$HASH" ||
    stop 'previously prepared candidate is invalid'
  echo 'MT_BUILD=REUSED_SEALED_STAGE'
else
  if [ -e "$WORK/stage" ]; then
    sudo -u yavdr -H python3 "$CHECK" check \
      --stage-root "$WORK/stage" --live-root "$WORK/stage" ||
      stop 'existing staged runtime invalid; not overwriting it'
  else
    echo 'MT_BUILD=START'
    sudo -u yavdr -H make -C "$WORK/repo" -j2 stage-install-runtime \
      BUILD_DIR="$WORK/build" RUNTIME_STAGE="$WORK/stage" ||
      stop 'Suite build failed'
  fi

  if [ ! -x "$WORK/build/addons/vdr-suite-media-import" ] ||
     [ ! -x "$WORK/build/addons/vdr-suite-media-shrink" ] ||
     [ ! -x "$WORK/build/addons/vdr-suite-media-check" ]; then
    sudo -u yavdr -H make -C "$WORK/repo" -j2 \
      addon-media-import addon-media-shrink addon-media-check \
      BUILD_DIR="$WORK/build" ||
      stop 'Media Tools build failed'
  fi

  HASH=$(sha256sum "$WORK/build/addons/vdr-suite-media-import" | cut -d' ' -f1) ||
    stop 'handler digest failed'
  if [ -e "$WORK/candidate.txt" ] || [ -e "$WORK/stage-with-media-tools" ]; then
    stop 'incomplete previous candidate; inspect before replacing'
  fi
  sudo -u yavdr -H sh -c 'printf "%s\n%s\n" "$1" "$2" > "$3"' \
    _ "$SHA" "$HASH" "$WORK/candidate.txt" ||
    stop 'candidate record could not be written'
  sudo -u yavdr -H python3 "$COMPOSE" --prepared-workdir "$WORK" \
    --output-stage "$WORK/stage-with-media-tools" \
    --approved-handler-sha256 "$HASH" ||
    stop 'candidate stage preparation failed'
fi

NEW="$WORK/stage-with-media-tools"
BACKUP="/root/vdr-suite-mt-observed-$SHORT-$(date +%Y%m%d%H%M%S)-$$"
echo "MT_BACKUP=$BACKUP"

# Only this existing stage/actual-live-byte preflight determines deployment
# space; no extra arbitrary multiplication or separate 16 GiB budget.
python3 "$TOOL" preflight --candidate-stage "$NEW" \
  --approved-handler-sha256 "$HASH" --backup-dir "$BACKUP" \
  --confirm LIVE-OBSERVED-UPDATE-REVIEWED ||
  stop "preflight failed; verified prepared candidate kept at $WORK"

if ! systemctl stop vdr-suite-backend-agent vdr-suite-daemon; then
  systemctl start vdr-suite-daemon vdr-suite-backend-agent || true
  stop 'Suite services could not be stopped'
fi

if ! python3 "$TOOL" live-update --candidate-stage "$NEW" \
  --approved-handler-sha256 "$HASH" --backup-dir "$BACKUP" \
  --confirm LIVE-OBSERVED-UPDATE-REVIEWED; then
  if [ ! -f "$BACKUP/review.json" ] ||
     python3 "$CHECK" check --stage-root \
       "$BACKUP/observed-previous-stage" --live-root / >/dev/null 2>&1; then
    systemctl start vdr-suite-daemon vdr-suite-backend-agent ||
      echo 'MT_SERVICES=START_FAILED' >&2
  else
    echo 'MT_SERVICES=STOPPED_ROLLBACK_NOT_VERIFIED' >&2
  fi
  stop 'update failed; check output and preserved candidate'
fi

if ! python3 "$CHECK" check --stage-root "$BACKUP/candidate-stage" --live-root /; then
  echo 'MT_INSTALL=POSTCHECK_FAILED; attempting rollback' >&2
  if python3 "$TOOL" live-rollback --approved-handler-sha256 "$HASH" \
    --backup-dir "$BACKUP" --confirm LIVE-OBSERVED-ROLLBACK-REVIEWED; then
    systemctl start vdr-suite-daemon vdr-suite-backend-agent || true
  fi
  stop 'candidate postcheck unsuccessful'
fi

systemctl start vdr-suite-daemon vdr-suite-backend-agent ||
  stop 'candidate installed but services failed to start'
systemctl is-active --quiet vdr-suite-daemon &&
systemctl is-active --quiet vdr-suite-backend-agent ||
  stop 'candidate installed but services not both active'
echo 'MT_INSTALL=PASS'
echo "MT_INSTALLED_COMMIT=$SHA"
echo "MT_ROLLBACK_BACKUP=$BACKUP"
echo 'MT_SERVICES=ACTIVE'
echo 'MT_CONFIG_AND_GRANTS=UNCHANGED'

if [ -z "$(sudo -u yavdr -H git -C "$WORK/repo" status --porcelain)" ]; then
  sudo -u yavdr -H rm -r -- "$WORK" ||
    echo "MT_WORKDIR_RETAINED=$WORK"
else
  echo "MT_WORKDIR_RETAINED=$WORK"
fi
