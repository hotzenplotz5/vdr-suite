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

    select_start = source.find("const int ready = select(")
    accept_start = source.find("const int clientSocket = accept(", select_start + 1)
    if select_start < 0 or accept_start < 0:
        errors.append("missing listener select/accept boundary")
        return errors

    # Only inspect the listener select branch. The live-stream writer has its
    # own poll/read loop where retrying EINTR is correct and must not satisfy
    # (or violate) this listener-shutdown contract.
    select_branch = source[select_start:accept_start]
    select_eintr_break = re.search(
        r"if\s*\(errno\s*==\s*EINTR\)\s*(?:break\s*;|\{\s*break\s*;\s*\})",
        select_branch,
        re.MULTILINE)
    if not select_eintr_break:
        errors.append("missing EINTR break handling")

    select_eintr_continue = re.search(
        r"if\s*\(errno\s*==\s*EINTR\)\s*(?:continue\s*;|\{\s*continue\s*;\s*\})",
        select_branch,
        re.MULTILINE)
    if select_eintr_continue:
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
