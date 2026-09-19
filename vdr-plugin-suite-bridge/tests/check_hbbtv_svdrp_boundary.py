#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / "suitebridge_svdrp.cpp").read_text()
header = (root / "suitebridge.h").read_text()
command = (root / "suitebridge_hbbtv_command.cpp").read_text()

help_start = source.find("SVDRPHelpPages")
command_start = source.find("SVDRPCommand")
if help_start < 0 or command_start < 0 or command_start <= help_start:
    raise SystemExit("SuiteBridge SVDRP sections not found")

help_section = source[help_start:command_start]
command_section = source[command_start:]

for private_command in ("HBBAPPS", "HBBRUN"):
    if private_command in help_section:
        raise SystemExit(
            f"private HbbTV command leaked into public help: {private_command}"
        )

if "hbbtvCommand_.Handle(Command, Option)" not in command_section:
    raise SystemExit("private HbbTV dispatch missing")
if "SuiteBridgeHbbtvCommandService hbbtvCommand_" not in header:
    raise SystemExit("HbbTV command owner missing from SuiteBridge")
if '"HBBRUN"' not in command:
    raise SystemExit("private HbbTV runtime command missing")

for forbidden in (
    "LoadUrl",
    "RedButton",
    "ProcessKey",
    "StartApplication",
    "cefbrowser",
):
    if forbidden in source or forbidden in header or forbidden in command:
        raise SystemExit(
            f"browser command leaked into SuiteBridge boundary: {forbidden}"
        )

print("suitebridge HbbTV SVDRP boundary: PASS")
