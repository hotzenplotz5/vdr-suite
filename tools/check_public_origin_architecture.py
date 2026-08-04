#!/usr/bin/env python3
"""Validate the VDR-Suite public-origin and base-path integration contract."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class ContractFailure(RuntimeError):
    pass


def read(relative: str) -> str:
    path = ROOT / relative
    if not path.is_file():
        raise ContractFailure(f"required file missing: {relative}")
    return path.read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractFailure(message)


def check_index(index: str) -> None:
    resolver = '<script src="../frontend/platform/public-url.js"></script>'
    stylesheet = '<link rel="stylesheet" href="../frontend/style.css">'
    favicon = '../channel-logos/vdr-suite-brand/favicon.svg'
    logo = '../channel-logos/vdr-suite-brand/logo-vdr-suite-dark.svg'

    require(resolver in index, "index.html must load the public URL runtime")
    require(stylesheet in index, "index.html stylesheet must use the relative frontend path")
    require(favicon in index, "index.html favicon must use the relative logo path")
    require(logo in index, "index.html brand logo must use the relative logo path")

    first_script = re.search(r'<script\s+src="([^"]+)"', index)
    require(first_script is not None, "index.html must contain external scripts")
    require(
        first_script.group(1) == "../frontend/platform/public-url.js",
        "public-url.js must be the first external frontend script",
    )
    require(
        index.index(resolver) < index.index(stylesheet),
        "public-url.js must load before parser-initiated stylesheet assets",
    )

    require('src="/frontend/' not in index, "index.html must not use absolute frontend src paths")
    require('href="/frontend/' not in index, "index.html must not use absolute frontend href paths")
    require('src="/channel-logos/' not in index, "index.html must not use absolute logo src paths")
    require('href="/channel-logos/' not in index, "index.html must not use absolute logo href paths")

    for path in re.findall(r'(?:src|href)="(\.\./(?:frontend|channel-logos)/[^"]+)"', index):
        require("\\" not in path and "/../" not in path[3:], f"unsafe relative bootstrap path: {path}")


def check_runtime(runtime: str) -> None:
    required_tokens = (
        "const SCRIPT_SUFFIX = '/frontend/platform/public-url.js';",
        "'/api'",
        "'/frontend'",
        "'/channel-logos'",
        "'/recording-artwork'",
        "basePath: basePath",
        "resolvePath: resolvePath",
        "Object.freeze",
        "Object.defineProperty(global, 'VdrSuitePublicUrl'",
        "installFetchAdapter();",
        "installEventSourceAdapter();",
        "installDomAdapters();",
        "installStyleAdapters();",
        "HTMLScriptElement",
        "HTMLImageElement",
        "CSSStyleDeclaration",
    )
    for token in required_tokens:
        require(token in runtime, f"public URL runtime missing contract token: {token}")

    forbidden_tokens = (
        "document.cookie",
        "localStorage",
        "sessionStorage",
        "Authorization",
        "X-CSRF-Token",
        "innerHTML",
        "eval(",
    )
    for token in forbidden_tokens:
        require(token not in runtime, f"public URL runtime must not own security state or code execution: {token}")


def check_http_asset_table(paths_source: str) -> None:
    entry = (
        '{"/frontend/platform/public-url.js", "platform/public-url.js", '
        '"application/javascript; charset=utf-8", nullptr}'
    )
    require(entry in paths_source, "daemon frontend asset table must expose public-url.js")
    require(
        paths_source.index(entry) < paths_source.index('{"/frontend/platform/bootstrap.js"'),
        "public-url.js must precede platform/bootstrap.js in the explicit asset table",
    )

    settings_entry = (
        '{"/frontend/settings-series-artwork.js", "settings-series-artwork.js", '
        '"application/javascript; charset=utf-8", nullptr}'
    )
    require(
        settings_entry in paths_source,
        "daemon frontend asset table must expose settings-series-artwork.js",
    )


def check_make_integration(makefile: str, make_fragment: str) -> None:
    require("include mk/public-origin.mk" in makefile, "Makefile must include mk/public-origin.mk")
    required_tokens = (
        "install-runtime: install-public-origin-runtime",
        "install: install-nginx",
        "web/frontend/platform/public-url.js",
        "packaging/nginx/vdr-suite.conf",
        "test-public-origin-install-staging",
        "test-frontend-contracts: test-public-url-runtime",
        "test-architecture: test-public-origin",
    )
    for token in required_tokens:
        require(token in make_fragment, f"public-origin make integration missing: {token}")

    runtime_section = make_fragment.split("install-runtime: install-public-origin-runtime", 1)[1]
    require(
        "install-nginx" not in runtime_section.split("install:", 1)[0],
        "install-runtime must not install or activate Nginx configuration",
    )


def check_nginx(nginx: str) -> None:
    required_tokens = (
        "location = /vdr-suite {",
        "location = /vdr-suite/ {",
        "location = /vdr-suite/frontend {",
        "return 308 /vdr-suite/frontend/;",
        "location ^~ /vdr-suite/ {",
        "proxy_pass http://127.0.0.1:18080/;",
        "proxy_set_header X-Forwarded-Prefix /vdr-suite;",
        "proxy_cookie_path / /vdr-suite/;",
        "proxy_buffering off;",
        "proxy_cache off;",
        "proxy_read_timeout 3600s;",
    )
    for token in required_tokens:
        require(token in nginx, f"Nginx public-origin snippet missing: {token}")

    forbidden_tokens = (
        "uvicorn.sock",
        "sub_filter",
        "proxy_pass http://unix:",
        "proxy_pass unix:",
    )
    for token in forbidden_tokens:
        require(token not in nginx, f"Nginx snippet contains forbidden ownership/rewrite token: {token}")

    root_api_location = re.search(
        r"location\s+(?:=|\^~|~\*?|@)?\s*/api(?:/|\s|\{)",
        nginx,
    )
    require(root_api_location is None, "Nginx snippet must never claim the public root /api namespace")

    require(
        nginx.count("location ^~ /vdr-suite/") == 1,
        "Nginx snippet must contain exactly one Suite prefix proxy location",
    )
    require(
        "proxy_pass http://127.0.0.1:18080/;" in nginx,
        "trailing-slash proxy_pass is required to strip /vdr-suite",
    )


def check_documentation(document: str) -> None:
    required_tokens = (
        "Phase 62 Slice 3A",
        "/vdr-suite/frontend/",
        "X-Forwarded-Prefix",
        "proxy_cookie_path",
        "does not activate",
        "separate runtime approval",
    )
    for token in required_tokens:
        require(token in document, f"public-origin documentation missing: {token}")


def main() -> int:
    try:
        check_index(read("web/frontend/index.html"))
        check_runtime(read("web/frontend/platform/public-url.js"))
        check_http_asset_table(read("core/http/src/TestHttpServerPaths.inc"))
        check_make_integration(read("Makefile"), read("mk/public-origin.mk"))
        check_nginx(read("packaging/nginx/vdr-suite.conf"))
        check_documentation(read("docs/development/phase-62-public-origin-base-path.md"))
        require(
            "test_public_url_runtime.js" in read("mk/public-origin.mk"),
            "public URL Node runtime test must be registered",
        )
        read("web/frontend/tests/test_public_url_runtime.js")
    except ContractFailure as exc:
        print(f"public-origin architecture contract failed: {exc}", file=sys.stderr)
        return 1

    print("public-origin architecture contracts ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
