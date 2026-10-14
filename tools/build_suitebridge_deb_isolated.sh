#!/usr/bin/env bash
# Non-root, isolated plugin-only Debian package experiment. Never deploys.
set -Eeuo pipefail
umask 077

fail() {
    printf 'SUITEBRIDGE_BUILD_STOP=%s\n' "$*" >&2
    exit 1
}

test "${1-}" != "" || fail "provide the verified immutable branch SHA as first argument"
expected="$1"
[[ "$expected" =~ ^[0-9a-f]{40}$ ]] || fail "expected SHA must be 40 lowercase hexadecimal characters"
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
branch=work/packaging-debian-ubuntu-install

[[ "$(id -un)" == "yavdr" ]] || fail "must run as yavdr (never root)"
[[ "$root" == "/home/yavdr/vdr-suite-packaging-test" ]] || fail "wrong checkout root: $root"
[[ "$(git -C "$root" rev-parse --show-toplevel)" == "$root" ]] || fail "not the independent repository checkout"
[[ "$(git -C "$root" branch --show-current)" == "$branch" ]] || fail "wrong branch"
[[ "$(git -C "$root" rev-parse HEAD)" == "$expected" ]] || fail "candidate mismatch"
[[ -z "$(git -C "$root" status --porcelain)" ]] || fail "worktree is not clean"
remote="$(git -C "$root" ls-remote --exit-code origin "refs/heads/$branch" | awk '{print $1}')" ||
    fail "cannot verify remote branch"
[[ "$remote" == "$expected" ]] || fail "remote branch changed; recheck before build"
printf 'CANDIDATE_IDENTITY=PASS %s\n' "$expected"

missing=0
for executable in git make g++ python3 pkg-config dpkg-buildpackage \
    dpkg-checkbuilddeps dpkg-deb dh dh_vdrplugin_depends df awk grep find \
    mktemp tee; do
    if ! command -v "$executable" >/dev/null 2>&1; then
        printf 'MISSING_BUILD_TOOL=%s\n' "$executable"
        missing=1
    fi
done
(( missing == 0 )) || fail "build tools missing; nothing installed or modified"
if ! pkg-config --exists vdr; then
    fail "vdr.pc unavailable; target distribution VDR development package/config not detected"
fi
vdr_libdir="$(pkg-config --variable=libdir vdr)"
vdr_api="$(pkg-config --variable=apiversion vdr)"
printf 'VDR_PLUGIN_LIBDIR=%s\nVDR_API=%s\n' "$vdr_libdir" "$vdr_api"
[[ "$vdr_libdir" == "/usr/lib/vdr/plugins" ]] ||
    fail "plugin install manifest expects /usr/lib/vdr/plugins; inspect pkg-config before build"
[[ "$vdr_api" =~ ^[A-Za-z0-9_.+-]+$ ]] || fail "invalid or empty VDR API version"

if ! (cd "$root" && dpkg-checkbuilddeps -B -Pplugin-only); then
    fail "plugin-only build dependencies missing (read-only check); do not install packages on productive yaVDR"
fi
printf 'BUILD_DEPENDENCIES=PASS\n'

# The entire isolated source + binary build + logs must fit this conservative
# additional space budget. It is an acceptance budget, not a reservation.
base=/home/yavdr
read -r total free < <(df -B1 --output=size,avail "$base" | awk 'NR==2 {print $1, $2}')
[[ "$total" =~ ^[0-9]+$ && "$free" =~ ^[0-9]+$ ]] || fail "unable to read filesystem capacity"
reserve=$((total * 15 / 100))
(( reserve >= 4294967296 )) || reserve=4294967296
budget=2147483648
printf 'RESOURCE_TOTAL=%s RESOURCE_FREE=%s RESOURCE_RESERVE=%s BUILD_BUDGET=%s\n' \
    "$total" "$free" "$reserve" "$budget"
(( free >= reserve + budget )) || fail "resource reserve would be violated"
printf 'RESOURCE_PREFLIGHT=PASS\n'

# No staging or runtime writes until all of the above gates have passed.
work="$(mktemp -d "$base/vdr-suite-pkgbuild-XXXXXXXX")" ||
    fail "unable to create bounded build area"
printf 'BUILD_AREA=%s\n' "$work"
trap 'result=$?; if (( result != 0 )); then printf "SUITEBRIDGE_BUILD_FAILED=%s BUILD_AREA=%s\n" "$result" "$work" >&2; fi' EXIT

git clone --no-local --quiet --single-branch --branch "$branch" "$root" "$work/src"
[[ "$(git -C "$work/src" rev-parse HEAD)" == "$expected" ]] ||
    fail "isolated source clone candidate changed"
printf 'ISOLATED_SOURCE=PASS\n'

(
    cd "$work/src"
    # Capped concurrency; build and debhelper remain wholly inside the
    # disposable local test source. No sudo, apt or service commands.
    export DEB_BUILD_OPTIONS=parallel=2
    dpkg-buildpackage -B -Pplugin-only -us -uc
) 2>&1 | tee "$work/build.log"

shopt -s nullglob
packages=("$work"/vdr-plugin-suitebridge_*.deb)
daemon=("$work"/vdr-suite-daemon_*.deb)
agents=("$work"/vdr-suite-backend-agent_*.deb)
(( ${#packages[@]} == 1 )) || fail "expected exactly one SuiteBridge binary package"
(( ${#daemon[@]} == 0 && ${#agents[@]} == 0 )) ||
    fail "plugin-only build unexpectedly produced server packages"
package="${packages[0]}"
depends="$(dpkg-deb --field "$package" Depends)"
printf 'PLUGIN_PACKAGE=%s\nPLUGIN_DEPENDS=%s\n' "$package" "$depends"
[[ "$depends" != *vdr-suite-daemon* && "$depends" != *vdr-suite-backend-agent* ]] ||
    fail "plugin binary package incorrectly depends on server binaries"
[[ "$depends" == *vdr-abi-* ]] || fail "VDR ABI dependency is missing"
dpkg-deb --contents "$package"
printf 'PLUGIN_ONLY_DEB=PASS\n'

read -r total_after free_after < <(df -B1 --output=size,avail "$base" | awk 'NR==2 {print $1, $2}')
(( free_after >= reserve )) || fail "post-build filesystem reserve violated"
printf 'RESOURCE_POSTCHECK=PASS\n'
printf 'RESULT=PASS\n'
