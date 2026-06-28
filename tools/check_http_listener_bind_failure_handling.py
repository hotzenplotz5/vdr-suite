#!/usr/bin/env python3
from pathlib import Path
import sys

SOURCE = Path("core/http/src/SimpleHttpListener.cpp")

REQUIRED_SNIPPETS = [
    "if (listenSocket < 0)",
    "return 1;",
    "failed to bind HTTP listener to",
    "return -1;",
    "listen failed:",
]

FORBIDDEN_SNIPPETS = [
    "#include <stdexcept>",
    "throw std::runtime_error",
]


def main() -> int:
    source = SOURCE.read_text(encoding="utf-8")

    missing = [snippet for snippet in REQUIRED_SNIPPETS if snippet not in source]
    forbidden = [snippet for snippet in FORBIDDEN_SNIPPETS if snippet in source]

    if missing or forbidden:
        print("HTTP listener bind failure handling check failed:")
        for snippet in missing:
            print(f"- missing required snippet: {snippet}")
        for snippet in forbidden:
            print(f"- forbidden snippet present: {snippet}")
        return 1

    print("HTTP listener bind failure handling check passed.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        print(f"HTTP listener bind failure handling check failed: {error}")
        sys.exit(1)
