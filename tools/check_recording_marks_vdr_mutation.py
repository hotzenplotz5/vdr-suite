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
protocol_header = read("vdr-plugin-suite-bridge/suitebridge_recording_marks_modify.h")
protocol_source = read("vdr-plugin-suite-bridge/suitebridge_recording_marks_modify.cpp")
source = read("vdr-plugin-suite-bridge/suitebridge_recording_marks_modify_vdr.cpp")
plugin_header = read("vdr-plugin-suite-bridge/suitebridge.h")
plugin_source = read("vdr-plugin-suite-bridge/suitebridge.cpp")
svdrp_source = read("vdr-plugin-suite-bridge/suitebridge_svdrp.cpp")
plugin_makefile = read("vdr-plugin-suite-bridge/Makefile")

for needle in (
    "ISuiteBridgeRecordingMarksModifyMutationCallback",
    "ModifyMarks(",
):
    require(header, needle, "typed VDR marks mutation callback")

for needle in (
    "  Reset,",
    "  Replace,",
    "std::vector<int> replacementFrames",
):
    require(protocol_header, needle, "complete native marks mutation protocol")

for needle in (
    'values[10] == "add"',
    'values[10] == "delete"',
    'values[10] == "move"',
    'values[10] == "reset"',
    'values[10] == "replace"',
    "replacementFramesValue(",
    'values[3] != "2"',
    'values[1] != "2"',
):
    require(protocol_source, needle, "bounded native marks mutation protocol v2")

for needle in (
    "LOCK_RECORDINGS_READ",
    "SuiteBridgeRecordingIdentity::KeyForNativeId",
    "recording->IsInUse() != 0",
    "current.inUseFlags != 0 || recording->IsInUse() != 0",
    "request.expectedMarksRevision",
    "SuiteBridgeRecordingMarksRevision(snapshot)",
    "cIndexFile indexFile",
    "indexFile.GetClosestIFrame(requestedFrame)",
    "nativeMarks.Add(alignedFrame)",
    "nativeMarks.Del(source)",
    "source->SetPosition(alignedFrame)",
    "nativeMarks.Sort()",
    "cMarks::DeleteMarksFile(recording)",
    "SuiteBridgeRecordingMarksModifyKind::Replace",
    "std::adjacent_find(alignedFrames.begin(), alignedFrames.end())",
    "while (cMark *mark = nativeMarks.First()) nativeMarks.Del(mark)",
    "for (const int frame : alignedFrames) nativeMarks.Add(frame)",
    "marks.Save()",
    "cStatus::MsgMarksModified",
    "SuiteBridgeRecordingMarksModifyMutationDisposition::AppliedUnverified",
    "appliedWithNativeReadback(",
    "currentFrames(readbackMarks) != expectedFrames",
    "readback.marksRevision == request.expectedMarksRevision",
    "nmarks:vdr:postrev:",
):
    require(source, needle, "native recording-marks mutation")

for needle in (
    '#include "suitebridge_recording_marks_modify.h"',
    '#include "suitebridge_recording_marks_modify_vdr.h"',
    "SuiteBridgeRecordingMarksModifyVdrMutationCallback recordingMarksModifyVdrMutation_",
    "SuiteBridgeRecordingMarksModifyService recordingMarksModify_",
):
    require(plugin_header, needle, "private recording-marks mutation owner")

for needle in (
    "recordingMarksModify_(",
    "&recordingMarksModifyVdrMutation_",
    "native-operation=vdr.recording.marks.modify",
    "mutations=enabled execution=enabled",
):
    require(plugin_source, needle, "recording-marks mutation activation")

require(
    svdrp_source,
    "recordingMarksModify_.Handle(Command, Option)",
    "private NMARKS dispatch",
)
for needle in (
    "suitebridge_recording_marks_modify.o",
    "suitebridge_recording_marks_modify_vdr.o",
):
    require(plugin_makefile, needle, "recording-marks mutation plugin object")

help_start = svdrp_source.find("const char **cPluginSuiteBridge::SVDRPHelpPages")
command_start = svdrp_source.find("cString cPluginSuiteBridge::SVDRPCommand")
if help_start < 0 or command_start <= help_start:
    raise SystemExit("unable to isolate SuiteBridge SVDRP public help block")
if "NMARKS" in svdrp_source[help_start:command_start]:
    raise SystemExit("private NMARKS command leaked into public SVDRP help")

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

if source.count("marks.Save()") != 1:
    raise SystemExit("recording marks callback must centralize marks Save")

print("recording marks native VDR mutation architecture guard passed")
