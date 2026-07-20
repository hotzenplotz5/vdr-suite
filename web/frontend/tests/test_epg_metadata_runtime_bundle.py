#!/usr/bin/env python3

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
INSTALL_MK = ROOT / "mk" / "install.mk"
LOADER = ROOT / "web" / "frontend" / "platform" / "deferred-runtime-loader.js"
HTTP_SERVER = ROOT / "core" / "http" / "src" / "TestHttpServer.cpp"
SOURCES = (
    ROOT / "web" / "frontend" / "epg-metadata-detail.js",
    ROOT / "web" / "frontend" / "epg-searchtimer-actions.js",
    ROOT / "web" / "frontend" / "epg-metadata-detail-hook.js",
)


def main() -> int:
    install = INSTALL_MK.read_text(encoding="utf-8")
    loader = LOADER.read_text(encoding="utf-8")
    http_server = HTTP_SERVER.read_text(encoding="utf-8")

    source_markers = tuple(str(path.relative_to(ROOT)) for path in SOURCES)
    positions = tuple(install.index(marker) for marker in source_markers)
    assert positions[0] < positions[1] < positions[2]

    assert "web/frontend/.epg-searchtimer-actions.js.tmp" in install
    assert "web/frontend/epg-searchtimer-actions.js" in install
    assert "mv -f" in install

    assert "'/frontend/epg-searchtimer-actions.js'" in loader
    assert "window.VdrSuiteEpgMetadataDetail" in loader
    assert "window.VdrSuiteEpgSearchTimerActions" in loader
    assert "window.VdrSuiteEpgMetadataDetailHook" in loader
    assert "'/frontend/epg-metadata-detail.js'" not in loader
    assert "'/frontend/epg-metadata-detail-hook.js'" not in loader

    assert 'path == "/frontend/epg-searchtimer-actions.js"' in http_server
    assert 'path == "/frontend/epg-metadata-detail.js"' not in http_server
    assert 'path == "/frontend/epg-metadata-detail-hook.js"' not in http_server

    with tempfile.TemporaryDirectory(prefix="vdr-suite-epg-metadata-bundle-") as directory:
        bundle = Path(directory) / "epg-searchtimer-actions.js"
        bundle.write_text(
            "\n\n".join(path.read_text(encoding="utf-8").rstrip() for path in SOURCES)
            + "\n",
            encoding="utf-8",
        )
        source = bundle.read_text(encoding="utf-8")

        renderer = source.index("global.VdrSuiteEpgMetadataDetail = Object.freeze")
        searchtimer = source.index("global.VdrSuiteEpgSearchTimerActions = Object.freeze")
        hook = source.index("global.VdrSuiteEpgMetadataDetailHook = Object.freeze")
        assert renderer < searchtimer < hook

        subprocess.run(["node", "--check", str(bundle)], check=True)

    print("test_epg_metadata_runtime_bundle passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
