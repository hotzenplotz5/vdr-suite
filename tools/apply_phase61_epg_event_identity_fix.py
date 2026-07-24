#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
repository_source = (
    ROOT / "core/vdr/src/EpgEventRepository.cpp"
).read_text(encoding="utf-8")

# repository.py must run before database_transaction.py on a pristine tree.
# On a previously transformed tree, running it again would search for the
# pre-transaction replacement block and fail before the incremental database
# move-semantics update can be applied.
if "replaceAuthoritativeWindowForBackend" not in repository_source:
    from phase61_authoritative_epg import repository

from phase61_authoritative_epg import database_transaction
from phase61_authoritative_epg import artwork_guard
from phase61_authoritative_epg import service_controller
from phase61_authoritative_epg import frontend_daemon
from phase61_authoritative_epg import tests
from phase61_authoritative_epg import artwork_guard_tests
from phase61_authoritative_epg import docs

print("Phase 61 authoritative EPG cache reconciliation applied.")
