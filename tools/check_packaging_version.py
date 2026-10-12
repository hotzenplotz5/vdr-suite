#!/usr/bin/env python3
"""Verify coherent source-level VDR-Suite version declarations; no build needed."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
version = (ROOT / "VERSION").read_text(encoding="ascii").strip()
if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+(?:[~-][a-zA-Z0-9.+~-]+)?", version):
    sys.exit("invalid VERSION: " + version)
header = (ROOT / "vdr-plugin-suite-bridge/suitebridge_plugin_identity.h").read_text()
match = re.search(r'\bVersion\s*=\s*"([^"]+)"', header)
if not match or match.group(1) != version:
    sys.exit("plugin identity does not match VERSION")
print("PACKAGING_VERSION_OK=" + version)
