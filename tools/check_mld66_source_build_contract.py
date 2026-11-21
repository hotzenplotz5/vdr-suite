#!/usr/bin/env python3
"""Read-only MLD.1A source-contract smoke; NOT a Yocto build or runtime test."""

from pathlib import Path
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent


def require(condition, description):
    if not condition:
        raise AssertionError(description)


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


def main():
    common = read("mk/common.mk")
    install = read("mk/install.mk")
    plugin = read("vdr-plugin-suite-bridge/Makefile")
    daemon = read("mk/daemon-sources.mk")

    require("CXX ?= g++" in common, "daemon must honor a caller-supplied CXX")
    require("PKG_CONFIG ?= pkg-config" in common, "pkg-config executable must be configurable")
    require("$(CXX) $(CPPFLAGS)" in common, "daemon compile must forward CPPFLAGS")
    require("CXXFLAGS += -std=c++17" in common, "daemon compile must append its C++ flags")
    require("LDFLAGS += $(shell $(PKG_CONFIG) --libs sqlite3)" in common,
            "daemon link must preserve supplied flags and link sqlite3")
    require("-lcrypt -lcrypto" in daemon, "daemon crypt/crypto link dependencies missing")
    require("build-runtime-binaries:" in install and "install-runtime:" in install,
            "missing canonical runtime build/install targets")
    require("$(DESTDIR)$(SBINDIR)/vdr-suite-daemon" in install,
            "daemon install must be DESTDIR-staged")
    require("$(DESTDIR)$(DATADIR)/web/frontend/index.html" in install,
            "web must be installed from same canonical stage")
    require("APIVERSION = $(call PKGCFG,apiversion)" in plugin,
            "SuiteBridge API version must derive from target vdr.pc")
    require("$(SOFILE).$(APIVERSION)" in plugin and "$(DESTDIR)$(LIBDIR)" in plugin,
            "SuiteBridge filename/location must derive from target vdr.pc")

    # Dry-run the *actual* daemon test target. The fake cross compiler cannot
    # execute, and GNU Make -n does not compile or create runtime files.
    env = os.environ.copy()
    env.update({
        "CXX": "mld-inventory-fake-cross-cxx",
        "CPPFLAGS": "-DMLD_CPPFLAGS_PROPAGATED=1",
        "CXXFLAGS": "-O2 -DMLD_CXXFLAGS_PROPAGATED=1",
        "LDFLAGS": "-Wl,--as-needed",
    })
    completed = subprocess.run(
        ["make", "-n", "-f", "mk/common.mk", "test-runtime-config"],
        cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, check=False, timeout=30
    )
    require(completed.returncode == 0,
            "daemon compile dry-run failed: " + completed.stderr.strip())
    for marker in (
        "mld-inventory-fake-cross-cxx", "-DMLD_CPPFLAGS_PROPAGATED=1",
        "-DMLD_CXXFLAGS_PROPAGATED=1", "-std=c++17", "-O2"
    ):
        require(marker in completed.stdout, "lost caller/toolchain flag: " + marker)

    shellcheck = subprocess.run(
        ["sh", "-n", "tools/mld66-readonly-inventory.sh"],
        cwd=ROOT, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, check=False, timeout=10
    )
    require(shellcheck.returncode == 0,
            "MLD read-only probe syntax invalid: " + shellcheck.stderr.strip())
    print("MLD_SOURCE_BUILD_PREFLIGHT=PASS")
    print("MLD_TARGET_BUILD_OR_RUNTIME_TEST=NOT_RUN")


if __name__ == "__main__":
    try:
        main()
    except (AssertionError, OSError, subprocess.TimeoutExpired) as exc:
        print("MLD_SOURCE_BUILD_PREFLIGHT=FAIL: " + str(exc), file=sys.stderr)
        sys.exit(1)
