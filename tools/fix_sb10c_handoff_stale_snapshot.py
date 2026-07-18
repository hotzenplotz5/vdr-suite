#!/usr/bin/env python3

from pathlib import Path

path = Path("vdr-plugin-suite-bridge/docs/VDR-SUITE-HANDOFF.md")
text = path.read_text(encoding="utf-8")

old = """SB.10a and SB.10b are Agent-side slices. Neither changes the plugin binary or
its local contract.

The later `main` head `410f104a45890dc86f3a83813552ffe3dd141202`
contains parallel Recording genre-artwork work only. It was not part of the
SB.10b acceptance head and must be compared and synchronized before SB.10c.
"""

new = """SB.10a, SB.10b and SB.10c are Agent-side slices. None changes the plugin
binary or its local contract.

The bridge branch included Suite `main` head
`d2e6f1745cdba3592ac49f2d7dba33626136fbbe` before SB.10c implementation.
Current `main` must be compared and synchronized again before SB.10d begins.
"""

if text.count(old) != 1:
    raise RuntimeError("stale SB.10b coordinated snapshot paragraph is not unique")

path.write_text(text.replace(old, new, 1), encoding="utf-8")
print("SB.10c handoff consistency fixed")
