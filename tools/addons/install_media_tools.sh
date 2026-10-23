#!/usr/bin/env bash
# Media Tools first-time installation on an already enrolled yaVDR.
set -Eeuo pipefail
stop() { printf 'MEDIA_TOOLS_INSTALL=STOP %s\n' "$*" >&2; exit 1; }
if [ "$#" -eq 1 ] && { [ "$1" = "--help" ] || [ "$1" = "-h" ]; }; then
  echo 'Usage: install_media_tools.sh EXPECTED_COMMIT REVIEWED_HANDLER_SHA256 BACKEND_ID SOURCE_ROOT VIDEO_ROOT'
  exit 0
fi
if [ "$#" -ne 5 ] && [ "$#" -ne 6 ]; then
  echo 'Usage: install_media_tools.sh COMMIT (--prepare|REVIEWED_SHA256) BACKEND SOURCE_ROOT VIDEO_ROOT [PREPARED_WORKDIR]' >&2
  exit 2
fi
SHA=$1; APPROVED=$2; BACKEND=$3; SOURCE=$4; VIDEO=$5
PREPARED_WORKDIR=
if [ "$#" -eq 6 ]; then PREPARED_WORKDIR=$6; fi
MODE=install
if [ "$APPROVED" = --prepare ]; then MODE=prepare; fi
[[ "$SHA" =~ ^[0-9a-f]{40}$ ]] || stop "commit SHA invalid"
if [ "$MODE" = install ]; then
  [[ "$APPROVED" =~ ^[0-9a-f]{64}$ ]] || stop "independent approved handler SHA-256 required"
else
  [ -z "$PREPARED_WORKDIR" ] || stop "prepare cannot reuse an existing build"
fi
[[ "$BACKEND" =~ ^[A-Za-z0-9_.:-]{1,128}$ ]] && [ "$BACKEND" != . ] && [ "$BACKEND" != .. ] || stop "backend invalid"
[ "$(id -u)" -ne 0 ] || stop "run as an ordinary user with sudo"
ROOT=$(cd "$(dirname "$0")/../.." && pwd -P) || stop "checkout absent"
cd "$ROOT" || stop "checkout absent"
[ "$(git rev-parse --show-toplevel)" = "$ROOT" ] || stop "wrong checkout"
[ "$(git branch --show-current)" = work/addon-platform-registry-lifecycle ] || stop "wrong branch"
[ "$(git rev-parse HEAD)" = "$SHA" ] || stop "not tested HEAD"
[ -z "$(git status --porcelain --untracked-files=no)" ] || stop "dirty checkout"
case "$(git remote get-url origin)" in
  *github.com:hotzenplotz5/vdr-suite.git|*github.com/hotzenplotz5/vdr-suite.git|*github.com/hotzenplotz5/vdr-suite) ;;
  *) stop "unexpected origin" ;;
esac
# Never overwrite newer installed Suite Core with an old add-on branch.
git fetch --quiet --no-tags origin main || stop "cannot check current main"
git merge-base --is-ancestor FETCH_HEAD HEAD || stop "main-not-integrated"
REMOTE=$(git ls-remote origin refs/heads/work/addon-platform-registry-lifecycle | cut -f1)
[ "$REMOTE" = "$SHA" ] || stop "remote branch advanced"
for cmd in sudo make g++ python3 dpkg dpkg-deb sha256sum pkg-config realpath; do
  command -v "$cmd" >/dev/null || stop "missing dependency $cmd"
done
pkg-config --exists sqlite3 libcurl openssl || stop "development dependencies missing"
for dir in "$SOURCE" "$VIDEO"; do
  [ -d "$dir" ] && [[ "$dir" = /* ]] && [ "$(realpath -e "$dir")" = "$dir" ] || stop "unsafe media directory"
  [[ "$dir" != *$'\t'* && "$dir" != *$'\n'* ]] || stop "unsafe media path characters"
done
[ "$SOURCE" != "$VIDEO" ] || stop "same source and destination"
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
dpkg -s vdr-suite-addon-media-tools 2>/dev/null | grep -Fq 'Status: install ok installed' && stop "existing installed package needs separate update procedure"
sudo -u vdr test -r "$SOURCE" && sudo -u vdr test -w "$VIDEO" || stop "vdr media access denied"
# The first write is allowed only if every affected FS retains 4 GiB / 15%.
resource_preflight() {
python3 - "$HOME" <<'PY' || stop "resource preflight"
import os, shutil, sys
checks = [(sys.argv[1], 8*1024**3), ('/usr', 1024**3), ('/var', 2*1024**3)]
grouped = {}
for path, budget in checks:
    device = os.stat(path).st_dev
    original = grouped.get(device, (path, 0))
    grouped[device] = (original[0], original[1] + budget)
ok = True
for path, budget in grouped.values():
    u = shutil.disk_usage(path)
    reserve = max(4*1024**3, (u.total*15+99)//100)
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
  [ -f "$WORK/candidate.txt" ] && [ -f "$WORK/stage/.vdr-suite-install-runtime-manifest.json" ] || stop "prepared build missing"
  [ "$(sed -n '1p' "$WORK/candidate.txt")" = "$SHA" ] || stop "prepared build SHA differs"
else
  WORK=$(mktemp -d "$HOME/vdr-suite-media-tools.XXXXXXXX") || stop "unable to create work directory"
  mkdir -m 0700 "$WORK/build" "$WORK/tmp" "$WORK/packages"
  export TMPDIR="$WORK/tmp"
  make -j2 stage-install-runtime BUILD_DIR="$WORK/build" RUNTIME_STAGE="$WORK/stage" || stop "Suite build failed"
  make -j2 build-media-tools-cpp-deb BUILD_DIR="$WORK/build" OUTPUT_DIR="$WORK/packages" ADDON_MAINTAINER='Local VDR operator <local@vdr-suite.invalid>' || stop "Media Tools build failed"
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
PACKAGE=$(find "$WORK/packages" -maxdepth 1 -name 'vdr-suite-addon-media-tools_*.deb' -type f -print)
[ -f "$PACKAGE" ] && [ "$(dpkg-deb -f "$PACKAGE" Package)" = vdr-suite-addon-media-tools ] || stop "invalid add-on package"
[ "$(dpkg-deb -f "$PACKAGE" Version)" = '0.1.0~cpppreview1' ] || stop "unexpected module version"
[ "$(dpkg-deb -f "$PACKAGE" Architecture)" = "$(dpkg --print-architecture)" ] || stop "wrong architecture"
dpkg-deb --contents "$PACKAGE" || stop "package contents unreadable"
if [ "$MODE" = prepare ]; then
  printf '%s\n%s\n' "$SHA" "$DIGEST" > "$WORK/candidate.txt"
  echo "MEDIA_TOOLS_PREPARED=YES"
  echo "WORKDIR=$WORK"
  echo "REVIEW_HANDLER_SHA256=$DIGEST"
  echo 'INSTALL_REQUIRES_INDEPENDENT_APPROVAL=YES'
  exit 0
fi
resource_preflight || stop "insufficient capacity after build"
mkdir -m 0700 "$WORK/backup" || stop "backup directory already present"
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
sudo python3 - /var/lib/vdr-suite/vdr-suite.db "$WORK/backup/vdr-suite.db" <<'PY'
import os, sqlite3, sys
source, dest = sys.argv[1:]
if os.path.getsize(source) > 3 * 1024**3:
    raise SystemExit('database too large for reserved backup budget')
with sqlite3.connect(f'file:{source}?mode=ro', uri=True) as db:
    copy = sqlite3.connect(dest)
    try: db.backup(copy)
    finally: copy.close()
os.chmod(dest, 0o600)
PY
sudo dpkg -i "$PACKAGE" || stop "add-on package installation"
sudo make deploy-install-runtime RUNTIME_STAGE="$WORK/stage" LIVE_ROOT=/ || stop "Suite runtime deployment"
sudo make check-install-runtime-deployment RUNTIME_STAGE="$WORK/stage" LIVE_ROOT=/ || stop "runtime did not match stage"
ISSUER="$ROOT/tools/addons/admin_approval_issuer.py"
sudo python3 "$ISSUER" issue --root / --allow-live-root --module rectools --version 0.1.0 --origin debian --approved-sha256 "$APPROVED" || stop "backend approval"
sudo python3 "$ISSUER" issue --root / --allow-live-root --core-expected-only --backend-id "$BACKEND" --module rectools --version 0.1.0 --origin debian --approved-sha256 "$APPROVED" || stop "Core approval"
sudo python3 - "$BACKEND" "$SOURCE" "$VIDEO" <<'PY'
import os, pathlib, stat, sys
backend, source, video = sys.argv[1:]
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
create('/etc/vdr-suite/addon-sources/media-import.tsv',
       f'{backend}\tlocal-media-import\t{source}\t{video}\t*\n')
create('/etc/systemd/system/vdr-suite-backend-agent.service.d/20-media-tools.conf',
       '[Service]\nExecStart=\nExecStart=/usr/sbin/vdr-suite-backend-agent --config /etc/vdr-suite/backend-agent.conf --enable-media-import --enable-media-shrink --media-shrink-state-dir /var/lib/vdr-suite/backend-agent/media-shrink\n')
PY
sudo install -d -o vdr -g vdr -m 0700 /var/lib/vdr-suite/backend-agent/media-shrink || stop "private shrink state"
sudo systemctl daemon-reload || stop "systemd reload"
sudo systemctl start vdr-suite-daemon vdr-suite-backend-agent || stop "Suite restart"
systemctl is-active --quiet vdr-suite-daemon && systemctl is-active --quiet vdr-suite-backend-agent || stop "Suite/Agent failed to start"
sudo make check-install-runtime-deployment RUNTIME_STAGE="$WORK/stage" LIVE_ROOT=/ || stop "installed runtime drift"
RESTART=0
resource_preflight || stop "post-install reserve insufficient"
echo "MEDIA_TOOLS_INSTALL=PASS"
echo "DATABASE_BACKUP=$WORK/backup/vdr-suite.db"
echo 'IMPORT_AND_SHRINK_AGENT_OPT_IN=YES'
echo 'CORE_ADMIN_ACTIVATION_AND_USER_GRANTS=PENDING'
