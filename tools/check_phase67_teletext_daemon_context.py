#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTEXT = ROOT / "core/daemon/include/BackendRuntimeContext.h"
SOURCES = ROOT / "mk/vdr-sources.mk"
DOMAIN = ROOT / "core/vdr/include/TeletextDomain.h"
RESOLVER = ROOT / "core/vdr/include/SuiteBridgeTeletextResolver.h"

errors: list[str] = []

for path in (CONTEXT, SOURCES, DOMAIN, RESOLVER):
    if not path.is_file():
        errors.append(f"missing Phase 67 Teletext daemon file: {path.relative_to(ROOT)}")

context = CONTEXT.read_text(encoding="utf-8") if CONTEXT.is_file() else ""
sources = SOURCES.read_text(encoding="utf-8") if SOURCES.is_file() else ""

for fragment in (
    '#include "SuiteBridgeTeletextResolver.h"',
    'std::unique_ptr<SuiteBridgeTeletextResolver> teletextResolver;',
    'SuiteBridgeTeletextResolver* ensureTeletextResolver()',
    'if (!suiteBridgeTransport)',
    'std::make_unique<SuiteBridgeTeletextResolver>',
    '*suiteBridgeTransport',
):
    if fragment not in context:
        errors.append(f"BackendRuntimeContext missing Teletext ownership fragment: {fragment}")

for forbidden in (
    'backendGeneration = 1',
    'backendGeneration{1}',
    'TTXC 1',
    'TTXP 1',
    '/var/cache/vdr/vtx',
    'OsdTeletext::',
):
    if forbidden in context:
        errors.append(f"daemon context must not own provider/wire detail: {forbidden}")

if 'BROADCAST_TELETEXT_DOMAIN_SRC :=' not in sources:
    errors.append("vdr source manifest must define Teletext domain source group")
if 'core/vdr/src/SuiteBridgeTeletextResolver.cpp' not in sources:
    errors.append("vdr source manifest must compile SuiteBridgeTeletextResolver")
if '$(BROADCAST_TELETEXT_DOMAIN_SRC)' not in sources:
    errors.append("VDR_SRC must include the Teletext domain source group")

if errors:
    for error in errors:
        print(error, file=sys.stderr)
    raise SystemExit(1)

print("Phase 67 Teletext daemon-context ownership contract ok")
