#!/usr/bin/env python3
from pathlib import Path

source = (Path(__file__).resolve().parents[1] / "suitebridge_svdrp.cpp").read_text()
help_start = source.find("SVDRPHelpPages")
command_start = source.find("SVDRPCommand", help_start)
if help_start < 0 or command_start < 0:
    raise SystemExit("SuiteBridge SVDRP boundary not found")

help_section = source[help_start:command_start]
for private_command in ("TTXC", "TTXP"):
    if private_command in help_section:
        raise SystemExit(f"private Teletext command leaked into public help: {private_command}")

command_section = source[command_start:]
if "teletextCommand_.Handle(Command, Option)" not in command_section:
    raise SystemExit("Teletext private command dispatch missing")

print("suitebridge teletext SVDRP boundary: PASS")
