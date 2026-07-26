#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
repository_source = (
    ROOT / "core/vdr/src/EpgEventRepository.cpp"
).read_text(encoding="utf-8")

already_transformed = "replaceAuthoritativeWindowForBackend" in repository_source

if already_transformed:
    # Incremental follow-up for a tree on which the full Phase 61 transform
    # already completed. Re-running the original transformation modules would
    # search for pre-transform source blocks and fail before later fixes run.
    from phase61_authoritative_epg import database_transaction
else:
    # Pristine tree: preserve the required transformation order.
    from phase61_authoritative_epg import repository
    from phase61_authoritative_epg import database_transaction
    from phase61_authoritative_epg import artwork_guard
    from phase61_authoritative_epg import service_controller
    from phase61_authoritative_epg import frontend_daemon
    from phase61_authoritative_epg import tests
    from phase61_authoritative_epg import artwork_guard_tests
    from phase61_authoritative_epg import docs

print("Phase 61 authoritative EPG cache reconciliation applied.")
