#!/usr/bin/env python3
from pathlib import Path
import re
import sys

SOURCE = Path("core/http/src/SimpleHttpListener.cpp")
HEADER = Path("core/http/include/SimpleHttpListener.h")

REQUIRED_SOURCE_SNIPPETS = [
    "if (listenSocket < 0)",
    "return 1;",
    "failed to bind HTTP listener to",
    "return -1;",
    "listen failed:",
    "#include <sys/select.h>",
    "while (!shouldStop_())",
    "select(",
]

REQUIRED_HEADER_SNIPPETS = [
    "#include <functional>",
    "std::function<bool()> shouldStop",
    "std::function<bool()> shouldStop_;",
]

FORBIDDEN_SOURCE_SNIPPETS = [
    "#include <stdexcept>",
    "throw std::runtime_error",
]


def requires_break_on_eintr(source: str) -> list[str]:
    errors = []

    select_eintr = re.search(
        r"if\s*\(errno\s*==\s*EINTR\)\s*\{\s*break\s*;\s*\}",
        source,
        re.MULTILINE)
    if not select_eintr:
        errors.append("missing EINTR break handling")

    if "if (errno == EINTR) {\n                continue;" in source:
        errors.append("EINTR must not continue the listener loop")

    return errors


def main() -> int:
    source = SOURCE.read_text(encoding="utf-8")
    header = HEADER.read_text(encoding="utf-8")

    missing = [
        snippet
        for snippet in REQUIRED_SOURCE_SNIPPETS
        if snippet not in source
    ]
    missing.extend(
        snippet
        for snippet in REQUIRED_HEADER_SNIPPETS
        if snippet not in header
    )

    forbidden = [
        snippet
        for snippet in FORBIDDEN_SOURCE_SNIPPETS
        if snippet in source
    ]

    semantic_errors = requires_break_on_eintr(source)

    if missing or forbidden or semantic_errors:
        print("HTTP listener bind failure handling check failed:")
        for snippet in missing:
            print(f"- missing required snippet: {snippet}")
        for snippet in forbidden:
            print(f"- forbidden snippet present: {snippet}")
        for error in semantic_errors:
            print(f"- {error}")
        return 1

    print("HTTP listener bind failure handling check passed.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        print(f"HTTP listener bind failure handling check failed: {error}")
        sys.exit(1)
