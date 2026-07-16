#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

required_files = (
    ROOT / "Makefile",
    ROOT / "README.md",
    ROOT / "suitebridge.h",
    ROOT / "suitebridge.cpp",
    ROOT / "suitebridge_lifecycle.h",
    ROOT / "suitebridge_lifecycle.cpp",
    ROOT / "tests/test_suitebridge_lifecycle.cpp",
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
lifecycle_header = (ROOT / "suitebridge_lifecycle.h").read_text(encoding="utf-8")
lifecycle_source = (ROOT / "suitebridge_lifecycle.cpp").read_text(encoding="utf-8")
combined = "\n".join(
    (header, source, lifecycle_header, lifecycle_source)
)

required_makefile_content = (
    "PLUGIN = suitebridge",
    "SOFILE = libvdr-$(PLUGIN).so",
    "APIVERSION = $(call PKGCFG,apiversion)",
    "OBJS = $(PLUGIN).o suitebridge_lifecycle.o",
    "test-lifecycle:",
    'test "$(VERSION)" = "0.2.0"',
)

for fragment in required_makefile_content:
    if fragment not in makefile:
        errors.append(f"missing Makefile contract: {fragment}")

required_source_content = (
    'static const char *VERSION = "0.2.0";',
    "bool cPluginSuiteBridge::Initialize(void)",
    "bool cPluginSuiteBridge::Start(void)",
    "void cPluginSuiteBridge::Stop(void)",
    "lifecycle_.Initialize()",
    "lifecycle_.Start()",
    "lifecycle_.Stop()",
    "return nullptr;",
    "VDRPLUGINCREATOR(cPluginSuiteBridge);",
)

for fragment in required_source_content:
    if fragment not in source:
        errors.append(f"missing source contract: {fragment}")

required_lifecycle_content = (
    "enum class SuiteBridgeLifecycleState",
    "SuiteBridgeLifecycleState::Constructed",
    "SuiteBridgeLifecycleState::Initialized",
    "SuiteBridgeLifecycleState::Started",
    "SuiteBridgeLifecycleState::Stopped",
    "bool SuiteBridgeLifecycle::Initialize() noexcept",
    "bool SuiteBridgeLifecycle::Start() noexcept",
    "void SuiteBridgeLifecycle::Stop() noexcept",
)

for fragment in required_lifecycle_content:
    if fragment not in combined:
        errors.append(f"missing lifecycle contract: {fragment}")

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

print("suitebridge lifecycle contract ok")
