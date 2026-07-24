#!/usr/bin/env python3
from pathlib import Path

path = Path(__file__).resolve().parents[1] / "web/frontend/tests/test_epg_metadata_detail.js"
text = path.read_text(encoding="utf-8")

replacements = (
    (
        "let metadataRequest = null;\n"
        "let metadataRequestCount = 0;\n"
        "let recordingSearch = null;\n",
        "let metadataRequest = null;\n"
        "let metadataRequestCount = 0;\n"
        "const scheduledTimeouts = [];\n"
        "let recordingSearch = null;\n",
    ),
    (
        "  setTimeout(callback) {\n"
        "    callback();\n"
        "    return 1;\n"
        "  },\n",
        "  setTimeout(callback) {\n"
        "    scheduledTimeouts.push(callback);\n"
        "    return scheduledTimeouts.length;\n"
        "  },\n",
    ),
    (
        "  for (let index = 0; index < 12; index += 1) {\n"
        "    await Promise.resolve();\n"
        "  }\n\n"
        "  assert.ok(metadataRequest);\n",
        "  for (let attempt = 0;\n"
        "       attempt < 20 && detail.dataset.epgMetadataAvailable !== 'true';\n"
        "       attempt += 1) {\n"
        "    while (scheduledTimeouts.length) {\n"
        "      scheduledTimeouts.shift()();\n"
        "    }\n"
        "    await Promise.resolve();\n"
        "    await Promise.resolve();\n"
        "  }\n\n"
        "  assert.ok(metadataRequest);\n",
    ),
)

for old, new in replacements:
    if new in text:
        continue
    if old not in text:
        raise SystemExit(f"expected block not found in {path}")
    text = text.replace(old, new, 1)

path.write_text(text, encoding="utf-8")
print("Deterministic EPG metadata retry test wait applied.")
