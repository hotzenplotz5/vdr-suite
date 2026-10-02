#include "RecordingActionValidationRequestParser.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    RecordingActionValidationRequestParser parser;

    RecordingActionRequest moveRequest =
        parser.parse(
            "{"
            "\"backendId\":\"default\","
            "\"recordingId\":\"recording-001\","
            "\"action\":\"MOVE\","
            "\"dryRun\":true,"
            "\"targetPath\":\"/srv/vdr/video/archive\","
            "\"recordingPath\":\"Movies/Tatort/2026-06-16.20.15.1-0.rec\","
            "\"recordingTitle\":\"Movies/Tatort am Abend\""
            "}");

    assert(moveRequest.backendId == "default");
    assert(moveRequest.recordingId == "recording-001");
    assert(moveRequest.type == RecordingActionType::Move);
    assert(moveRequest.dryRun);
    assert(moveRequest.parameters.at("targetPath") == "/srv/vdr/video/archive");
    assert(
        moveRequest.parameters.at("recordingPath") ==
        "Movies/Tatort/2026-06-16.20.15.1-0.rec");
    assert(
        moveRequest.parameters.at("recordingTitle") ==
        "Movies/Tatort am Abend");

    RecordingActionRequest browserOrderMoveRequest =
        parser.parse(
            "{"
            "\"backendId\":\"default\","
            "\"recordingId\":\"recording-005\","
            "\"action\":\"MOVE\","
            "\"dryRun\":true,"
            "\"recordingPath\":\"Movies/Tatort/2026-06-16.20.15.1-0.rec\","
            "\"backendNativeId\":\"native-005\","
            "\"recordingTitle\":\"Tatort, Folge \\\"Nacht\\\"\","
            "\"targetPath\":\"VDRSuite_Testordner\""
            "}");

    assert(browserOrderMoveRequest.backendId == "default");
    assert(browserOrderMoveRequest.recordingId == "recording-005");
    assert(browserOrderMoveRequest.type == RecordingActionType::Move);
    assert(browserOrderMoveRequest.dryRun);
    assert(
        browserOrderMoveRequest.parameters.at("recordingTitle") ==
        "Tatort, Folge \"Nacht\"");
    assert(
        browserOrderMoveRequest.parameters.at("targetPath") ==
        "VDRSuite_Testordner");

    RecordingActionRequest renameRequest =
        parser.parse(
            "{"
            "\"backendId\":\"default\","
            "\"recordingId\":\"recording-002\","
            "\"action\":\"RENAME\","
            "\"dryRun\":false,"
            "\"newName\":\"Evening News\""
            "}");

    assert(renameRequest.backendId == "default");
    assert(renameRequest.recordingId == "recording-002");
    assert(renameRequest.type == RecordingActionType::Rename);
    assert(!renameRequest.dryRun);
    assert(renameRequest.parameters.at("newName") == "Evening News");

    RecordingActionRequest deleteRequest =
        parser.parse(
            "{"
            "\"backendId\":\"default\","
            "\"recordingId\":\"recording-003\","
            "\"action\":\"DELETE\""
            "}");

    assert(deleteRequest.backendId == "default");
    assert(deleteRequest.recordingId == "recording-003");
    assert(deleteRequest.type == RecordingActionType::Delete);
    assert(deleteRequest.dryRun);

    RecordingActionRequest unknownRequest =
        parser.parse(
            "{"
            "\"backendId\":\"default\","
            "\"recordingId\":\"recording-004\","
            "\"action\":\"DOES_NOT_EXIST\""
            "}");

    assert(unknownRequest.type == RecordingActionType::Unknown);

    RecordingActionRequest missingFieldsRequest =
        parser.parse("{}");

    assert(missingFieldsRequest.backendId.empty());
    assert(missingFieldsRequest.recordingId.empty());
    assert(missingFieldsRequest.type == RecordingActionType::Unknown);
    assert(missingFieldsRequest.dryRun);

    std::cout
        << "test_recording_action_validation_request_parser passed"
        << std::endl;

    return 0;
}
