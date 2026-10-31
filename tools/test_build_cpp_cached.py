#!/usr/bin/env python3
from __future__ import annotations

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

WRAPPER = Path(__file__).with_name("build_cpp_cached.py")


def run(command: list[str], cwd: Path, trace: Path, extra_env: dict[str, str] | None = None) -> None:
    subprocess.run(
        [
            sys.executable,
            str(WRAPPER),
            "--compiler",
            "g++",
            "--cache-dir",
            str(cwd / ".build" / "obj"),
            "--trace-file",
            str(trace),
            "--",
            *command,
        ],
        cwd=cwd,
        check=True,
        env={**os.environ, **(extra_env or {})},
    )


def events(trace: Path) -> list[dict[str, str]]:
    return [json.loads(line) for line in trace.read_text().splitlines() if line]


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="vdr-suite-cxx-cache-") as raw:
        root = Path(raw)
        trace = root / "trace.jsonl"
        (root / "value.h").write_text("#define VALUE 7\n")
        (root / "shared.cpp").write_text(
            '#include "value.h"\nint shared() { return VALUE; }\n'
        )
        (root / "one.cpp").write_text(
            "int shared(); int main() { return shared() == 7 ? 0 : 1; }\n"
        )
        (root / "two.cpp").write_text(
            "int shared(); int main() { return shared() == 7 ? 0 : 1; }\n"
        )

        run(
            ["-std=c++17", "-pthread", "shared.cpp", "one.cpp", "-o", ".build/one"],
            root,
            trace,
        )
        run(
            ["-std=c++17", "-pthread", "shared.cpp", "two.cpp", "-o", ".build/two"],
            root,
            trace,
        )
        subprocess.run([str(root / ".build" / "one")], check=True)
        subprocess.run([str(root / ".build" / "two")], check=True)

        first = events(trace)
        shared_compiles = [
            event
            for event in first
            if event["event"] == "compile" and event["source"] == "shared.cpp"
        ]
        shared_hits = [
            event
            for event in first
            if event["event"] == "hit" and event["source"] == "shared.cpp"
        ]
        if len(shared_compiles) != 1 or len(shared_hits) != 1:
            raise AssertionError(f"shared object was not reused: {first}")

        object_mtime = max(
            path.stat().st_mtime_ns
            for path in (root / ".build" / "obj").rglob("*.o")
        )
        header = root / "value.h"
        header.write_text("#define VALUE 8\n")
        os.utime(header, ns=(object_mtime + 1, object_mtime + 1))
        (root / "two.cpp").write_text(
            "int shared(); int main() { return shared() == 8 ? 0 : 1; }\n"
        )
        run(
            ["-std=c++17", "-pthread", "shared.cpp", "two.cpp", "-o", ".build/two"],
            root,
            trace,
        )
        subprocess.run([str(root / ".build" / "two")], check=True)

        final = events(trace)
        shared_compiles = [
            event
            for event in final
            if event["event"] == "compile" and event["source"] == "shared.cpp"
        ]
        if len(shared_compiles) != 2:
            raise AssertionError(
                f"header dependency did not rebuild shared.cpp: {final}"
            )

        depfiles = list((root / ".build" / "obj").rglob("*.d"))
        if not depfiles:
            raise AssertionError("no dependency files were generated")

        # A commit-only version change must *not* recompile unrelated
        # translation units. The actual version consumer must be rebuilt.
        (root / "ServerBuildIdentity.h").write_text(
            '#pragma once\n#ifndef VDR_SUITE_SERVER_VERSION\n'
            '#define VDR_SUITE_SERVER_VERSION "unknown"\n#endif\n'
            'inline constexpr const char* version = VDR_SUITE_SERVER_VERSION;\n'
        )
        (root / "version.cpp").write_text(
            '#include "ServerBuildIdentity.h"\n'
            'const char* get_version() { return version; }\n'
        )
        (root / "version_main.cpp").write_text(
            '#include <string>\n'
            'const char* get_version();\n'
            'int main() { return std::string(get_version()) == "git-new" ? 0 : 1; }\n'
        )
        v1 = [
            "-std=c++17", '-DVDR_SUITE_SERVER_VERSION="git-old"',
            "shared.cpp", "version.cpp", "version_main.cpp", "-o", ".build/old-version"
        ]
        v2 = [
            "-std=c++17", '-DVDR_SUITE_SERVER_VERSION="git-new"',
            "shared.cpp", "version.cpp", "version_main.cpp", "-o", ".build/new-version"
        ]
        run(v1, root, trace, {"VDR_SUITE_CXX_JOBS": "2"})
        before = len(events(trace))
        run(v2, root, trace, {
            "VDR_SUITE_CXX_JOBS": "2",
            "VDR_SUITE_CXX_CACHE_FROM_VERSION": "git-old",
        })
        subprocess.run([str(root / ".build" / "new-version")], check=True)
        delta = events(trace)[before:]
        def count(event: str, src: str) -> int:
            return sum(e["event"] == event and e["source"] == src for e in delta)
        if count("compile", "shared.cpp") or count("compile", "version_main.cpp"):
            raise AssertionError(f"version bump rebuilt unrelated sources: {delta}")
        if count("compile", "version.cpp") != 1:
            raise AssertionError(f"version translation unit not rebuilt: {delta}")
        if count("hit", "shared.cpp") != 1:
            raise AssertionError(f"stable object not reused: {delta}")

    print("cached C++ build contract ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
