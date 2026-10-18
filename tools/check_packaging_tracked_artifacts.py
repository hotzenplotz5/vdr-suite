#!/usr/bin/env python3
"""Fail when generated Debian/Plugin build outputs are already tracked by git."""
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
GENERATED = (
    re.compile(r"^debian/(?:tmp/|\.debhelper/|files$|debhelper-build-stamp$)"),
    re.compile(r"^debian/(?:vdr-suite-daemon|vdr-suite-backend-agent|vdr-plugin-suitebridge)/"),
    re.compile(r"^debian/[^/]+\.(?:substvars|debhelper(?:\.log)?|postinst\.debhelper|postrm\.debhelper|prerm\.debhelper|preinst\.debhelper)$"),
    re.compile(r"^vdr-plugin-suite-bridge/[^/]+\.(?:o|so(?:\.[^/]*)?|tgz|d|gcda|gcno)$"),
    re.compile(r"^(?:\.build|\.pc)/"),
    re.compile(r"[^/]+\.(?:deb|udeb|ddeb|dsc|changes|buildinfo|upload|orig\.tar\.[^/]+|debian\.tar\.[^/]+)$"),
)

def is_generated(path: str) -> bool:
    return any(p.search(path) for p in GENERATED)

def main():
    try:
        run = subprocess.run(["git", "-C", str(ROOT), "ls-files", "-z"],
                             check=True, capture_output=True)
    except (OSError, subprocess.CalledProcessError) as exc:
        sys.exit("TRACKED_ARTIFACT_AUDIT_ERROR: " + str(exc))
    paths = [p.decode("utf-8") for p in run.stdout.split(b"\x00") if p]
    suspect = sorted(x for x in paths if is_generated(x))
    if suspect:
        for x in suspect:
            print("TRACKED_GENERATED_ARTIFACT=" + x, file=sys.stderr)
        sys.exit(1)
    print("TRACKED_ARTIFACT_AUDIT=PASS")

if __name__ == "__main__":
    main()
