#!/usr/bin/env python3
"""One-command Public API v1 acceptance. Default mode performs GET requests only."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "api-v1-acceptance"))

from runner import main

if __name__ == "__main__":
    sys.exit(main())
