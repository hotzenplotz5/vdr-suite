#!/usr/bin/env python3
"""Validate the synchronous web frontend i18n asset and ownership contract."""

from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FRONTEND = ROOT / "web" / "frontend"


def read(path: Path) -> str:
    if not path.exists():
        raise RuntimeError(f"required i18n file missing: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


def catalog(path: Path, locale: str) -> dict[str, str]:
    text = read(path)
    marker = f"catalogs.{locale} = Object.freeze("
    start = text.find(marker)
    if start < 0:
        raise RuntimeError(f"locale catalog marker missing: {locale}")
    payload_start = start + len(marker)
    payload_end = text.find(");", payload_start)
    if payload_end < 0:
        raise RuntimeError(f"locale catalog terminator missing: {locale}")
    return json.loads(text[payload_start:payload_end])


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> int:
    de = catalog(FRONTEND / "locales" / "de.js", "de")
    en = catalog(FRONTEND / "locales" / "en.js", "en")
    i18n = read(FRONTEND / "platform" / "i18n.js")
    index = read(FRONTEND / "index.html")
    app = read(FRONTEND / "app.js")
    recordings = read(FRONTEND / "modules" / "recordings.js")
    install = read(ROOT / "mk" / "install.mk")
    server = read(ROOT / "core" / "http" / "src" / "TestHttpServer.cpp")

    require(set(de) == set(en), "de/en locale catalogs must expose identical keys")
    require(len(de) >= 80, "i18n foundation must contain the shell, settings and Move key set")

    referenced = set(re.findall(r"data-i18n(?:-aria-label|-placeholder)?=\"([^\"]+)\"", index))
    referenced.update(re.findall(r"frontendTranslate\(\s*['\"]([^'\"]+)", app))
    referenced.update(re.findall(r"recordingBrowserTranslate\(\s*['\"]([^'\"]+)", recordings))
    missing = sorted(referenced - set(de))
    require(not missing, "translation keys missing from catalogs: " + ", ".join(missing))

    require("fetch(" not in i18n, "platform/i18n.js must stay HTTP-free")
    require("XMLHttpRequest" not in i18n, "platform/i18n.js must stay HTTP-free")
    require("window.VdrSuiteI18n" in i18n or "global.VdrSuiteI18n" in i18n, "i18n API export missing")
    require("localStorage" in i18n, "i18n locale preference must be persisted in browser storage")
    require("recordingBrowserTranslate(" in recordings, "Recording Move workflow must use i18n keys")
    require("recordings.move.targetReady" in recordings, "Move ready feedback must use its i18n key")
    require("function recordingBrowserValidateNewMoveFolderName(value)" in recordings, "Move new-folder validation helper missing")
    require("function recordingBrowserJoinMoveFolderPath(parentPath, folderName)" in recordings, "Move new-folder path helper missing")
    require("recordings.move.createFolder" in recordings, "Move new-folder trigger must use i18n")
    require("recordings.move.newFolderSelected" in recordings, "Move new-folder selection feedback must use i18n")
    require("function recordingBrowserCreateDeleteEditor(recording, folderData, resultBox)" in recordings, "Recording Delete editor missing")
    require("recordings.delete.finalValidation" in recordings, "Recording Delete must repeat validation before execution")
    require("recordings.delete.execute" in recordings, "Recording Delete execution label must use i18n")
    require("recordingBrowserScheduleDeleteFolderReload();" in recordings, "Recording Delete cache readback missing")
    require("frontendTranslate('settings.language'" in app, "Settings must expose the language selector")

    for asset in [
        "platform/i18n.js",
        "locales/de.js",
        "locales/en.js",
    ]:
        require(f"web/frontend/{asset}" in install, f"install contract missing {asset}")
        require(f'"{asset}"' in server, f"static serving contract missing {asset}")

    print("frontend i18n contracts ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
