#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / "suitebridge_svdrp.cpp").read_text()
header = (root / "suitebridge.h").read_text()

help_start = source.find("SVDRPHelpPages")
command_start = source.find("SVDRPCommand")
if help_start < 0 or command_start < 0 or command_start <= help_start:
    raise SystemExit("SuiteBridge SVDRP sections not found")

help_section = source[help_start:command_start]
command_section = source[command_start:]

if "HBBAPPS" in help_section:
    raise SystemExit("private HbbTV discovery command leaked into public help")
if "hbbtvCommand_.Handle(Command, Option)" not in command_section:
    raise SystemExit("private HbbTV discovery dispatch missing")
if "SuiteBridgeHbbtvCommandService hbbtvCommand_" not in header:
    raise SystemExit("HbbTV command owner missing from SuiteBridge")

for forbidden in (
    "LoadUrl",
    "RedButton",
    "ProcessKey",
    "StartApplication",
    "cefbrowser",
):
    if forbidden in source or forbidden in header:
        raise SystemExit(f"browser command leaked into SuiteBridge public owner: {forbidden}")

print("suitebridge HbbTV SVDRP boundary: PASS")
