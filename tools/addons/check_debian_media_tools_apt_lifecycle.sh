#!/usr/bin/env bash
# MT.I4.2: real apt/dpkg lifecycle on a disposable CI Ubuntu container only.
set -euo pipefail
series="${1:?expected noble or resolute}"
case "$series" in noble|resolute) ;; *) exit 2 ;; esac
test "$(id -u)" -eq 0
test -f /.dockerenv
test "$(dpkg --print-architecture)" = amd64
test "$(. /etc/os-release; printf '%s' "$VERSION_CODENAME")" = "$series"
# CI mounts the checkout under a different uid inside the disposable container.
# Trust only this exact workspace for this single read-only Git identity check.
test "$(git -c safe.directory="$PWD" rev-parse HEAD)" = "$GITHUB_SHA"
shopt -s nullglob
packages=(media-tools/vdr-suite-addon-media-tools_*_amd64.deb)
test "${#packages[@]}" -eq 1
package="${packages[0]}"
test "$(dpkg-deb --field "$package" Package)" = vdr-suite-addon-media-tools
test "$(dpkg-deb --field "$package" Architecture)" = amd64
version="$(dpkg-deb --field "$package" Version)"
case "$version" in *~exp2) previous="${version%~exp2}~exp1" ;; *) exit 2 ;; esac
dpkg --compare-versions "$version" gt "$previous"
dpkg-deb --field "$package" Depends | grep -Fw ffmpeg
dpkg-deb --field "$package" Depends | grep -Fw vdr
# Do not run this on a real host: no host services or state are used.
printf '%s\n' '#!/bin/sh' 'exit 101' > /usr/sbin/policy-rc.d
chmod 0755 /usr/sbin/policy-rc.d
apt-get update
apt-get install -y --no-install-recommends ca-certificates gnupg software-properties-common python3
add-apt-repository -y ppa:seahawk1986-hotmail/vdr-2.8.3
apt-get update
apt-get install -y --no-install-recommends vdr ffmpeg
case "$(dpkg-query -W -f='${Version}' vdr)" in 2.8.3-*) ;; *) echo MT_I42_VDR_VERSION=FAIL; exit 1 ;; esac
# Independent operator state, unrelated to this package.
mkdir -p /etc/vdr-suite/addon-approvals /var/lib/vdr-suite /srv/vdr/video.00/holiday
printf 'fixture config\n' > /etc/vdr-suite/backend-agent.conf
printf 'fixture approval\n' > /etc/vdr-suite/addon-approvals/rectools.json
printf 'fixture sqlite bytes\n' > /var/lib/vdr-suite/vdr-suite.db
printf 'fixture recording bytes\n' > /srv/vdr/video.00/holiday/001.vdr
snapshot="$(mktemp)"
sha256sum /etc/vdr-suite/backend-agent.conf /etc/vdr-suite/addon-approvals/rectools.json \
  /var/lib/vdr-suite/vdr-suite.db /srv/vdr/video.00/holiday/001.vdr > "$snapshot"
# Exercise the actual package preinst: manual worker must be preserved.
target=/usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import
mkdir -p "$(dirname "$target")"
printf 'manual worker\n' > "$target"
original="$(sha256sum "$target")"
failed_log="$(mktemp)"
if apt-get -y install "./$package" >"$failed_log" 2>&1; then
  cat "$failed_log"
  echo MT_I42_UNEXPECTED_COLLISION_INSTALL=FAIL
  exit 1
fi
cat "$failed_log"
grep -F 'MEDIA_TOOLS_DPKG=BLOCKED' "$failed_log"
test "$(sha256sum "$target")" = "$original"
sha256sum -c "$snapshot"
echo MT_I42_REAL_APT_MANUAL_COLLISION=PASS
rm -- "$target"
# Change only the control Version in a disposable copy of the real .deb.
# This is a synthetic prior-version fixture, NOT a historical release/build.
old_root="$(mktemp -d)"
old_deb="$(mktemp --suffix=.deb)"
dpkg-deb --raw-extract "$package" "$old_root"
sed -i "s/^Version: .*/Version: $previous/" "$old_root/DEBIAN/control"
test "$(grep -c "^Version: $previous$" "$old_root/DEBIAN/control")" -eq 1
dpkg-deb --build --root-owner-group "$old_root" "$old_deb"
apt-get -y install "$old_deb"
test "$(dpkg-query -W -f='${Version}' vdr-suite-addon-media-tools)" = "$previous"
sha256sum -c "$snapshot"
echo MT_I42_SYNTHETIC_PREVIOUS_INSTALL=PASS
apt-get -y install "./$package"
test "$(dpkg-query -W -f='${Version}' vdr-suite-addon-media-tools)" = "$version"
test "$(dpkg-query -W -f='${Status}' vdr-suite-addon-media-tools)" = 'install ok installed'
sha256sum -c "$snapshot"
echo MT_I42_REAL_APT_UPGRADE=PASS
# Every installed payload byte and mode must match the exact downloaded .deb.
extracted="$(mktemp -d)"
dpkg-deb --extract "$package" "$extracted"
python3 tools/addons/debian_media_tools.py --verify-package-root --package-root "$extracted"
python3 - "$extracted" <<'PY'
import hashlib
import pathlib
import stat
import subprocess
import sys
sys.path.insert(0, 'tools/addons')
from build_media_tools_cpp_deb import stage_package_paths
root = pathlib.Path(sys.argv[1])
for rel in stage_package_paths():
    expected = root / rel
    installed = pathlib.Path('/') / rel
    assert expected.is_file() and installed.is_file() and not installed.is_symlink(), rel
    assert hashlib.sha256(expected.read_bytes()).digest() == hashlib.sha256(installed.read_bytes()).digest(), rel
    assert stat.S_IMODE(expected.stat().st_mode) == stat.S_IMODE(installed.stat().st_mode), rel
    owner = subprocess.check_output(['dpkg-query', '-S', str(installed)], text=True).strip()
    assert owner == 'vdr-suite-addon-media-tools: ' + str(installed), (rel, owner)
print('MT_I42_EIGHT_INSTALLED_FILES_AND_OWNERSHIP=PASS')
PY
apt-get -y --reinstall install "./$package"
sha256sum -c "$snapshot"
echo MT_I42_REAL_APT_REINSTALL=PASS
apt-get -y remove vdr-suite-addon-media-tools
test ! -e "$target"
sha256sum -c "$snapshot"
echo MT_I42_REAL_APT_REMOVE=PASS
# This exact-run .deb is local, not indexed in an APT repository. After
# removing its conffile-free package, APT cannot resolve the old package name.
# Reinstall the same .deb so purge exercises an INSTALLED package separately.
apt-get -y install "./$package"
test "$(dpkg-query -W -f='${Status}' vdr-suite-addon-media-tools)" = 'install ok installed'
test -x "$target"
sha256sum -c "$snapshot"
echo MT_I42_REAL_APT_REINSTALL_BEFORE_PURGE=PASS
apt-get -y purge vdr-suite-addon-media-tools
test ! -e "$target"
sha256sum -c "$snapshot"
echo MT_I42_REAL_APT_PURGE_PRESERVATION=PASS
