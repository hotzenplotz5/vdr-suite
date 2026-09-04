#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing recording-marks mutation file: {path}")
    return target.read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise SystemExit(f"forbidden {label}: {needle}")


header = read("vdr-plugin-suite-bridge/suitebridge_recording_marks_modify_vdr.h")
source = read("vdr-plugin-suite-bridge/suitebridge_recording_marks_modify_vdr.cpp")

for needle in (
    "ISuiteBridgeRecordingMarksModifyMutationCallback",
    "ModifyMarks(",
):
    require(header, needle, "typed VDR marks mutation callback")

for needle in (
    "LOCK_RECORDINGS_READ",
    "SuiteBridgeRecordingIdentity::KeyForNativeId",
    "recording->IsInUse() != 0",
    "request.expectedMarksRevision",
    "SuiteBridgeRecordingMarksRevision(snapshot)",
    "cIndexFile indexFile",
    "indexFile.GetClosestIFrame(requestedFrame)",
    "nativeMarks.Add(alignedFrame)",
    "nativeMarks.Del(source)",
    "source->SetPosition(alignedFrame)",
    "nativeMarks.Sort()",
    "marks.Save()",
    "cMarks::DeleteMarksFile(recording)",
    "cStatus::MsgMarksModified",
    "SuiteBridgeRecordingMarksModifyMutationDisposition::AppliedUnverified",
):
    require(source, needle, "native recording-marks mutation")

for token in (
    "system(",
    "popen(",
    "fopen(",
    "unlink(",
    "remove(",
    "RESTfulAPI",
    "restfulapi",
):
    forbid(source, token, "alternate mutation path")

if source.count("nativeMarks.Add(alignedFrame)") != 1:
    raise SystemExit("recording marks callback must contain exactly one native Add")
if source.count("nativeMarks.Del(source)") != 1:
    raise SystemExit("recording marks callback must contain exactly one native Del")

print("recording marks native VDR mutation architecture guard passed")
