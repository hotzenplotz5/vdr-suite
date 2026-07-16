#!/usr/bin/env python3
from __future__ import annotations

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

WRAPPER = Path(__file__).with_name("build_cpp_cached.py")


def run(command: list[str], cwd: Path, trace: Path) -> None:
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

    print("cached C++ build contract ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
