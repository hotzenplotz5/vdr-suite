#!/usr/bin/env python3
"""Read-only deterministic packaging inventory of isolated DESTDIR trees."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import stat
import posixpath

SENSITIVE_PREFIXES = ("etc/vdr-suite/", "var/lib/vdr-suite/", "var/cache/vdr-suite/")
ALLOWED_PREFIXES = ("usr/", "etc/vdr-suite/", "etc/default/vdr-suite-", "var/lib/vdr-suite/", "var/cache/vdr-suite/", "lib/systemd/system/vdr-suite-", "usr/lib/systemd/system/vdr-suite-")
PLUGIN_RE = re.compile(r"^libvdr-suitebridge\.so\.[A-Za-z0-9_.+-]+$")

def package_for(path, origin):
    if not path.as_posix().startswith(ALLOWED_PREFIXES):
        raise ValueError("unexpected install destination: " + str(path))
    name = path.name
    if origin == "plugin":
        if PLUGIN_RE.fullmatch(name):
            return "vdr-plugin-suitebridge"
        raise ValueError("unexpected SuiteBridge stage payload: " + str(path))
    if path.as_posix() in ("lib/systemd/system/vdr-suite-backend-agent.service", "usr/lib/systemd/system/vdr-suite-backend-agent.service"):
        return "vdr-suite-backend-agent"
    if name.startswith("vdr-suite-backend-agent") or path.as_posix().endswith("/backend-agent.conf") or "backend-agent" in path.parts:
        return "vdr-suite-backend-agent"
    if name.startswith("vdr-suite-") or name.startswith("vdr-suite.") or "vdr-suite" in path.parts or name.startswith("vdr-suite"):
        return "vdr-suite-daemon"
    if path.as_posix().startswith("lib/systemd/system/vdr-suite-"):
        return "vdr-suite-daemon"
    if path.as_posix().startswith("etc/default/vdr-suite-"):
        return "vdr-suite-daemon"
    raise ValueError("unowned staged path: " + str(path))

def elf_needed(path):
    with path.open("rb") as stream:
        if stream.read(4) != b"\x7fELF":
            return None
    try:
        run = subprocess.run(["readelf", "-d", str(path)], text=True,
                             capture_output=True, check=True)
    except (FileNotFoundError, subprocess.CalledProcessError) as exc:
        raise RuntimeError("readelf required for ELF dependency audit: " + str(path)) from exc
    return sorted(set(re.findall(r"\(NEEDED\).*?\[([^\]]+)\]", run.stdout)))

def inspect(root, origin):
    if not root.is_dir() or root.is_symlink():
        raise ValueError("stage root must be an existing real directory: " + str(root))
    entries = []
    for directory, dirs, files in os.walk(root, followlinks=False):
        dirs.sort()
        files.sort()
        for name in dirs + files:
            full = Path(directory) / name
            relative = full.relative_to(root)
            if full.is_dir() and not full.is_symlink():
                continue
            owner = package_for(relative, origin)
            mode = full.lstat().st_mode
            record = {"path": "/" + relative.as_posix(), "package": owner,
                      "origin": origin, "mode": format(stat.S_IMODE(mode), "04o")}
            relative_name = relative.as_posix()
            if relative_name.startswith("var/lib/vdr-suite/secrets/") or relative_name.startswith("var/lib/vdr-suite/backend-agent/"):
                raise ValueError("persistent private data must not be shipped in package: " + relative_name)
            if stat.S_ISREG(mode) and stat.S_IMODE(mode) & 0o002:
                raise ValueError("world writable package file: " + relative_name)
            if relative_name.startswith("etc/vdr-suite/") or relative_name.startswith("etc/default/"):
                record["config_candidate"] = True
            if full.is_symlink():
                target = os.readlink(full)
                if not target:
                    raise ValueError("empty link: " + relative_name)
                resolved = posixpath.normpath(posixpath.join(posixpath.dirname("/" + relative_name), target)) if not target.startswith("/") else posixpath.normpath(target)
                if not resolved.startswith(("/usr/", "/lib/", "/etc/")):
                    raise ValueError("symlink points outside allowed package roots: " + relative_name + " -> " + target)
                record.update(type="symlink", target=target, resolved=resolved)
            elif full.is_file():
                record.update(type="file", size=full.stat().st_size,
                              sha256=hashlib.sha256(full.read_bytes()).hexdigest())
                needed = elf_needed(full)
                if needed is not None:
                    record["elf_needed"] = needed
            else:
                raise ValueError("unsupported special staged file: " + str(full))
            entries.append(record)
    return entries

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime-stage", type=Path, required=True)
    parser.add_argument("--plugin-stage", type=Path, required=True)
    parser.add_argument("--output", type=Path, help="write report; stdout by default")
    args = parser.parse_args(argv)
    try:
        records = inspect(args.runtime_stage, "runtime") + inspect(args.plugin_stage, "plugin")
        seen = set()
        for entry in records:
            if entry["path"] in seen:
                raise ValueError("collision across stages: " + entry["path"])
            seen.add(entry["path"])
        records.sort(key=lambda e: e["path"])
        if not records:
            raise ValueError("empty staging inventory")
        if not any(x["package"] == "vdr-plugin-suitebridge" for x in records):
            raise ValueError("SuiteBridge plugin missing from staging inventory")
        if not any(x["path"] == "/usr/sbin/vdr-suite-daemon" and x["type"] == "file" for x in records):
            raise ValueError("daemon missing from staging inventory")
        result = {"schema": 2, "entries": records, "count": len(records),
                  "note": "Inventory only; not a package build or proof of runtime compatibility"}
        document = json.dumps(result, indent=2, sort_keys=True) + "\n"
        if args.output:
            args.output.write_text(document, encoding="utf-8")
        else:
            sys.stdout.write(document)
    except (OSError, ValueError, RuntimeError) as exc:
        parser.exit(1, "packaging inventory ERROR: " + str(exc) + "\n")
    return 0

if __name__ == "__main__":
    sys.exit(main())
