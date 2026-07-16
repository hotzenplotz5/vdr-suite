#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

required_files = (
    ROOT / "Makefile",
    ROOT / "README.md",
    ROOT / "suitebridge.h",
    ROOT / "suitebridge.cpp",
)

errors = []

for path in required_files:
    if not path.is_file():
        errors.append(f"missing file: {path.relative_to(ROOT)}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
header = (ROOT / "suitebridge.h").read_text(encoding="utf-8")
source = (ROOT / "suitebridge.cpp").read_text(encoding="utf-8")
combined = header + "\n" + source

required_makefile_content = (
    "PLUGIN = suitebridge",
    "SOFILE = libvdr-$(PLUGIN).so",
    "APIVERSION = $(call PKGCFG,apiversion)",
    "check-version:",
    'test "$(VERSION)" = "0.1.0"',
)

for fragment in required_makefile_content:
    if fragment not in makefile:
        errors.append(f"missing Makefile contract: {fragment}")

required_source_content = (
    'static const char *VERSION = "0.1.0";',
    "bool cPluginSuiteBridge::Initialize(void)",
    "bool cPluginSuiteBridge::Start(void)",
    "void cPluginSuiteBridge::Stop(void)",
    "return nullptr;",
    "VDRPLUGINCREATOR(cPluginSuiteBridge);",
)

for fragment in required_source_content:
    if fragment not in source:
        errors.append(f"missing source contract: {fragment}")

forbidden_content = (
    "#include <thread>",
    "std::thread",
    "cThread",
    "socket(",
    "bind(",
    "listen(",
    "connect(",
    "system(",
    "fork(",
    "execv(",
)

for fragment in forbidden_content:
    if fragment in combined:
        errors.append(f"forbidden foundation functionality: {fragment}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("suitebridge foundation contract ok")
