#!/usr/bin/env bash
# MT.I4.3: opt-in verification of an ACTUALLY PUBLISHED Media Tools .deb.
# Called only by the existing disposable Noble/Resolute PPA smoke workflows.
# No Launchpad upload, runtime activation, migration or live-host operation.
set -euo pipefail

series="${1:?usage: check_published_media_tools_ppa.sh noble|resolute}"
case "$series" in noble|resolute) ;; *) printf 'MT_I43_SERIES=INVALID\n' >&2; exit 1 ;; esac
test "$(. /etc/os-release; printf '%s' "$VERSION_CODENAME")" = "$series"
test "$(dpkg --print-architecture)" = amd64

pkg=vdr-suite-addon-media-tools
policy="$(apt-cache policy "$pkg")"
printf '%s\n' "$policy"
candidate="$(printf '%s\n' "$policy" | awk '/Candidate:/ {print $2; exit}')"
test -n "$candidate" && test "$candidate" != '(none)' || {
  echo 'MT_I43_PUBLISHED_ADDON=NOT_AVAILABLE (no install or acceptance)' >&2
  exit 1
}
# The PPA URL must belong to the selected candidate's VERSION ROW.
# A PPA offering only an older version must not validate a foreign candidate.
printf '%s\n' "$policy" | awk \
  -v candidate="$candidate" \
  -v origin="ppa.launchpadcontent.net/hotzenplotz5/vdr-suite/ubuntu" \
  -v series="$series" '
    $1 == "***" && $3 ~ /^[0-9]+$/ { active=($2 == candidate); next }
    $2 ~ /^[0-9]+$/ && $1 != "Candidate:" { active=($1 == candidate); next }
    active && index($0, origin) && index($0, " " series "/") { accepted=1 }
    END { exit !accepted }
  ' || {
    echo "MT_I43_CANDIDATE_PPA_ORIGIN=INVALID version=$candidate" >&2
    exit 1
  }
# All four binaries from the same source recipe carry one version.
for required in vdr-suite-daemon vdr-suite-backend-agent vdr-plugin-suitebridge; do
  installed="$(dpkg-query -W -f='${Version}' "$required")"
  test "$installed" = "$candidate" || {
    printf 'MT_I43_SOURCE_VERSION_MISMATCH pkg=%s suite=%s candidate=%s\n' \
      "$required" "$installed" "$candidate" >&2
    exit 1
  }
done
test "$(dpkg-query -W -f='${Version}' vdr)" = "2.8.3-0yavdr0~$series"
dpkg-query -W -f='${Provides}\n' vdr | grep -Fw vdr-abi-2.8.3-0yavdr
apt-get -s install --no-install-recommends "$pkg=$candidate"
installer=()
if test "$(id -u)" -ne 0; then installer=(sudo); fi
"${installer[@]}" env DEBIAN_FRONTEND=noninteractive \
  apt-get install -y --no-install-recommends "$pkg=$candidate"
test "$(dpkg-query -W -f='${Version}' "$pkg")" = "$candidate"
test "$(dpkg-query -W -f='${Status}' "$pkg")" = 'install ok installed'
dpkg --audit
dependencies="$(dpkg-query -W -f='${Depends}' "$pkg")"
printf '%s\n' "$dependencies" | grep -Fw ffmpeg
printf '%s\n' "$dependencies" | grep -Fw vdr

# Inspect the installed inventory itself, not a second local payload list.
# The inventory is descriptive, never an add-on activation or grant.
python3 tools/addons/check_installed_media_tools_payload.py
for worker in /usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-*; do
  test -x "$worker"
  ldd "$worker" > /tmp/vdr-suite-media-tools-ppa-ldd.log
  if grep -Fq 'not found' /tmp/vdr-suite-media-tools-ppa-ldd.log; then
    cat /tmp/vdr-suite-media-tools-ppa-ldd.log
    echo "MT_I43_WORKER_LIBRARY_MISSING=$worker" >&2
    exit 1
  fi
done
echo "MT_I43_PUBLISHED_PPA_APT=PASS series=$series version=$candidate"
