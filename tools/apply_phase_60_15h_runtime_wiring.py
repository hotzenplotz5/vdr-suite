#!/usr/bin/env python3

from pathlib import Path

RUNTIME_PATH = Path("core/daemon/src/DaemonRuntime.cpp")
WORKFLOW_PATH = Path(".github/workflows/phase-60-15h-runtime-wiring.yml")
SCRIPT_PATH = Path(__file__)


def replace_exactly_once(text: str, before: str, after: str, label: str) -> str:
    count = text.count(before)
    if count != 1:
        raise SystemExit(
            f"{label}: expected exactly one match, found {count}"
        )
    return text.replace(before, after, 1)


def main() -> None:
    text = RUNTIME_PATH.read_text(encoding="utf-8")

    text = replace_exactly_once(
        text,
        '#include "TestHttpServer.h"\n',
        '#include "TestHttpServer.h"\n'
        '#include "RecordingArtworkHttpServer.h"\n',
        "runtime include wiring",
    )

    text = replace_exactly_once(
        text,
        '    httpServer_ = std::make_unique<TestHttpServer>(*apiRouter_);\n',
        '    httpServer_ = std::make_unique<RecordingArtworkHttpServer>(\n'
        '        std::make_unique<TestHttpServer>(*apiRouter_),\n'
        '        *vdrRecordingCacheRepository_,\n'
        '        config_.recordingArtworkRoots());\n',
        "runtime HTTP server wiring",
    )

    RUNTIME_PATH.write_text(text, encoding="utf-8")

    WORKFLOW_PATH.unlink(missing_ok=False)
    SCRIPT_PATH.unlink(missing_ok=False)


if __name__ == "__main__":
    main()
