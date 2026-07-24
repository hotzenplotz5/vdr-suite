#!/usr/bin/env python3
"""Apply the Phase 61 Genre correction with exact C++ JSON string anchors."""

from __future__ import annotations

import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "tools" / "apply_phase61_live_genre_contract.py"

spec = importlib.util.spec_from_file_location("phase61_genre_patch", SOURCE)
if spec is None or spec.loader is None:
    raise SystemExit("ERROR: cannot load base Phase 61 patch module")

module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

controller = module.REPLACEMENTS[
    "api/rest/tests/test_genre_browser_controller.cpp"
]

controller[1] = (
    r'''    assert(contains(initialMovie, "\"eventId\":\"100\""));
    assert(contains(initialMovie, "\"channelName\":\"Das Erste HD\""));
    assert(contains(initialMovie, "\"available\":true"));
''',
    r'''    assert(!contains(initialMovie, "\"eventId\":\"100\""));
    assert(!contains(initialMovie, "Hartz Rot Gold"));
''',
)

controller[2] = (
    r'''    assert(contains(initialMovies, "\"eventId\":\"100\""));
    assert(!contains(initialMovies, "\"eventId\":\"101\""));
''',
    r'''    assert(!contains(initialMovies, "\"eventId\":\"100\""));
    assert(!contains(initialMovies, "\"eventId\":\"101\""));
''',
)

controller[4] = (
    r'''    assert(contains(movies, "\"eventId\":\"100\""));
    assert(!contains(movies, "\"eventId\":\"101\""));
''',
    r'''    assert(contains(movies, "\"eventId\":\"100\""));
    assert(!contains(movies, "\"eventId\":\"101\""));
    assert(contains(movies, "\"channelName\":\"Das Erste HD\""));
    assert(contains(movies, "\"available\":true"));
''',
)

raise SystemExit(module.main())
