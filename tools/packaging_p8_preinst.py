#!/usr/bin/env python3
"""Generate minimal dpkg pre-unpack collision guards from the canonical .install manifests.

The generated POSIX-shell preinst scripts use only dpkg-query and shell builtins.
They do not approve an installation or replace the complete read-only P6 scan.
"""
from pathlib import Path
import os
import argparse
import re
import shlex
import sys

ROOT = Path(__file__).resolve().parent.parent
MANIFESTS = {
    "vdr-suite-daemon": "debian/vdr-suite-daemon.install",
    "vdr-suite-backend-agent": "debian/vdr-suite-backend-agent.install",
    "vdr-plugin-suitebridge": "debian/vdr-plugin-suitebridge.install",
}
MANUAL_PATHS = {
    "vdr-suite-daemon": (
        "usr/local/sbin/vdr-suite-daemon",
        "usr/local/bin/vdr-suite-dashboard",
        "etc/systemd/system/vdr-suite-daemon.service",
    ),
    "vdr-suite-backend-agent": (
        "usr/local/sbin/vdr-suite-backend-agent",
        "etc/systemd/system/vdr-suite-backend-agent.service",
    ),
    "vdr-plugin-suitebridge": (
        "usr/local/lib/vdr/plugins/libvdr-suitebridge.so.*",
    ),
}
# System state from an earlier manual installation must never be treated as
# new empty package state. This check is only for a previously uninstalled
# binary package; upgrade compatibility remains an independent P7 gate.
STATE_ROOT = "var/lib/vdr-suite"
SAFE_PATH = re.compile(r"^[a-zA-Z0-9_./*+\-]+$")


def manifest_entries(repo, package):
    path = Path(repo) / MANIFESTS[package]
    result = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        entry = raw.strip()
        if not entry or entry.startswith("#"):
            continue
        parts = Path(entry).parts
        if (entry.startswith("/") or ".." in parts or
                not SAFE_PATH.fullmatch(entry) or
                ("*" in entry and (entry.count("*") != 1 or
                                   not entry.endswith(".*"))) or
                "?" in entry or "[" in entry):
            raise ValueError("unsupported package manifest entry: " + entry)
        result.append(entry.rstrip("/"))
    if not result:
        raise ValueError("empty package manifest: " + package)
    return sorted(set(result))


def render(package, repo=ROOT, test_root=""):
    if package not in MANIFESTS:
        raise ValueError("unknown package " + package)
    if test_root and (not Path(test_root).is_absolute() or
                      test_root.rstrip("/") == "/"):
        raise ValueError("test root must be a private absolute directory")
    prefix = test_root.rstrip("/")

    def path(entry):
        return prefix + "/" + entry

    out = [
        "#!/bin/sh",
        "# GENERATED from tools/packaging_p8_preinst.py and debian/*.install.",
        "# Do not edit manually; checks run before dpkg unpacks package files.",
        "set -eu",
        "case \"${1:-}\" in",
        "  install|upgrade) ;;",
        "  *) exit 0 ;; # recovery/abort operations must remain possible",
        "esac",
        "package=" + shlex.quote(package),
        "deny() {",
        "  printf 'P8_PREUNPACK=BLOCKED package=%s reason=%s path=%s\\n' \"$package\" \"$1\" \"$2\" >&2",
        "  exit 1",
        "}",
        "check_target() {",
        "  target=$1",
        "  if [ -L \"$target\" ]; then",
        "    deny SYMLINK_TARGET \"$target\"",
        "  fi",
        "  if [ ! -e \"$target\" ]; then",
        "    return 0",
        "  fi",
        "  if [ ! -f \"$target\" ] && [ ! -d \"$target\" ]; then",
        "    deny SPECIAL_TARGET \"$target\"",
        "  fi",
        "  owner=$(dpkg-query -S -- \"$target\" 2>/dev/null) || deny OWNERSHIP_UNKNOWN \"$target\"",
        "  [ \"$owner\" = \"$package: $target\" ] || deny FOREIGN_OR_AMBIGUOUS_OWNER \"$target\"",
        "}",
        "check_manual() {",
        "  target=$1",
        "  if [ -e \"$target\" ] || [ -L \"$target\" ]; then",
        "    deny MANUAL_INSTALL_PRESENT \"$target\"",
        "  fi",
        "}",
    ]
    package_entries = manifest_entries(repo, package)
    # debhelper may compress manpages after dh_install but before dh_installdeb.
    # Check both names; never miss a pre-existing compressed manual page.
    package_entries += [e + ".gz" for e in package_entries
                        if e.startswith("usr/share/man/") and not e.endswith(".gz")]
    for entry in package_entries:
        target = path(entry)
        if "*" in entry:
            # All variants are checked; a current VDR ABI cannot hide an older one.
            out += ["for found in " + target + "; do",
                    "  if [ -e \"$found\" ] || [ -L \"$found\" ]; then",
                    "    check_target \"$found\"",
                    "  fi", "done"]
        else:
            out.append("check_target " + shlex.quote(target))
    for entry in MANUAL_PATHS[package]:
        target = path(entry)
        if "*" in entry:
            out += ["for found in " + target + "; do",
                    "  check_manual \"$found\"", "done"]
        else:
            out.append("check_manual " + shlex.quote(target))
    if package != "vdr-plugin-suitebridge":
        out += [
            "installed=$(dpkg-query -W -f='${Status}' \"$package\" 2>/dev/null) || installed=unknown",
            "if [ \"$installed\" != 'install ok installed' ]; then",
            "  check_manual " + shlex.quote(path(STATE_ROOT)),
            "fi",
        ]
    out += ["exit 0", ""]
    return "\n".join(out)


def stage_preinst(stage_dir, repo=ROOT):
    """Write scripts only into existing debhelper DEBIAN metadata directories.

    Called after dh_installdeb; never installs to / or touches host services.
    """
    stage_dir = Path(stage_dir)
    if not stage_dir.is_dir() or stage_dir.is_symlink():
        raise ValueError("Debian package stage root must be a real directory")
    generated = []
    for package in MANIFESTS:
        package_root = stage_dir / package
        if package_root.is_symlink():
            raise ValueError("unsafe Debian package root: " + package)
        if not package_root.exists():
            continue  # plugin-only profile does not create daemon/agent roots
        if not package_root.is_dir():
            raise ValueError("unsafe Debian package root: " + package)
        control = package_root / "DEBIAN"
        if not control.exists():
            raise ValueError("missing debhelper control directory: " + package)
        if control.is_symlink() or not control.is_dir():
            raise ValueError("unsafe Debian package control directory: " + package)
        script = control / "preinst"
        if script.is_symlink():
            raise ValueError("unsafe preinst symlink: " + package)
        guard = render(package, repo)
        if script.exists():
            # dh_installdeb can already create preinst via dh_* autosnippets.
            # Prepend the guard, but KEEP every existing maintainer hook and
            # its abort/recovery flow. The guard runs only before unpack.
            existing = script.read_text(encoding="utf-8")
            header = "#!/bin/sh\nset -e\n"
            if not existing.startswith(header) or "# BEGIN P8 PREUNPACK" in existing:
                raise ValueError("unknown or already guarded preinst: " + package)
            guard_body = guard.split("\n", 1)[1]
            embedded = (
                "# BEGIN P8 PREUNPACK - generated from canonical .install manifests\n"
                "p8_preunpack_guard() (\n" + guard_body +
                ")\ncase \"${1:-}\" in\n"
                "  install|upgrade) p8_preunpack_guard \"$@\" ;;\n"
                "esac\n# END P8 PREUNPACK\n"
            )
            script.write_text(header + embedded + existing[len(header):], encoding="utf-8")
        else:
            fd = os.open(script, os.O_WRONLY | os.O_CREAT | os.O_EXCL |
                         getattr(os, "O_NOFOLLOW", 0), 0o755)
            with os.fdopen(fd, "w", encoding="utf-8") as out:
                out.write(guard)
        os.chmod(script, 0o755)
        generated.append(package)
    if not generated:
        raise ValueError("no Debian package control directories to protect")
    return generated


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", action="store_true", required=True,
                        help="install preinst guards into existing debhelper package metadata only")
    parser.add_argument("--stage-dir", type=Path, default=ROOT / "debian",
                        help="debhelper package staging root")
    args = parser.parse_args(argv)
    try:
        packages = stage_preinst(args.stage_dir)
    except (OSError, ValueError) as exc:
        print("P8_PREINST_STAGE=BLOCKED " + str(exc), file=sys.stderr)
        return 1
    print("P8_PREINST_STAGE=PASS packages=" + ",".join(packages))
    return 0


if __name__ == "__main__":
    sys.exit(main())
