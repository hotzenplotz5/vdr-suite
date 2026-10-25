#!/usr/bin/env bash
# yaVDR/systemd first-install adapter for optional Media Tools.
# Portable staged artifacts are separate; do not treat this host-specific
# service/config/backup choreography as the installer for MLD or other init systems.
set -Eeuo pipefail
stop() { printf 'MEDIA_TOOLS_INSTALL=STOP %s\n' "$*" >&2; exit 1; }
# Explicit yaVDR file update: old+new complete Suite stages are mandatory;
# services must already be stopped. No service control or grant issuance.
if [ "${1:-}" = live-update ]; then
  [ "$#" -eq 7 ] ||
    stop 'Usage: install_media_tools.sh live-update OLD_STAGE NEW_STAGE OLD_SHA256 NEW_SHA256 NEW_ROOT_BACKUP_DIR LIVE-UPDATE-REVIEWED'
  SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P) || stop 'script path unavailable'
  exec python3 "$SCRIPT_DIR/media_tools_runtime_transaction.py" live-update \
    --previous-stage "$2" --new-stage "$3" --live-root / \
    --previous-approved-sha256 "$4" --new-approved-sha256 "$5" \
    --backup-dir "$6" --confirm "$7"
fi
# MT.I2 same-shape update and rollback rehearsal: synthetic roots ONLY.
# The real '/' host is denied by the transaction helper before any write.
if [ "${1:-}" = update ] || [ "${1:-}" = rollback ]; then
  [ "$#" -eq 7 ] || stop 'Usage: install_media_tools.sh (update|rollback) OLD_STAGE NEW_STAGE SYNTHETIC_ROOT OLD_SHA256 NEW_SHA256 ISOLATED-ONLY'
  SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P) || stop 'script path unavailable'
  exec python3 "$SCRIPT_DIR/media_tools_runtime_transaction.py" "$1" \
    --previous-stage "$2" --new-stage "$3" --live-root "$4" \
    --previous-approved-sha256 "$5" --new-approved-sha256 "$6" \
    --confirm "$7"
fi
# Idempotent MT.I2 prepare: verify and reuse an existing prepared workdir.
# The old SHA --prepare BACKEND mode remains the explicitly building path.
if [ "${1:-}" = prepare ]; then
  [ "$#" -eq 3 ] || stop 'Usage: install_media_tools.sh prepare ABSOLUTE_WORKDIR REVIEWED_SHA256'
  SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P) || stop 'script path unavailable'
  exec python3 "$SCRIPT_DIR/media_tools_install_preflight.py" prepare \
    --prepared-workdir "$2" --approved-handler-sha256 "$3"
fi
# MT.I2 read-only entrypoints reuse the exact MT.I1 sealed-stage verifier.
# These operations do not fetch/build, elevate privileges, install or restart.
if [ "${1:-}" = check ] || [ "${1:-}" = verify ]; then
  [ "$#" -eq 4 ] || stop 'Usage: install_media_tools.sh (check|verify) ABSOLUTE_STAGE ABSOLUTE_ROOT REVIEWED_SHA256'
  ACTION=$1
  STAGE=$2
  TARGET_ROOT=$3
  APPROVED=$4
  SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P) || stop 'script path unavailable'
  exec python3 "$SCRIPT_DIR/media_tools_install_preflight.py" "$ACTION" \
    --stage "$STAGE" --live-root "$TARGET_ROOT" \
    --approved-handler-sha256 "$APPROVED"
fi
if [ "$#" -eq 1 ] && { [ "$1" = "--help" ] || [ "$1" = "-h" ]; }; then
  echo 'Usage: install_media_tools.sh COMMIT (--prepare|REVIEWED_SHA256) BACKEND [SOURCE_ROOT VIDEO_ROOT] [PREPARED_WORKDIR]'
  echo 'Reuse prepared: install_media_tools.sh prepare ABSOLUTE_WORKDIR REVIEWED_SHA256'
  echo 'Explicit offline live update: install_media_tools.sh live-update OLD_STAGE NEW_STAGE OLD_SHA256 NEW_SHA256 NEW_ROOT_BACKUP_DIR LIVE-UPDATE-REVIEWED'
  echo 'Isolated test only: install_media_tools.sh (update|rollback) OLD_STAGE NEW_STAGE SYNTHETIC_ROOT OLD_SHA256 NEW_SHA256 ISOLATED-ONLY'
  echo 'Read-only: install_media_tools.sh (check|verify) ABSOLUTE_STAGE ABSOLUTE_ROOT REVIEWED_SHA256'
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
  [ "${MEDIA_TOOLS_INSTALL_CONFIRM:-}" = REVIEWED_FIRST_INSTALL ] ||
    stop 'manual first install needs MEDIA_TOOLS_INSTALL_CONFIRM=REVIEWED_FIRST_INSTALL (no automatic grants or service start)'
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
# A newly installed or intentionally stopped Agent need not be running yet:
# this installer starts it only after the sealed runtime and approvals exist.
# Require a loaded unit with a clean prior result. A crashed/failed Agent
# must be diagnosed, never silently treated as an acceptable stopped unit.
AGENT_LOAD=$(systemctl show -p LoadState --value vdr-suite-backend-agent) ||
  stop "Backend Agent unit state unavailable"
AGENT_STATE=$(systemctl show -p ActiveState --value vdr-suite-backend-agent) ||
  stop "Backend Agent activity unavailable"
AGENT_RESULT=$(systemctl show -p Result --value vdr-suite-backend-agent) ||
  stop "Backend Agent result unavailable"
[ "$AGENT_LOAD" = loaded ] && [ "$AGENT_RESULT" = success ] ||
  stop "Backend Agent unit missing or failed (load=$AGENT_LOAD result=$AGENT_RESULT)"
case "$AGENT_STATE" in
  active|inactive) ;;
  *) stop "Backend Agent unsafe state before install: $AGENT_STATE" ;;
esac
echo "BACKEND_AGENT_PREFLIGHT_STATE=$AGENT_STATE"
systemctl cat vdr-suite-daemon | grep -Fq 'ExecStart=/usr/sbin/vdr-suite-daemon' || stop "noncanonical Suite service"
systemctl cat vdr-suite-backend-agent | grep -Fq 'ExecStart=/usr/sbin/vdr-suite-backend-agent --config /etc/vdr-suite/backend-agent.conf' || stop "noncanonical Agent service"
# Discover the effective service principal instead of presuming the distro
# runs every Backend Agent under an account literally named 'vdr'.
AGENT_ACCOUNT=$(systemctl show -p User --value vdr-suite-backend-agent) || stop "cannot inspect Agent identity"
[[ "$AGENT_ACCOUNT" =~ ^[a-z_][a-z0-9_-]*[$]?$ ]] &&
  [ "$AGENT_ACCOUNT" != root ] && id "$AGENT_ACCOUNT" >/dev/null 2>&1 ||
  stop "unsupported or privileged Backend Agent account"
AGENT_GROUP=$(systemctl show -p Group --value vdr-suite-backend-agent) || stop "cannot inspect Agent group"
if [ -z "$AGENT_GROUP" ]; then AGENT_GROUP=$(id -gn "$AGENT_ACCOUNT") || stop "cannot resolve Agent group"; fi
getent group "$AGENT_GROUP" >/dev/null || stop "Agent group missing"
for path in /etc/vdr-suite/addon-approvals/rectools.json \
  "/etc/vdr-suite/addon-approvals/rectools.$BACKEND.json" \
  /etc/vdr-suite/addon-approvals/rectools.import-running.lock \
  /etc/vdr-suite/addon-sources/media-import.tsv \
  /etc/systemd/system/vdr-suite-backend-agent.service.d/20-media-tools.conf \
  /var/lib/vdr-suite/backend-agent/media-shrink; do
  [ ! -e "$path" ] && [ ! -L "$path" ] || stop "pre-existing Media Tools state: $path"
done
[ -f /var/lib/vdr-suite/vdr-suite.db ] || stop "unknown active database location"
# Metadata is read by the service's actual account from the installed
# inventory. Fail before any stop if host-owned ancestors deny traversal;
# never chmod a live host directory implicitly.
for shared in /usr/libexec/vdr-suite /usr/libexec/vdr-suite/addons; do
  if [ -e "$shared" ] || [ -L "$shared" ]; then
    [ -d "$shared" ] && [ ! -L "$shared" ] &&
      sudo -u "$AGENT_ACCOUNT" test -x "$shared" ||
      stop "existing Agent metadata ancestor inaccessible to $AGENT_ACCOUNT: $shared (no permissions changed)"
  fi
done
# Never overlay files that are still owned by an older Debian add-on package.
# First-time source installation has a separate, reviewed migration path.
dpkg-query -W -f='${Status}' vdr-suite-addon-media-tools 2>/dev/null |
  grep -Fxq 'install ok installed' &&
  stop "existing Debian-owned Media Tools needs a separate migration, not an overlay"
if [ "$SOURCE_CONFIGURED" -eq 1 ]; then
  sudo -u "$AGENT_ACCOUNT" test -r "$SOURCE" || stop "vdr source access denied"
fi
if [ "$VIDEO_CONFIGURED" -eq 1 ]; then
  sudo -u "$AGENT_ACCOUNT" test -w "$VIDEO" || stop "vdr video access denied"
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
# One MT.I1 verifier, shared by manual install, read-only preflight and package staging.
# A first install must not silently overlay a package, partial install, or
# earlier root-owned grant. No host writes occur in this preflight.
python3 - "$STAGE" "$APPROVED" <<'PY' || stop "Media Tools first-install stage or host is not clean"
import pathlib, sys
sys.path.insert(0, "tools/addons")
from media_tools_install_preflight import inspect
report = inspect(pathlib.Path(sys.argv[1]), pathlib.Path("/"), sys.argv[2])
print("MEDIA_TOOLS_FIRST_INSTALL_STATE=" + report["install_state"])
if report["install_state"] != "absent":
    raise SystemExit("existing Media Tools needs an explicit reviewed update/migration")
PY
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
    echo 'MEDIA_TOOLS_SERVICE_STATE=STOPPED_REQUIRES_MANUAL_REVIEW' >&2
    echo 'NO_AUTOMATIC_RESTART_OR_APPROVAL_ON_INSTALL_FAILURE' >&2
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
sudo install -d -o "$AGENT_ACCOUNT" -g "$AGENT_GROUP" -m 0700 /var/lib/vdr-suite/backend-agent/media-shrink || stop "private shrink state"
sudo systemctl daemon-reload || stop "systemd reload"
sudo make check-install-runtime-deployment RUNTIME_STAGE="$STAGE" LIVE_ROOT=/ ||
  stop "installed Suite + Media Tools runtime drift"
resource_preflight after || stop "post-install reserve insufficient"
RESTART=0
echo 'SUITE_SERVICES=STOPPED_REQUIRES_EXPLICIT_OPERATOR_START'
echo 'MEDIA_TOOLS_HANDLER_APPROVAL=NOT_ISSUED'
echo 'MEDIA_TOOLS_CORE_APPROVAL=NOT_ISSUED'
echo "MEDIA_TOOLS_INSTALL=PASS"
echo "DATABASE_BACKUP=$WORK/backup/vdr-suite.db"
echo 'IMPORT_AND_SHRINK_AGENT_OPT_IN=CONFIGURED_BUT_INACTIVE'
if [ "$SOURCE_CONFIGURED" -eq 1 ] && [ "$VIDEO_CONFIGURED" -eq 1 ]; then
  echo 'MEDIA_TOOLS_PATHS=CONFIGURED'
else
  echo 'MEDIA_TOOLS_PATHS=PENDING'
fi
# File installation does not approve the handler, start the Agent, or grant
# any actor. Root/Core approvals and service restart require their separate,
# already existing explicit operator procedures.
echo 'MEDIA_TOOLS_ACTIVATION=INACTIVE_PENDING_SEPARATE_REVIEW'
echo 'OTHER_USER_GRANTS=UNCHANGED'
