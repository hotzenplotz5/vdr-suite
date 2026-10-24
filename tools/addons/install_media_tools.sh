#!/usr/bin/env bash
# Media Tools first-time installation on an already enrolled yaVDR.
set -Eeuo pipefail
stop() { printf 'MEDIA_TOOLS_INSTALL=STOP %s\n' "$*" >&2; exit 1; }
if [ "$#" -eq 1 ] && { [ "$1" = "--help" ] || [ "$1" = "-h" ]; }; then
  echo 'Usage: install_media_tools.sh COMMIT (--prepare|REVIEWED_SHA256) BACKEND [SOURCE_ROOT VIDEO_ROOT] [PREPARED_WORKDIR]'
  echo 'Defaults: SOURCE_ROOT=/srv/video/Filme VIDEO_ROOT=/srv/vdr/video.00'
  echo 'Missing paths are allowed at install time but imports remain unconfigured.'
  exit 0
fi
if [ "$#" -lt 3 ] || [ "$#" -gt 6 ]; then
  echo 'Usage: install_media_tools.sh COMMIT (--prepare|REVIEWED_SHA256) BACKEND [SOURCE_ROOT VIDEO_ROOT] [PREPARED_WORKDIR]' >&2
  exit 2
fi
SHA=$1; APPROVED=$2; BACKEND=$3
# Confirmed Rectools import directory on yaVDR; override both paths if needed.
SOURCE=/srv/video/Filme
VIDEO=/srv/vdr/video.00
PREPARED_WORKDIR=
MODE=install
if [ "$APPROVED" = --prepare ]; then MODE=prepare; fi
case "$#" in
  3) [ "$MODE" = prepare ] || stop "install needs prepared workdir or explicit source/video paths" ;;
  4) [ "$MODE" = install ] || stop "prepare does not accept a workdir"; PREPARED_WORKDIR=$4 ;;
  5) SOURCE=$4; VIDEO=$5 ;;
  6) SOURCE=$4; VIDEO=$5; PREPARED_WORKDIR=$6 ;;
esac
[[ "$SHA" =~ ^[0-9a-f]{40}$ ]] || stop "commit SHA invalid"
if [ "$MODE" = install ]; then
  [[ "$APPROVED" =~ ^[0-9a-f]{64}$ ]] || stop "independent approved handler SHA-256 required"
else
  [ -z "$PREPARED_WORKDIR" ] || stop "prepare cannot reuse an existing build"
fi
[[ "$BACKEND" =~ ^[A-Za-z0-9_.:-]{1,128}$ ]] && [ "$BACKEND" != . ] && [ "$BACKEND" != .. ] || stop "backend invalid"
# Root may only APPLY an existing, separately built and reviewed staging tree.
# Source preparation and all C++ compilation must run as an unprivileged user.
if [ "$(id -u)" -eq 0 ]; then
  [ "$MODE" = install ] && [ -n "$PREPARED_WORKDIR" ] ||
    stop "root may install a prepared build only; never compile as root"
fi
ROOT=$(cd "$(dirname "$0")/../.." && pwd -P) || stop "checkout absent"
cd "$ROOT" || stop "checkout absent"
[ "$(git rev-parse --show-toplevel)" = "$ROOT" ] || stop "wrong checkout"
CHECKOUT_SHA=$(git rev-parse HEAD) || stop "source checkout absent"
[ -z "$(git status --porcelain --untracked-files=no)" ] || stop "dirty checkout"
case "$(git remote get-url origin)" in
  *github.com:hotzenplotz5/vdr-suite.git|*github.com/hotzenplotz5/vdr-suite.git|*github.com/hotzenplotz5/vdr-suite) ;;
  *) stop "unexpected origin" ;;
esac
# A prepared, independently reviewed release is pinned to its build commit.
# A subsequent fast-forward of the official branch must not invalidate that
# immutable release. No new source may be compiled under the old commit pin.
if [ "$CHECKOUT_SHA" != "$SHA" ]; then
  [ "$MODE" = install ] && [ -n "$PREPARED_WORKDIR" ] ||
    stop "source SHA mismatch (prepare requires exact checkout)"
  git cat-file -e "$SHA^{commit}" 2>/dev/null ||
    stop "prepared source commit absent from checkout"
  git merge-base --is-ancestor "$SHA" "$CHECKOUT_SHA" ||
    stop "prepared source not ancestor of installer checkout"
  echo "PREPARED_SOURCE_SHA=$SHA"
  echo "INSTALLER_CHECKOUT_SHA=$CHECKOUT_SHA"
fi
# Fetch the exact official branch ref and require the pinned build source to
# remain part of that branch history. This does not trust an arbitrary local SHA.
if [ "$(id -u)" -eq 0 ]; then
  # Root must not leave root-owned Git control files in an unprivileged build
  # checkout; fetch the provenance ref as the checkout's real owner.
  OWNER=$(stat -c %U "$ROOT") || stop "cannot inspect checkout owner"
  [ -n "$OWNER" ] && [ "$OWNER" != UNKNOWN ] ||
    stop "cannot identify checkout owner"
  sudo -u "$OWNER" -H git -C "$ROOT" fetch --quiet --no-tags origin \
    refs/heads/work/addon-platform-registry-lifecycle ||
    stop "cannot fetch official add-on branch"
else
  git fetch --quiet --no-tags origin refs/heads/work/addon-platform-registry-lifecycle ||
    stop "cannot fetch official add-on branch"
fi
REMOTE=$(git rev-parse FETCH_HEAD) || stop "remote branch ref unavailable"
git merge-base --is-ancestor "$SHA" "$REMOTE" ||
  stop "reviewed commit is no longer in official branch history"
echo "OFFICIAL_BRANCH_HEAD=$REMOTE"
for cmd in sudo make g++ python3 dpkg-query sha256sum pkg-config realpath; do
  command -v "$cmd" >/dev/null || stop "missing dependency $cmd"
done
pkg-config --exists sqlite3 libcurl openssl || stop "development dependencies missing"
for dir in "$SOURCE" "$VIDEO"; do
  [[ "$dir" = /* && "$dir" != *$'\t'* && "$dir" != *$'\n'* ]] ||
    stop "unsafe media path characters or relative path"
  if [ -e "$dir" ] || [ -L "$dir" ]; then
    [ -d "$dir" ] && [ "$(realpath -e "$dir")" = "$dir" ] ||
      stop "unsafe existing media directory"
  fi
done
[ "$SOURCE" != "$VIDEO" ] || stop "same source and destination"
SOURCE_CONFIGURED=0
VIDEO_CONFIGURED=0
[ -d "$SOURCE" ] && SOURCE_CONFIGURED=1
[ -d "$VIDEO" ] && VIDEO_CONFIGURED=1
echo "MEDIA_IMPORT_SOURCE_DEFAULT_OR_OVERRIDE=$SOURCE"
echo "MEDIA_VIDEO_ROOT_DEFAULT_OR_OVERRIDE=$VIDEO"
if [ "$SOURCE_CONFIGURED" -eq 0 ] || [ "$VIDEO_CONFIGURED" -eq 0 ]; then
  echo 'MEDIA_TOOLS_PATHS=PENDING (missing path; import stays unavailable)'
fi
# ProtectSystem=strict needs an explicit bounded VDR video write exception.
[[ "$VIDEO" =~ ^/[a-zA-Z0-9_./-]+$ ]] || stop "video path cannot be safely represented in systemd override"
case "$SOURCE" in /home/*|/root/*|/run/user/*) stop "source hidden by Backend Agent ProtectHome sandbox";; esac
if [ "$MODE" = install ]; then
[ -f /etc/vdr-suite/backend-agent.conf ] || stop "Agent not configured"
grep -Fxq "BACKEND_ID=$BACKEND" /etc/vdr-suite/backend-agent.conf || stop "backend mismatch"
grep -Eq '^CONTROL_PLANE_URL=https://[^[:space:]]+$' /etc/vdr-suite/backend-agent.conf || stop "Agent control plane unavailable"
! grep -q 'example.invalid' /etc/vdr-suite/backend-agent.conf || stop "Agent is still placeholder"
systemctl is-active --quiet vdr-suite-daemon || stop "Suite daemon not active"
systemctl is-active --quiet vdr-suite-backend-agent || stop "Backend Agent not active"
systemctl cat vdr-suite-daemon | grep -Fq 'ExecStart=/usr/sbin/vdr-suite-daemon' || stop "noncanonical Suite service"
systemctl cat vdr-suite-backend-agent | grep -Fq 'ExecStart=/usr/sbin/vdr-suite-backend-agent --config /etc/vdr-suite/backend-agent.conf' || stop "noncanonical Agent service"
for path in /etc/vdr-suite/addon-approvals/rectools.json \
  "/etc/vdr-suite/addon-approvals/rectools.$BACKEND.json" \
  /etc/vdr-suite/addon-approvals/rectools.import-running.lock \
  /etc/vdr-suite/addon-sources/media-import.tsv \
  /etc/systemd/system/vdr-suite-backend-agent.service.d/20-media-tools.conf \
  /var/lib/vdr-suite/backend-agent/media-shrink; do
  [ ! -e "$path" ] && [ ! -L "$path" ] || stop "pre-existing Media Tools state: $path"
done
[ -f /var/lib/vdr-suite/vdr-suite.db ] || stop "unknown active database location"
# Media Tools metadata is read by User=vdr from the fixed public Suite
# inventory. Fail before stopping services or installing files if an
# existing, host-owned ancestor is inaccessible; never chmod it here.
for shared in /usr/share/vdr-suite /usr/share/vdr-suite/addons; do
  if [ -e "$shared" ] || [ -L "$shared" ]; then
    [ -d "$shared" ] && [ ! -L "$shared" ] &&
      sudo -u vdr test -x "$shared" ||
      stop "existing Suite metadata ancestor inaccessible to vdr: $shared (no permissions changed)"
  fi
done
# Never overlay files that are still owned by an older Debian add-on package.
# First-time source installation has a separate, reviewed migration path.
dpkg-query -W -f='${Status}' vdr-suite-addon-media-tools 2>/dev/null |
  grep -Fxq 'install ok installed' &&
  stop "existing Debian-owned Media Tools needs a separate migration, not an overlay"
if [ "$SOURCE_CONFIGURED" -eq 1 ]; then
  sudo -u vdr test -r "$SOURCE" || stop "vdr source access denied"
fi
if [ "$VIDEO_CONFIGURED" -eq 1 ]; then
  sudo -u vdr test -w "$VIDEO" || stop "vdr video access denied"
fi
fi
# The first write is allowed only if every affected FS retains 4 GiB / 15%.
resource_preflight() {
# Reserve for the *actual* database size before backup. After the backup has
# been written, do not count its size twice; preserve the normal base reserve.
python3 - "${PREPARED_WORKDIR:-$HOME}" "$MODE" "${1:-before}" <<'PY' || stop "resource preflight"
import os, shutil, stat, sys
workdir, mode, phase = sys.argv[1:]
gib = 1024**3
work_budget = 8*gib
if mode == 'install' and phase == 'before':
    path = '/var/lib/vdr-suite/vdr-suite.db'
    info = os.lstat(path)
    if not stat.S_ISREG(info.st_mode) or info.st_size == 0:
        raise SystemExit('DATABASE_BACKUP_PREFLIGHT=FAIL (not a regular nonempty database)')
    # SQLite backup can be larger than a sparse source and must have room
    # to complete while retaining the operator's 15% filesystem reserve.
    required = info.st_size + max(gib, (info.st_size+4)//5)
    work_budget = max(work_budget, required)
    print(f'DATABASE_BYTES={info.st_size} BACKUP_BUDGET={work_budget}')
checks = [(workdir, work_budget), ('/usr', gib), ('/var', 2*gib)]
grouped = {}
for path, budget in checks:
    device = os.stat(path).st_dev
    original = grouped.get(device, (path, 0))
    grouped[device] = (original[0], original[1] + budget)
ok = True
for path, budget in grouped.values():
    u = shutil.disk_usage(path)
    reserve = max(4*gib, (u.total*15+99)//100)
    print(f'RESOURCE_FS={path} FREE={u.free} BUDGET={budget} RESERVE={reserve}')
    ok &= u.free >= budget+reserve
print('RESOURCE_PREFLIGHT=' + ('PASS' if ok else 'FAIL'))
raise SystemExit(0 if ok else 1)
PY
}
resource_preflight
if [ "$MODE" = install ]; then sudo -v || stop "sudo authentication failed"; fi
if [ -n "$PREPARED_WORKDIR" ]; then
  WORK=$PREPARED_WORKDIR
  [[ "$WORK" = /* ]] && [ ! -L "$WORK" ] && [ "$(realpath -m "$WORK")" = "$WORK" ] || stop "unsafe prepared workdir"
  [ -f "$WORK/candidate.txt" ] && [ -f "$WORK/stage-with-media-tools/.vdr-suite-install-runtime-manifest.json" ] || stop "prepared sealed Suite + Media Tools runtime missing"
  [ "$(sed -n '1p' "$WORK/candidate.txt")" = "$SHA" ] || stop "prepared build SHA differs"
else
  WORK=$(mktemp -d "$HOME/vdr-suite-media-tools.XXXXXXXX") || stop "unable to create work directory"
  mkdir -m 0700 "$WORK/build" "$WORK/tmp"
  export TMPDIR="$WORK/tmp"
  [ "$(id -u)" -ne 0 ] || stop "refuse root compilation"
  make -j2 stage-install-runtime BUILD_DIR="$WORK/build" RUNTIME_STAGE="$WORK/stage" || stop "Suite build failed"
  make -j2 addon-media-import addon-media-shrink addon-media-check BUILD_DIR="$WORK/build" || stop "Media Tools build failed"
fi
echo "TESTED_SHA=$SHA"
echo "WORKDIR=$WORK"
BINARY="$WORK/build/addons/vdr-suite-media-import"
[ -x "$BINARY" ] || stop "missing handler binary"
DIGEST=$(sha256sum "$BINARY" | cut -d' ' -f1)
if [ "$MODE" = install ]; then
  [ "$DIGEST" = "$APPROVED" ] || stop "handler digest mismatched independent approval"
  if [ -n "$PREPARED_WORKDIR" ]; then
    [ "$(sed -n '2p' "$WORK/candidate.txt")" = "$APPROVED" ] || stop "prepared handler changed"
  fi
fi
# A source install and the optional Media Tools files share one canonical
# sealed install-runtime stage. The Debian package builder is not involved.
STAGE="$WORK/stage-with-media-tools"
if [ "$MODE" = prepare ]; then
  printf '%s\n%s\n' "$SHA" "$DIGEST" > "$WORK/candidate.txt"
  python3 tools/addons/stage_media_tools_runtime.py \
    --prepared-workdir "$WORK" --output-stage "$STAGE" \
    --approved-handler-sha256 "$DIGEST" ||
    stop "package-free sealed Suite + Media Tools stage failed"
  echo "MEDIA_TOOLS_PREPARED=YES"
  echo "WORKDIR=$WORK"
  echo "REVIEW_HANDLER_SHA256=$DIGEST"
  echo 'INSTALL_REQUIRES_INDEPENDENT_APPROVAL=YES'
  exit 0
fi
# Verify sealed overlay and its exact pinned handler bytes BEFORE any live
# service stop; no package manager and no compilation are involved.
python3 tools/install_runtime_stage.py check --stage-root "$STAGE" --live-root / \
  >/dev/null 2>&1 && stop "already deployed; refuse blind reinstall"
[ -f "$STAGE/usr/share/vdr-suite/addons/rectools/addon.json" ] ||
  stop "addon metadata missing from sealed stage"
[ "$(sha256sum "$STAGE/usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import" | cut -d' ' -f1)" = "$APPROVED" ] ||
  stop "sealed stage import handler differs from approved hash"
resource_preflight || stop "insufficient capacity for actual database backup"
# A previous failed attempt may have left an EMPTY root-owned backup folder.
# Reuse only that precise safe state; never overwrite existing backup bytes.
if [ -e "$WORK/backup" ] || [ -L "$WORK/backup" ]; then
  [ -d "$WORK/backup" ] && [ ! -L "$WORK/backup" ] &&
    [ "$(stat -c '%u:%a' "$WORK/backup")" = '0:700' ] &&
    [ -z "$(find "$WORK/backup" -mindepth 1 -maxdepth 1 -print -quit)" ] ||
    stop "existing backup directory is not empty and root-owned 0700"
else
  mkdir -m 0700 "$WORK/backup" || stop "cannot create backup directory"
fi
# A live database backup must happen with the daemon stopped.
RESTART=0
recover() {
  result=$?
  if [ "$RESTART" -eq 1 ]; then
    sudo systemctl daemon-reload || true
    sudo systemctl start vdr-suite-daemon || true
    sudo systemctl start vdr-suite-backend-agent || true
  fi
  exit "$result"
}
trap recover EXIT
RESTART=1
sudo systemctl stop vdr-suite-backend-agent vdr-suite-daemon || stop "could not quiesce Suite"
# Recheck remaining capacity with services quiesced; never skip the backup.
resource_preflight || stop "database backup capacity changed after stop"
sudo python3 - /var/lib/vdr-suite/vdr-suite.db "$WORK/backup/vdr-suite.db" <<'PY'
import os, sqlite3, sys
source, dest = sys.argv[1:]
if os.path.lexists(dest):
    raise SystemExit('refusing to overwrite existing database backup')
print('DATABASE_BACKUP=START', flush=True)
with sqlite3.connect(f'file:{source}?mode=ro', uri=True) as db:
    copy = sqlite3.connect(dest)
    try: db.backup(copy)
    finally: copy.close()
os.chmod(dest, 0o600)
print('DATABASE_BACKUP=PASS bytes=' + str(os.path.getsize(dest)), flush=True)
PY
sudo make deploy-install-runtime RUNTIME_STAGE="$STAGE" LIVE_ROOT=/ ||
  stop "sealed Suite + Media Tools runtime deployment"
sudo make check-install-runtime-deployment RUNTIME_STAGE="$STAGE" LIVE_ROOT=/ ||
  stop "sealed Suite + Media Tools runtime mismatch"
ISSUER="$ROOT/tools/addons/admin_approval_issuer.py"
sudo python3 "$ISSUER" issue --root / --allow-live-root --module rectools --version 0.1.0 --origin manual --approved-sha256 "$APPROVED" || stop "backend approval"
sudo python3 "$ISSUER" issue --root / --allow-live-root --core-expected-only --backend-id "$BACKEND" --module rectools --version 0.1.0 --origin manual --approved-sha256 "$APPROVED" || stop "Core approval"
sudo python3 - "$BACKEND" "$SOURCE" "$VIDEO" "$SOURCE_CONFIGURED" "$VIDEO_CONFIGURED" <<'PY'
import os, pathlib, stat, sys
backend, source, video, source_ready, video_ready = sys.argv[1:]
source_ready = source_ready == '1'
video_ready = video_ready == '1'
for d in ('/etc/vdr-suite/addon-approvals', '/etc/vdr-suite/addon-sources',
          '/etc/systemd/system/vdr-suite-backend-agent.service.d'):
    p = pathlib.Path(d)
    if not p.exists(): p.mkdir(mode=0o755)
    s = p.lstat()
    if not stat.S_ISDIR(s.st_mode) or s.st_uid != 0 or s.st_mode & 0o022:
        raise SystemExit('unsafe root-controlled directory: ' + d)
def create(path, data):
    fd = os.open(path, os.O_CREAT|os.O_EXCL|os.O_WRONLY|os.O_NOFOLLOW, 0o644)
    with os.fdopen(fd, 'w') as out:
        out.write(data)
        out.flush()
        os.fsync(out.fileno())
create('/etc/vdr-suite/addon-approvals/rectools.import-running.lock', '')
# An absent source or video directory means configuration is deferred.
# No entry in the root-owned source ledger -> no discoverable import sources.
if source_ready and video_ready:
    create('/etc/vdr-suite/addon-sources/media-import.tsv',
           f'{backend}\tlocal-media-import\t{source}\t{video}\t*\n')
override = ('[Service]\nExecStart=\n'
            'ExecStart=/usr/sbin/vdr-suite-backend-agent --config '
            '/etc/vdr-suite/backend-agent.conf --enable-media-import '
            '--enable-media-shrink --media-shrink-state-dir '
            '/var/lib/vdr-suite/backend-agent/media-shrink\n')
if video_ready:
    override += 'ReadWritePaths=' + video + '\n'
create('/etc/systemd/system/vdr-suite-backend-agent.service.d/20-media-tools.conf',
       override)
PY
sudo install -d -o vdr -g vdr -m 0700 /var/lib/vdr-suite/backend-agent/media-shrink || stop "private shrink state"
sudo systemctl daemon-reload || stop "systemd reload"
sudo systemctl start vdr-suite-daemon vdr-suite-backend-agent || stop "Suite restart"
systemctl is-active --quiet vdr-suite-daemon && systemctl is-active --quiet vdr-suite-backend-agent || stop "Suite/Agent failed to start"
sudo make check-install-runtime-deployment RUNTIME_STAGE="$STAGE" LIVE_ROOT=/ || stop "installed Suite + Media Tools runtime drift"
RESTART=0
resource_preflight after || stop "post-install reserve insufficient"
echo "MEDIA_TOOLS_INSTALL=PASS"
echo "DATABASE_BACKUP=$WORK/backup/vdr-suite.db"
echo 'IMPORT_AND_SHRINK_AGENT_OPT_IN=YES'
if [ "$SOURCE_CONFIGURED" -eq 1 ] && [ "$VIDEO_CONFIGURED" -eq 1 ]; then
  echo 'MEDIA_TOOLS_PATHS=CONFIGURED'
else
  echo 'MEDIA_TOOLS_PATHS=PENDING'
fi
echo 'CORE_ADMIN_ACTIVATION_AND_USER_GRANTS=PENDING'
