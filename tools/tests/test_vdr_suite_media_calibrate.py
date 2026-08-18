#!/usr/bin/env python3
"""Regression tests for the media transcode calibrator policy contract."""

from __future__ import annotations

import importlib.util
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[2]
MODULE_PATH = ROOT / "tools" / "vdr_suite_media_calibrate.py"

spec = importlib.util.spec_from_file_location("vdr_suite_media_calibrate", MODULE_PATH)
assert spec is not None and spec.loader is not None
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

assert module.PROFILE_VERSION == 3
assert module.DEFAULT_REAL_SOURCE_SECONDS == 30
assert module.MINIMUM_REALTIME_SPEED == 1.25

choice, measured = module.policy_choice(
    "deinterlace",
    {
        "superfast": 1.03,
        "veryfast": 0.682,
        "faster": 0.507,
        "fast": 0.376,
    },
)
assert choice == "superfast"
assert not measured

choice, measured = module.policy_choice(
    "uhd-source",
    {
        "superfast": 0.940,
        "veryfast": 0.861,
        "faster": 0.732,
        "fast": 0.633,
    },
)
assert choice == "veryfast"
assert not measured

choice, measured = module.policy_choice(
    "standard",
    {
        "superfast": 2.27,
        "veryfast": 1.79,
        "faster": 1.14,
        "fast": 0.964,
    },
)
assert choice == "veryfast"
assert measured

choice, measured = module.policy_choice(
    "deinterlace",
    {
        "superfast": 1.54,
        "veryfast": 0.992,
        "faster": 0.80,
        "fast": 0.70,
    },
)
assert choice == "superfast"
assert measured

choice, measured = module.policy_choice(
    "deinterlace",
    {
        "superfast": 2.20,
        "veryfast": 1.85,
        "faster": 1.55,
        "fast": 1.31,
    },
)
assert choice == "fast"
assert measured
