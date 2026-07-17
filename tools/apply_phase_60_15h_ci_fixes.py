#!/usr/bin/env python3

from pathlib import Path

SERVICE_PATH = Path("core/vdr/src/VdrRecordingArtworkService.cpp")
CHECKER_PATH = Path("tools/check_frontend_ownership_contracts.py")
DIAGNOSTICS_PATH = Path("phase-60-15-diagnostics.txt")
WORKFLOW_PATH = Path(".github/workflows/phase-60-15h-ci-fixes.yml")
SCRIPT_PATH = Path(__file__)


def replace_exactly_once(text: str, before: str, after: str, label: str) -> str:
    count = text.count(before)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {count}")
    return text.replace(before, after, 1)


def main() -> None:
    service = SERVICE_PATH.read_text(encoding="utf-8")
    service = replace_exactly_once(
        service,
        """        std::string content(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>());
""",
        """        std::string content{
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()};
""",
        "artwork binary iterator construction",
    )
    SERVICE_PATH.write_text(service, encoding="utf-8")

    checker = CHECKER_PATH.read_text(encoding="utf-8")
    checker = replace_exactly_once(
        checker,
        """    require(
        "test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recording-browser.js" in install_mk,
        "test-install-staging must verify recording-browser.js"
    )
""",
        """    require(
        "test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recording-browser.js" in install_mk,
        "test-install-staging must verify recording-browser.js"
    )
    require(
        "web/frontend/recording-artwork.js $(DESTDIR)$(DATADIR)/web/frontend/recording-artwork.js" in install_mk,
        "install-runtime must install recording-artwork.js",
    )
    require(
        "test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recording-artwork.js" in install_mk,
        "test-install-staging must verify recording-artwork.js",
    )
""",
        "frontend artwork install contract",
    )

    checker = replace_exactly_once(
        checker,
        """        require(
            f'path == "{asset_path}"' in test_http_server_cpp
            or f'"{asset_path}"' in test_http_server_cpp,
            "TestHttpServer must allow " + asset_path,
        )
        require(
            f'"{relative_path}"' in test_http_server_cpp,
            "TestHttpServer must map " + asset_path + " to " + relative_path,
        )
""",
        """        require(
            f'path == "{asset_path}"' in test_http_server_cpp
            or f'"{asset_path}"' in test_http_server_cpp,
            "TestHttpServer must allow " + asset_path,
        )

        if asset_path == "/frontend/recording-browser.js":
            require(
                "makeFrontendScriptBundleResponse(" in test_http_server_cpp
                and '"modules/recordings.js"' in test_http_server_cpp
                and '"recording-artwork.js"' in test_http_server_cpp,
                "TestHttpServer must bundle modules/recordings.js and recording-artwork.js for /frontend/recording-browser.js",
            )
        else:
            require(
                f'"{relative_path}"' in test_http_server_cpp,
                "TestHttpServer must map " + asset_path + " to " + relative_path,
            )
""",
        "frontend static bundle contract",
    )
    CHECKER_PATH.write_text(checker, encoding="utf-8")

    DIAGNOSTICS_PATH.unlink(missing_ok=False)
    WORKFLOW_PATH.unlink(missing_ok=False)
    SCRIPT_PATH.unlink(missing_ok=False)


if __name__ == "__main__":
    main()
