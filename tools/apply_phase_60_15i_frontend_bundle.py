#!/usr/bin/env python3

from pathlib import Path

SERVER_PATH = Path("core/http/src/TestHttpServer.cpp")
WORKFLOW_PATH = Path(".github/workflows/phase-60-15i-frontend-bundle.yml")
SCRIPT_PATH = Path(__file__)

BEFORE = '''    if (path == "/frontend/recording-browser.js")
    {
        return makeFrontendAssetResponse(
            "recording-browser.js",
            "application/javascript; charset=utf-8");
    }
'''

AFTER = '''    if (path == "/frontend/recording-browser.js")
    {
        return makeFrontendScriptBundleResponse(
            "modules/recordings.js",
            "recording-artwork.js");
    }
'''


def main() -> None:
    text = SERVER_PATH.read_text(encoding="utf-8")
    count = text.count(BEFORE)
    if count != 1:
        raise SystemExit(
            f"frontend Recording bundle: expected one match, found {count}"
        )

    SERVER_PATH.write_text(
        text.replace(BEFORE, AFTER, 1),
        encoding="utf-8",
    )

    WORKFLOW_PATH.unlink(missing_ok=False)
    SCRIPT_PATH.unlink(missing_ok=False)


if __name__ == "__main__":
    main()
