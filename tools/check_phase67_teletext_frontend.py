#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
VIEW = ROOT / "web/frontend/teletext-view.js"
LIVE = ROOT / "web/frontend/live-tv-view.js"
CLIENT = ROOT / "web/frontend/api/client-api.js"
INDEX = ROOT / "web/frontend/index.html"
PATHS = ROOT / "core/http/src/TestHttpServerPaths.inc"
INSTALL = ROOT / "mk/install.mk"
TEST = ROOT / "web/frontend/tests/test_phase67_teletext_view.js"

errors: list[str] = []

for path in (VIEW, LIVE, CLIENT, INDEX, PATHS, INSTALL, TEST):
    if not path.is_file():
        errors.append(f"missing Phase 67 Teletext frontend file: {path.relative_to(ROOT)}")

def read(path: Path) -> str:
    return path.read_text(encoding="utf-8") if path.is_file() else ""

view = read(VIEW)
live = read(LIVE)
client = read(CLIENT)
index = read(INDEX)
paths = read(PATHS)
install = read(INSTALL)
test = read(TEST)

for fragment in (
    "global.VdrSuiteTeletextView = api",
    "fetchClientTeletextService",
    "fetchClientTeletextPage",
    "cells.length === 1000",
    "rowIndex < 25",
    "column < 40",
    "safePageNumber",
    "safeSubpageCode",
    "navigatePage",
    "navigateSubpage",
    "Unterseite Auto",
    "credentials: 'same-origin'",
    "cache: 'no-store'",
    "body.vdr-suite-teletext-open .vdr-suite-teletext-overlay",
    "body.vdr-suite-teletext-open .vdr-suite-live-tv-player",
    "right:32vw",
    "width:32vw",
    "right:30vw",
    "width:30vw",
    "max-height:66vh",
    "setDesktopCompanionActive(true)",
    "setDesktopCompanionActive(false)",
):
    if fragment not in view:
        errors.append(f"Teletext view missing contract fragment: {fragment}")

for forbidden in (
    "fetch(",
    "TTXC 1",
    "TTXP 1",
    "/var/cache/vdr/vtx",
    "OsdTeletext::",
    "VdrSuitePlaybackShell",
    "createLivePanel",
    ".destroy(",
    "appendChild(state.playback",
    "createMediaSession",
):
    if forbidden in view:
        errors.append(f"Teletext view must not own provider/playback detail: {forbidden}")

for fragment in (
    "global.VdrSuiteTeletextView",
    "teletext.createLauncher(",
    "head.appendChild(teletextButton)",
):
    if fragment not in live:
        errors.append(f"Live-TV integration missing Teletext launcher fragment: {fragment}")

for fragment in (
    "function fetchClientTeletextService(options)",
    "function fetchClientTeletextPage(options)",
    "requestJson('/api/vdr/broadcast/teletext/service', options)",
    "requestJson('/api/vdr/broadcast/teletext/page', options)",
    "fetchClientTeletextService: fetchClientTeletextService",
    "fetchClientTeletextPage: fetchClientTeletextPage",
):
    if fragment not in client:
        errors.append(f"Web Client API missing Teletext wrapper fragment: {fragment}")

teletext_script = '<script src="../frontend/teletext-view.js"></script>'
live_script = '<script src="../frontend/live-tv-view.js"></script>'
if teletext_script not in index:
    errors.append("index.html missing Teletext frontend runtime")
elif live_script not in index:
    errors.append("index.html missing Live-TV runtime")
elif index.index(teletext_script) > index.index(live_script):
    errors.append("Teletext runtime must load before Live-TV launcher integration")

if '{"/frontend/teletext-view.js", "teletext-view.js"' not in paths:
    errors.append("HTTP frontend asset map missing teletext-view.js")

for fragment in (
    "web/frontend/teletext-view.js $(DESTDIR)$(DATADIR)/web/frontend/teletext-view.js",
    "node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/teletext-view.js",
):
    if fragment not in install:
        errors.append(f"install contract missing Teletext frontend fragment: {fragment}")

for fragment in (
    "Phase 67 Teletext frontend ownership and navigation contract ok",
    "!teletext.includes('fetch(')",
    "!teletext.includes('VdrSuitePlaybackShell')",
):
    if fragment not in test:
        errors.append(f"Teletext frontend regression test missing fragment: {fragment}")

if errors:
    for error in errors:
        print(error, file=sys.stderr)
    raise SystemExit(1)

print("Phase 67 Teletext frontend architecture contract ok")
