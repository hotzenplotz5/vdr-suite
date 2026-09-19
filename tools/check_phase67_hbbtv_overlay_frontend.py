#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

live = (ROOT / "web/frontend/live-tv-view.js").read_text()
client = (ROOT / "web/frontend/api/client-api.js").read_text()
qoi = (ROOT / "web/frontend/hbbtv-qoi.js").read_text()
index = (ROOT / "web/frontend/index.html").read_text()
paths = (ROOT / "core/http/src/TestHttpServerPaths.inc").read_text()
install = (ROOT / "mk/install.mk").read_text()

required = (
    (live, "function openHbbtvSession()"),
    (live, "function closeHbbtvSession()"),
    (live, "function pollHbbtvPresentation(sequence)"),
    (live, "function pollHbbtvMedia(sequence)"),
    (live, "function startHbbtvMediaPlayback(media, sequence)"),
    (live, "function stopHbbtvMediaPlayback(sequence, notifyServer, restoreBroadcast)"),
    (live, "function drawHbbtvPresentation(frame)"),
    (live, "function sendHbbtvInput(action)"),
    (live, "function alignHbbtvCanvas()"),
    (live, "vdr-suite-hbbtv-overlay"),
    (live, "vdr-suite-hbbtv-remote"),
    (live, "fetchClientHbbtvSessionLaunch"),
    (live, "fetchClientHbbtvSessionInput"),
    (live, "fetchClientHbbtvPresentation"),
    (client, "function fetchClientHbbtvSessionLaunch(options)"),
    (client, "function fetchClientHbbtvSessionStatus(options)"),
    (client, "function fetchClientHbbtvSessionInput(options)"),
    (client, "function fetchClientHbbtvSessionClose(options)"),
    (client, "function fetchClientHbbtvMedia(options)"),
    (client, "function mutateClientHbbtvMedia(options)"),
    (client, "function fetchClientHbbtvPresentation(options)"),
    (client, "activeSessionCsrfHeaders()"),
    (qoi, "global.VdrSuiteQoi = Object.freeze"),
    (qoi, "function decode(input)"),
    (index, '<script src="../frontend/hbbtv-qoi.js"></script>'),
    (
        paths,
        '{"/frontend/hbbtv-qoi.js", "hbbtv-qoi.js", '
        '"application/javascript; charset=utf-8", nullptr}',
    ),
    (
        install,
        "web/frontend/hbbtv-qoi.js "
        "$(DESTDIR)$(DATADIR)/web/frontend/hbbtv-qoi.js",
    ),
)

errors = []
for content, token in required:
    if token not in content:
        errors.append(f"missing HbbTV browser overlay token: {token}")

if index.index('<script src="../frontend/hbbtv-qoi.js"></script>') > index.index(
    '<script src="../frontend/live-tv-view.js"></script>'
):
    errors.append("QOI decoder must load before Live-TV overlay")

for forbidden in (
    "HBBAPPS",
    "HBBRUN",
    "HBBPRES",
    "LoadUrl",
    "RedButton",
    "ProcessKey",
    "executeJavascript",
    "VK_LEFT",
    "VK_ENTER",
):
    if forbidden in live or forbidden in client:
        errors.append(f"private HbbTV provider detail leaked to browser: {forbidden}")

if "fetch(" in live:
    errors.append("Live-TV HbbTV overlay bypasses VdrSuiteClientApi")

if errors:
    raise SystemExit("\n".join(errors))

print("Phase 67 HbbTV Live-TV browser overlay: PASS")
