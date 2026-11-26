#!/usr/bin/env python3
# Read-only installed package verifier, shared by the real APT lifecycle and the
# future published-PPA smoke. These bytes alone never activate the add-on.
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
source = record["prepared_source_sha"]
if not re.fullmatch(r"[a-f0-9]{40}", source):
    prefix = re.fullmatch(r"git-prefix:([0-9a-f]{7,39})", source)
    assert prefix, source
    version = subprocess.check_output(
        ["dpkg-query", "-W", "-f=${Version}", package], text=True
    ).strip()
    assert re.search(r"\+g" + prefix[1] + r"~ubuntu[0-9.]+$", version), (source, version)
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
print("MT_I43_INSTALLED_STAGE_INTEGRITY=PASS files=8 workers=3 inactive=true")
