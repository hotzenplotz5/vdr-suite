#!/usr/bin/env bash
# Resume the already-built c26429741f9a yaVDR Media Tools candidate.
# Keep the operator's existing root shell and original checkout unchanged.
set -Eeuo pipefail
stop() { printf 'MT_RESUME=STOP %s\n' "$*" >&2; exit 1; }
trap 'rc=$?; printf "MT_RESUME_EXIT=%s\n" "$rc"' EXIT

OLD_SOURCE=c26429741f9a868bcf129cd69f956c2c5a86ba00
FIX_SOURCE=20a0ebc626fb9c4ac1b7c8e9c39d037a7d6e7a87
FIX_BLOB=de5947e977de24fc1391830cc9cde7f4df5872ed
WORK=/home/yavdr/vdr-suite-mt-live-c26429741f9a
ROOT=/home/yavdr/vdr-suite
[ "$(id -u)" -eq 0 ] || stop 'existing root shell required'
cd "$ROOT" || stop 'original checkout unavailable'
[ -d "$WORK/repo" ] || stop "prepared checkout missing: $WORK/repo"
[ "$(sudo -u yavdr -H git -C "$WORK/repo" rev-parse HEAD)" = "$OLD_SOURCE" ] ||
  stop 'prepared source SHA differs'
[ "$(sudo -u yavdr -H git -C "$WORK/repo" rev-parse --show-toplevel)" = "$WORK/repo" ] ||
  stop 'prepared checkout root differs'
[ -z "$(sudo -u yavdr -H git -C "$WORK/repo" status --porcelain --untracked-files=no)" ] ||
  stop 'prepared tracked checkout modified'

CANDIDATE="$WORK/stage-with-media-tools"
RECORD="$WORK/candidate.txt"
[ -f "$RECORD" ] && [ -f "$CANDIDATE/.vdr-suite-install-runtime-manifest.json" ] ||
  stop 'existing candidate stage/record missing (will NOT rebuild)'
[ "$(head -n 1 "$RECORD")" = "$OLD_SOURCE" ] ||
  stop 'prepared source record mismatch'
HASH=$(sed -n '2p' "$RECORD")
[[ "$HASH" =~ ^[0-9a-f]{64}$ ]] || stop 'prepared handler SHA256 invalid'
[ "$(sha256sum "$WORK/build/addons/vdr-suite-media-import" | cut -d' ' -f1)" = "$HASH" ] ||
  stop 'compiled handler does not match sealed candidate'
python3 "$WORK/repo/tools/addons/stage_media_tools_runtime.py" \
  --verify-stage "$CANDIDATE" --approved-handler-sha256 "$HASH" ||
  stop 'existing candidate seal mismatch'

# Fetch ONLY the corrected installation helper, as a new untracked filename.
# Existing source, compiled binaries, candidate.txt and stage stay unchanged.
FIX="$WORK/repo/tools/addons/media_tools_observed_live_update_resume.py"
if [ -e "$FIX" ]; then
  [ -f "$FIX" ] && [ ! -L "$FIX" ] ||
    stop 'existing corrected helper is not a regular file'
else
  TMP="$FIX.download"
  [ ! -e "$TMP" ] || stop 'unexpected incomplete helper download'
  sudo -u yavdr -H curl --fail --location --silent --show-error \
    "https://raw.githubusercontent.com/hotzenplotz5/vdr-suite/$FIX_SOURCE/tools/addons/media_tools_observed_live_update.py" \
    --output "$TMP" ||
    stop 'corrected helper download failed'
  sudo -u yavdr -H mv -- "$TMP" "$FIX" ||
    stop 'corrected helper move failed'
fi
[ "$(sudo -u yavdr -H git -C "$WORK/repo" hash-object "$FIX")" = "$FIX_BLOB" ] ||
  stop 'corrected helper differs from reviewed blob'
python3 -B - "$FIX" <<'PY' || stop 'corrected helper syntax invalid'
import ast, pathlib, sys
ast.parse(pathlib.Path(sys.argv[1]).read_text(encoding='utf-8'))
PY

BACKUP="/root/vdr-suite-mt-observed-c26429741f9a-$(date +%Y%m%d%H%M%S)-$$"
echo "MT_RESUME_PREPARED=$OLD_SOURCE"
echo "MT_BUILD=SKIPPED_REUSED_SEALED_STAGE"
echo "MT_BACKUP=$BACKUP"

# Candidate verification and actual measured backup requirement, no second
# arbitrary outer quota; services remain active until this passes.
python3 "$FIX" preflight \
  --candidate-stage "$CANDIDATE" \
  --approved-handler-sha256 "$HASH" \
  --backup-dir "$BACKUP" \
  --confirm LIVE-OBSERVED-UPDATE-REVIEWED ||
  stop 'read-only deployment preflight failed; services unchanged'

if ! systemctl stop vdr-suite-backend-agent vdr-suite-daemon; then
  systemctl start vdr-suite-daemon vdr-suite-backend-agent || true
  stop 'Suite services could not be cleanly stopped'
fi

if ! python3 "$FIX" live-update \
  --candidate-stage "$CANDIDATE" \
  --approved-handler-sha256 "$HASH" \
  --backup-dir "$BACKUP" \
  --confirm LIVE-OBSERVED-UPDATE-REVIEWED; then
  if [ ! -f "$BACKUP/review.json" ] ||
     python3 "$WORK/repo/tools/install_runtime_stage.py" check \
       --stage-root "$BACKUP/observed-previous-stage" --live-root / >/dev/null 2>&1; then
    systemctl start vdr-suite-daemon vdr-suite-backend-agent ||
      echo 'MT_SERVICES=START_FAILED' >&2
  else
    echo 'MT_SERVICES=STOPPED_ROLLBACK_UNVERIFIED' >&2
  fi
  stop 'update failed (candidate retained)'
fi

if ! python3 "$WORK/repo/tools/install_runtime_stage.py" check \
  --stage-root "$BACKUP/candidate-stage" --live-root /; then
  if python3 "$FIX" live-rollback \
    --approved-handler-sha256 "$HASH" \
    --backup-dir "$BACKUP" --confirm LIVE-OBSERVED-ROLLBACK-REVIEWED; then
    systemctl start vdr-suite-daemon vdr-suite-backend-agent || true
  fi
  stop 'installed stage mismatch; see backup/rollback status'
fi

systemctl start vdr-suite-daemon vdr-suite-backend-agent ||
  stop 'runtime installed, Suite services failed to start'
systemctl is-active --quiet vdr-suite-daemon &&
systemctl is-active --quiet vdr-suite-backend-agent ||
  stop 'runtime installed but services not active'
echo 'MT_INSTALL=PASS'
echo "MT_INSTALLED_SOURCE_SHA=$OLD_SOURCE"
echo "MT_BACKUP=$BACKUP"
echo 'MT_SERVICES=ACTIVE'
echo 'MT_RECORDINGS_AND_APPROVALS=UNCHANGED'
