#!/usr/bin/env python3
"""Build and round-trip an unsigned source-only Debian package in an isolated tree.

This never installs packages, signs anything, contacts Launchpad, or touches services.
"""
import argparse
import os
from pathlib import Path
import pwd
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ALLOWED_ROOT = Path("/home/yavdr/vdr-suite-packaging-test")
BASE = Path("/home/yavdr")
BRANCH = "work/packaging-debian-ubuntu-install"
BUDGET = 2 * 1024**3


def stop(message):
    raise RuntimeError(message)


def command(args, cwd=ROOT, **kwargs):
    return subprocess.run(args, cwd=cwd, check=True, **kwargs)


def output(args, cwd=ROOT):
    return command(args, cwd=cwd, capture_output=True, text=True).stdout.strip()


def preflight(expected):
    if pwd.getpwuid(os.geteuid()).pw_name != "yavdr" or os.geteuid() == 0:
        stop("run as non-root yavdr")
    if ROOT.resolve() != ALLOWED_ROOT:
        stop("wrong checkout path; Codec checkout must not be touched")
    if output(["git", "rev-parse", "--show-toplevel"]) != str(ROOT):
        stop("not at the correct repository root")
    if output(["git", "branch", "--show-current"]) != BRANCH:
        stop("not on the packaging branch")
    if output(["git", "rev-parse", "HEAD"]) != expected:
        stop("unexpected local commit")
    if output(["git", "status", "--porcelain"]):
        stop("checkout is not clean")
    remote = output(["git", "ls-remote", "--exit-code", "origin",
                     "refs/heads/" + BRANCH]).split()[0]
    if remote != expected:
        stop("remote head differs from the verified candidate")
    print("CANDIDATE_IDENTITY=PASS " + expected, flush=True)

    needed = ("git", "tar", "xz", "dpkg-buildpackage", "dpkg-source",
              "dpkg-parsechangelog", "dh", "dh_vdrplugin_depends")
    missing = [item for item in needed if shutil.which(item) is None]
    if missing:
        stop("missing tools: " + ", ".join(missing))
    command(["python3", "tools/check_packaging_version.py"])
    command(["python3", "tools/check_packaging_tracked_artifacts.py"])
    distro = output(["dpkg-parsechangelog", "-l", "debian/changelog",
                     "-S", "Distribution"])
    if distro != "unstable":
        stop("only an offline source smoke test is allowed: expected changelog suite unstable")

    version = (ROOT / "VERSION").read_text(encoding="ascii").strip()
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", version):
        stop("unsupported upstream version string")
    epoch = output(["git", "show", "-s", "--format=%ct", expected])
    if not epoch.isdigit():
        stop("source commit timestamp missing")

    stat = os.statvfs(BASE)
    total = stat.f_blocks * stat.f_frsize
    free = stat.f_bavail * stat.f_frsize
    reserve = max(total * 15 // 100, 4 * 1024**3)
    print("RESOURCE_TOTAL={} RESOURCE_FREE={} RESOURCE_RESERVE={} BUILD_BUDGET={}".format(
        total, free, reserve, BUDGET), flush=True)
    if free < reserve + BUDGET:
        stop("insufficient reserved storage headroom")
    print("RESOURCE_PREFLIGHT=PASS", flush=True)
    return version, epoch, reserve


def build_source(expected, version, epoch, reserve):
    area = Path(tempfile.mkdtemp(prefix="vdr-suite-sourcepkg-", dir=BASE))
    print("SOURCE_BUILD_AREA=" + str(area), flush=True)
    project = "vdr-suite-" + version
    source = area / project
    source.mkdir()
    try:
        archive = area / "tracked-source.tar"
        command(["git", "archive", "--format=tar", "--output=" + str(archive), expected])
        command(["tar", "-xf", str(archive), "-C", str(source)])
        if not (source / "debian/rules").is_file():
            stop("source snapshot is missing debian/rules")
        upstream = area / ("vdr-suite_" + version + ".orig.tar.xz")
        command(["tar", "--sort=name", "--mtime=@" + epoch,
                 "--owner=0", "--group=0", "--numeric-owner",
                 "--exclude=" + project + "/debian", "-C", str(area),
                 "-cJf", str(upstream), project])
        if upstream.stat().st_size == 0:
            stop("upstream tarball is empty")
        print("PRISTINE_UPSTREAM_TARBALL=PASS", flush=True)
        log = area / "source-build.log"
        env = os.environ.copy()
        env["SOURCE_DATE_EPOCH"] = epoch
        with log.open("wb") as handle:
            outcome = subprocess.run(
                ["dpkg-buildpackage", "-S", "-sa", "-us", "-uc", "-d"],
                cwd=source, env=env, stdout=handle, stderr=subprocess.STDOUT)
        if outcome.returncode != 0:
            print("SOURCE_BUILD_LOG=" + str(log), flush=True)
            stop("source-only dpkg-buildpackage failed; inspect saved log")
        changes = sorted(area.glob("vdr-suite_*_source.changes"))
        dsc = sorted(area.glob("vdr-suite_*.dsc"))
        if len(changes) != 1 or len(dsc) != 1:
            stop("expected exactly one source changes and DSC")
        extracted = area / "unpacked"
        command(["dpkg-source", "-x", str(dsc[0]), str(extracted)])
        if (extracted / "VERSION").read_bytes() != (ROOT / "VERSION").read_bytes():
            stop("source roundtrip changed project version")
        print("SOURCE_ROUNDTRIP=PASS", flush=True)
        stat = os.statvfs(BASE)
        if stat.f_bavail * stat.f_frsize < reserve:
            stop("post-build storage reserve violated")
        print("RESOURCE_POSTCHECK=PASS", flush=True)
        print("SOURCE_CHANGESET=" + str(changes[0]), flush=True)
        print("SOURCE_ONLY_PACKAGE=PASS", flush=True)
        print("UPLOAD=NOT_PERFORMED (unsigned, unstable; PPA release gates remain)", flush=True)
    except Exception:
        print("SOURCE_BUILD_FAILED_AREA=" + str(area), file=sys.stderr, flush=True)
        raise


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("expected_sha", help="exact 40-character remote/checkout commit")
    parser.add_argument("--preflight-only", action="store_true")
    args = parser.parse_args(argv)
    if not re.fullmatch(r"[0-9a-f]{40}", args.expected_sha):
        parser.error("expected SHA must contain exactly 40 lowercase hex characters")
    try:
        version, epoch, reserve = preflight(args.expected_sha)
        if not args.preflight_only:
            build_source(args.expected_sha, version, epoch, reserve)
    except (OSError, RuntimeError, subprocess.CalledProcessError) as exc:
        print("SOURCE_PACKAGE_STOP=" + str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
