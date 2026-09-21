#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
FILES = (
    "suitebridge_recording_identity.cpp",
    "suitebridge_recording_metadata_contract.cpp",
    "suitebridge_tvscraper_recording_adapter.cpp",
    "suitebridge_recording_metadata_command.cpp",
    "suitebridge_svdrp.cpp",
    "tests/test_suitebridge_recording_identity.cpp",
    "tests/test_suitebridge_recording_metadata_contract.cpp",
)
errors = []
for name in FILES:
    if not (ROOT / name).is_file():
        errors.append(f"missing file: {name}")

if not errors:
    identity = (ROOT / FILES[0]).read_text(encoding="utf-8")
    contract = (ROOT / FILES[1]).read_text(encoding="utf-8")
    adapter = (ROOT / FILES[2]).read_text(encoding="utf-8")
    command = (ROOT / FILES[3]).read_text(encoding="utf-8")
    svdrp = (ROOT / FILES[4]).read_text(encoding="utf-8")

    required = {
        "identity": (
            '"vdr-suite-recording-native-v1\\n"',
            "kMaximumNativeIdBytes",
            "std::setw(16) << first",
            "std::setw(16) << second",
        ),
        "contract": (
            'EqualsIgnoreCase(command, "RMETA")',
            "SuiteBridgeRecordingIdentity::IsValidKey(recordingKey_)",
            "SuiteBridgeRecordingMetadata::kMaxPeople",
            "SuiteBridgeRecordingMetadata::kMaxImages",
            "metadata.providerId == 0",
            "complete_ = size_ < data_.size();",
        ),
        "adapter": (
            "cGetScraperVideo request(nullptr, &recording);",
            "cPlugin *scraper = request.call();",
            "request.m_scraperVideo->getDbId() == 0",
            "impl->video = std::move(request.m_scraperVideo);",
            "video.getCharacters(true)",
            "video.getImages(",
        ),
        "command": (
            "LOCK_RECORDINGS_READ;",
            "recording->FileName()",
            "session = adapter.Start(*matchedRecording);",
            "metadata = session.Resolve(recordingKey);",
            "RecordingNotFound",
            "IdentityAmbiguous",
            "ProviderNoMatch",
        ),
        "svdrp": (
            '"RMETA <recording-key>\\n"',
            "SuiteBridgeRecordingMetadataCommand::Handle(Command, Option)",
        ),
    }
    sources = {
        "identity": identity,
        "contract": contract,
        "adapter": adapter,
        "command": command,
        "svdrp": svdrp,
    }
    for group, fragments in required.items():
        for fragment in fragments:
            if fragment not in sources[group]:
                errors.append(f"missing {group} contract: {fragment}")

    lock = command.find("LOCK_RECORDINGS_READ;")
    start = command.find("session = adapter.Start(*matchedRecording);")
    unlock = command.find("\n  }\n\n  if (matchCount == 0)", start)
    resolve = command.find("metadata = session.Resolve(recordingKey);")
    if not 0 <= lock < start < unlock < resolve:
        errors.append("invalid cRecording lock/lifetime order")
    if "statusMonitor_" in command:
        errors.append("RMETA must not alter SuiteBridge counters")

    forbidden = ("std::ifstream", "fopen(", "std::filesystem", "std::thread", "socket(")
    for fragment in forbidden:
        if fragment in identity + contract + command:
            errors.append(f"forbidden Recording metadata function: {fragment}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("suitebridge recording metadata contract ok")
