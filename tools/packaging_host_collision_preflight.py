#!/usr/bin/env python3
"""Read-only P6/P8 review of manual-install and nginx collisions before dpkg.

Does not start services, copy data, change permissions, or approve installation.
"""
import argparse
import json
import os
from pathlib import Path
import re
import stat
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
PACKAGE_INSTALL_FILES = {
    "vdr-suite-daemon": "debian/vdr-suite-daemon.install",
    "vdr-suite-backend-agent": "debian/vdr-suite-backend-agent.install",
    "vdr-plugin-suitebridge": "debian/vdr-plugin-suitebridge.install",
}
KNOWN_TARGETS = {
    "etc/nginx/snippets/vdr-suite.conf",
    "etc/default/vdr-suite-daemon",
    "etc/vdr-suite/backend-agent.conf",
    "usr/sbin/vdr-suite-daemon",
    "usr/sbin/vdr-suite-backend-agent",
    "lib/systemd/system/vdr-suite-daemon.service",
    "lib/systemd/system/vdr-suite-backend-agent.service",
    "usr/lib/tmpfiles.d/vdr-suite-live.conf",
}
MANUAL_PATHS = (
    "usr/local/sbin/vdr-suite-daemon",
    "usr/local/sbin/vdr-suite-backend-agent",
    "usr/local/bin/vdr-suite-dashboard",
    "usr/local/share/vdr-suite",
    "usr/share/vdr-suite/web/frontend",
    "etc/systemd/system/vdr-suite-daemon.service",
    "etc/systemd/system/vdr-suite-backend-agent.service",
    "var/lib/vdr-suite",
)
SITE_ROOTS = ("etc/nginx/nginx.conf", "etc/nginx/sites-enabled",
              "etc/nginx/conf.d")
# nginx directives can appear immediately after a brace or semicolon,
# not only at the start of a physical line.
LOCATION = re.compile(r"(?m)(?:^|[;{}])\s*location\s+(?:=\s*|\^~\s*|~\*?\s*)?/vdr-suite(?:[/{\s]|$)")
REFERENCE = re.compile(r"(?m)(?:^|[;{}])\s*include\s+[^;]*vdr-suite[^;]*;")
MAX_CONFIGS = 256
MAX_CONFIG_SIZE = 1024 * 1024


def package_targets(source=ROOT):
    """Use tracked Debian manifests, not a guessed replacement file list."""
    targets = {}
    for package, filename in PACKAGE_INSTALL_FILES.items():
        for raw in (source / filename).read_text(encoding="utf-8").splitlines():
            entry = raw.strip()
            if not entry or entry.startswith("#"):
                continue
            if entry.startswith("/") or ".." in Path(entry).parts:
                raise ValueError("unsafe Debian payload path in " + filename)
            if entry.endswith("/") or "*" in entry:
                continue
            targets[entry] = package
    if not KNOWN_TARGETS.issubset(targets):
        raise ValueError("critical Debian install paths changed: manual review required")
    return targets


def dpkg_owner(path):
    """Return package name, None if unowned; do not guess on query errors."""
    try:
        query = subprocess.run(
            ["dpkg-query", "-S", "--", str(path)], capture_output=True,
            text=True, timeout=5, check=False)
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise ValueError("dpkg ownership query failed") from exc
    if query.returncode == 1:
        return None
    if query.returncode != 0:
        raise ValueError("dpkg ownership query failed")
    owners = set()
    for line in query.stdout.splitlines():
        if ": " not in line:
            raise ValueError("ambiguous dpkg ownership result")
        # dpkg-query -S may list several owners for shared paths.
        parts = line.split(": ", 1)[0].split(", ")
        owners.update(item.split(":", 1)[0] for item in parts)
    if len(owners) != 1 or not next(iter(owners)):
        raise ValueError("ambiguous dpkg ownership result")
    return next(iter(owners))


def exists_no_follow(path):
    return os.path.lexists(path)


def inspect(root=Path("/"), ownership=None, source=ROOT):
    """Inspect a fake/private root just like '/', without ever writing to it."""
    root = Path(root).resolve(strict=True)
    if not root.is_dir():
        raise ValueError("inspection root must be a directory")
    if ownership is None:
        ownership = dpkg_owner if root == Path("/") else lambda _: None
    targets = package_targets(source)
    findings = []

    def report(code, rel, severity):
        findings.append({"code": code, "path": "/" + rel,
                         "severity": severity})

    for rel, expected in sorted(targets.items()):
        path = root / rel
        if not exists_no_follow(path):
            continue
        if path.is_symlink() or not path.is_file():
            report("TARGET_NOT_REGULAR_FILE", rel, "BLOCK")
            continue
        try:
            owner = ownership(path)
        except ValueError:
            report("OWNERSHIP_UNVERIFIED", rel, "BLOCK")
            continue
        if owner is None:
            report("UNOWNED_PACKAGE_TARGET", rel, "BLOCK")
        elif owner != expected:
            report("OTHER_PACKAGE_OWNS_TARGET", rel, "BLOCK")
        else:
            report("EXISTING_PACKAGE_TARGET_REVIEW", rel, "REVIEW")

    # Legacy/manual roots may not overlap dpkg paths but can still be
    # running, selected by PATH, or retain secrets/older runtime state.
    for rel in MANUAL_PATHS:
        if exists_no_follow(root / rel):
            report("EXISTING_MANUAL_OR_STATE_PATH", rel, "REVIEW")

    # The plugin path depends on the target distro's VDR API version.
    plugin_root = root / "usr/lib/vdr/plugins"
    if plugin_root.is_dir() and not plugin_root.is_symlink():
        for p in sorted(plugin_root.iterdir()):
            if p.name.startswith("libvdr-suitebridge.so."):
                report("INSTALLED_VDR_PLUGIN_ABI_REVIEW",
                       "usr/lib/vdr/plugins/" + p.name, "REVIEW")

    # A complete nginx parser is out of scope. Look for obvious active
    # route conflicts, and require operator review for unparsed settings.
    nginx_root = root / "etc/nginx"
    candidate_paths = []
    for rel in SITE_ROOTS:
        p = root / rel
        if p.is_file() or p.is_symlink():
            candidate_paths.append(p)
        elif p.is_dir():
            candidate_paths.extend(sorted(p.iterdir()))
    if nginx_root.exists():
        report("NGINX_ACTIVE_CONFIG_REVIEW", "etc/nginx", "REVIEW")
    if len(candidate_paths) > MAX_CONFIGS:
        report("NGINX_SCAN_LIMIT", "etc/nginx", "BLOCK")
    for path in candidate_paths[:MAX_CONFIGS]:
        rel = path.relative_to(root).as_posix()
        try:
            # Sites-enabled symlinks are common. Follow only destinations
            # within this inspected nginx root; never escape test fixtures.
            resolved = path.resolve(strict=True)
            if not resolved.is_relative_to(nginx_root.resolve(strict=True)):
                report("NGINX_INCLUDE_OUTSIDE_ROOT", rel, "REVIEW")
                continue
            info = resolved.stat()
            if not stat.S_ISREG(info.st_mode) or info.st_size > MAX_CONFIG_SIZE:
                report("NGINX_UNREADABLE_OR_LARGE", rel, "REVIEW")
                continue
            body = resolved.read_text(encoding="utf-8")
        except (OSError, UnicodeError, RuntimeError):
            report("NGINX_UNREADABLE_OR_LARGE", rel, "REVIEW")
            continue
        if LOCATION.search(body) or REFERENCE.search(body):
            report("NGINX_EXISTING_VDR_SUITE_ROUTE", rel, "REVIEW")
    # No reverse proxy config should be activated or reloaded by this tool.
    severity = {item["severity"] for item in findings}
    verdict = ("BLOCKED" if "BLOCK" in severity else
               "REVIEW_REQUIRED" if findings else "NO_KNOWN_CONFLICT")
    return {"schema": 1, "verdict": verdict, "install_approved": False,
            "read_only": True, "checks": sorted(findings,
            key=lambda x: (x["path"], x["code"])),
            "note": "Non-exhaustive host scan; nginx includes, systemd "
                    "runtime, source version and SQLite upgrade need review"}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path("/"),
                        help="filesystem root (default: live host, READ-ONLY)")
    args = parser.parse_args(argv)
    try:
        result = inspect(args.root)
    except (OSError, ValueError) as exc:
        parser.exit(2, "HOST_COLLISION_PREFLIGHT=BLOCKED: " + str(exc) + "\n")
    print(json.dumps(result, sort_keys=True, indent=2))
    return 2 if result["verdict"] == "BLOCKED" else 0


if __name__ == "__main__":
    sys.exit(main())
