#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
RUNTIME = ROOT / "core/daemon/src/DaemonRuntime.cpp"
TELETEXT_HEADER = ROOT / "core/daemon/include/DaemonTeletextRuntime.h"
TELETEXT_SOURCE = ROOT / "core/daemon/src/DaemonTeletextRuntime.cpp"
SOURCES = ROOT / "mk/daemon-sources.mk"

errors: list[str] = []

for path in (RUNTIME, TELETEXT_HEADER, TELETEXT_SOURCE, SOURCES):
    if not path.is_file():
        errors.append(f"missing Phase 67 Teletext daemon wiring file: {path.relative_to(ROOT)}")

runtime = RUNTIME.read_text(encoding="utf-8") if RUNTIME.is_file() else ""
teletext = TELETEXT_SOURCE.read_text(encoding="utf-8") if TELETEXT_SOURCE.is_file() else ""
sources = SOURCES.read_text(encoding="utf-8") if SOURCES.is_file() else ""

for fragment in (
    '#include "DaemonTeletextRuntime.h"',
    'configureDaemonTeletextRuntime(',
    '*backendRegistryService_',
    '*vdrSnapshotReadService_',
    '*embeddedBackendLifecycleService_',
    'backendRuntimeContexts_',
    'resetDaemonTeletextRuntime();',
):
    if fragment not in runtime:
        errors.append(f"DaemonRuntime missing Teletext wiring fragment: {fragment}")

reset_index = runtime.find('resetDaemonTeletextRuntime();')
lifecycle_reset_index = runtime.find('backendAgentLifecycleService_.reset();')
contexts_clear_index = runtime.find('backendRuntimeContexts_.clear();')
if reset_index < 0 or lifecycle_reset_index < 0 or reset_index > lifecycle_reset_index:
    errors.append("Teletext runtime must reset before lifecycle teardown")
if reset_index < 0 or contexts_clear_index < 0 or reset_index > contexts_clear_index:
    errors.append("Teletext runtime must reset before backend runtime contexts are cleared")

for fragment in (
    'std::unique_ptr<EmbeddedBackendTeletextAuthority> teletextBackendAuthority;',
    'std::unique_ptr<TeletextControlPlaneReadService> teletextControlPlaneReadService;',
    'std::make_unique<EmbeddedBackendTeletextAuthority>',
    'std::make_unique<TeletextControlPlaneReadService>',
    'context->ensureTeletextResolver()',
    'teletextControlPlaneReadService.reset();',
    'teletextBackendAuthority.reset();',
):
    if fragment not in teletext:
        errors.append(f"DaemonTeletextRuntime missing ownership fragment: {fragment}")

service_reset_index = teletext.find('teletextControlPlaneReadService.reset();')
authority_reset_index = teletext.find('teletextBackendAuthority.reset();')
if service_reset_index < 0 or authority_reset_index < 0 or service_reset_index > authority_reset_index:
    errors.append("Teletext read service must reset before its lifecycle authority")

for source in (
    'core/daemon/src/EmbeddedBackendLifecycle.cpp',
    'core/daemon/src/EmbeddedBackendTeletextAuthority.cpp',
    'core/daemon/src/TeletextControlPlaneReadService.cpp',
    'core/daemon/src/DaemonTeletextRuntime.cpp',
):
    if source not in sources:
        errors.append(f"daemon source manifest missing Teletext runtime source: {source}")

for forbidden in (
    'TTXC 1',
    'TTXP 1',
    '/var/cache/vdr/vtx',
    'OsdTeletext::',
):
    if forbidden in runtime or forbidden in teletext:
        errors.append(f"daemon Teletext wiring must not own provider/wire detail: {forbidden}")

if errors:
    for error in errors:
        print(error, file=sys.stderr)
    raise SystemExit(1)

print("Phase 67 Teletext daemon runtime wiring contract ok")
