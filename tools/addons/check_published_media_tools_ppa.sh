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
printf '%s\n' "$policy" |
  grep -F "ppa.launchpadcontent.net/hotzenplotz5/vdr-suite/ubuntu $series/"
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
python3 - <<'PY'
import hashlib
import json
import pathlib
import re
import stat
import subprocess

package = "vdr-suite-addon-media-tools"
inventory_rel = "usr/share/vdr-suite/addons/rectools/media-tools-install-inventory.json"
path = pathlib.Path("/") / inventory_rel
assert path.is_file() and not path.is_symlink()
record = json.loads(path.read_text(encoding="utf-8"))
assert record["schema"] == 1
assert record["module"] == "rectools"
assert record["package"] == package
assert re.fullmatch(r"[a-f0-9]{40}", record["prepared_source_sha"])
entries = record["files"]
assert isinstance(entries, dict) and len(entries) == 7
prefixes = ("usr/share/vdr-suite/addons/rectools/",
            "usr/libexec/vdr-suite/addons/rectools/")
roles = {"metadata": 0, "agent-metadata": 0, "worker": 0}

def check_owner(rel):
    target = pathlib.Path("/") / rel
    owner = subprocess.check_output(
        ["dpkg-query", "-S", str(target)], text=True
    ).strip()
    assert owner == package + ": " + str(target), (rel, owner)

check_owner(inventory_rel)
assert stat.S_IMODE(path.stat().st_mode) == 0o644
for rel, detail in entries.items():
    assert isinstance(rel, str) and rel.startswith(prefixes)
    assert ".." not in pathlib.PurePosixPath(rel).parts
    assert detail["role"] in roles
    roles[detail["role"]] += 1
    file = pathlib.Path("/") / rel
    assert file.is_file() and not file.is_symlink(), rel
    data = file.read_bytes()
    assert len(data) == detail["size"], rel
    assert hashlib.sha256(data).hexdigest() == detail["sha256"], rel
    assert stat.S_IMODE(file.stat().st_mode) == detail["mode"], rel
    if detail["role"] == "worker":
        assert data[:4] == b"\x7fELF" and detail["mode"] == 0o755, rel
    else:
        assert detail["mode"] == 0o644, rel
    check_owner(rel)
assert roles == {"metadata": 3, "agent-metadata": 1, "worker": 3}
assert (pathlib.Path("/usr/libexec/vdr-suite/addons/rectools/addon.json").read_bytes()
        == pathlib.Path("/usr/share/vdr-suite/addons/rectools/addon.json").read_bytes())
module = json.loads(pathlib.Path(
    "/usr/share/vdr-suite/addons/rectools/addon.json"
).read_text(encoding="utf-8"))
assert module["state"] == "scaffold"
assert module["version"] == record["version"]
assert module["package"] == record["package"]
present = set()
for line in subprocess.check_output(
        ["dpkg-query", "-L", package], text=True).splitlines():
    normalized = line.lstrip("/")
    if normalized.startswith(prefixes) and pathlib.Path(line).is_file():
        present.add(normalized)
assert present == set(entries) | {inventory_rel}, (present, entries)
print("MT_I43_PUBLISHED_STAGE_INTEGRITY=PASS files=8 workers=3 inactive=true")
PY
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
