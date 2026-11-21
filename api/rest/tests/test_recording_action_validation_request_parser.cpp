#include "RecordingActionValidationRequestParser.h"
#include "PublicRecordingActionProviderPreflight.h"

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


    // Public-v1 accepts the public identity only and rejects native fields,
    // duplicate keys, non-boolean dry-run and unsupported mutation types.
    RecordingActionRequest publicRequest;
    const std::string validPublicBody =
        "{\"backendId\":\"backend-a\","
        "\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"action\":\"DELETE\",\"dryRun\":true}";
    assert(parser.parsePublic(validPublicBody, publicRequest));
    assert(publicRequest.recordingId ==
        "rec_0123456789abcdef0123456789abcdef");
    assert(publicRequest.dryRun && publicRequest.parameters.empty());
    assert(!parser.parsePublic(
        "{\"backendId\":\"backend-a\",\"recordingId\":\"native\","
        "\"action\":\"DELETE\"}", publicRequest));
    assert(!parser.parsePublic(
        "{\"backendId\":\"backend-a\","
        "\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"action\":\"DELETE\",\"backendNativeId\":\"/private/native\"}",
        publicRequest));
    assert(!parser.parsePublic(
        "{\"backendId\":\"backend-a\","
        "\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"action\":\"DELETE\",\"recordingPath\":\"/private/path\"}",
        publicRequest));
    assert(!parser.parsePublic(
        "{\"backendId\":\"backend-b\",\"backendId\":\"backend-a\","
        "\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"action\":\"DELETE\"}", publicRequest));
    assert(!parser.parsePublic(
        "{\"backendId\":\"backend-a\","
        "\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"action\":\"DELETE\",\"dryRun\":\"false\"}", publicRequest));
    assert(!parser.parsePublic(
        "{\"backendId\":\"backend-a\","
        "\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"action\":\"SHRINK\"}", publicRequest));
    assert(parser.parsePublic(
        "{\"backendId\":\"backend-a\","
        "\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"action\":\"RENAME\",\"newName\":\"Test\",\"dryRun\":false}",
        publicRequest));
    assert(!publicRequest.dryRun && publicRequest.type == RecordingActionType::Rename);
    assert(!parser.parsePublic(
        "{\"backendId\":\"backend-a\","
        "\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"action\":\"DELETE\",\"newName\":\"wrong\"}", publicRequest));
    assert(!parser.parsePublic(
        "{\"backendId\":\"backend-a\","
        "\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"action\":\"MOVE\",\"targetPath\":\"/private/move\"}",
        publicRequest));
    assert(!parser.parsePublic(
        "{\"backendId\":\"backend-a\","
        "\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"action\":\"RENAME\",\"newName\":\"../escape\"}", publicRequest));

    // R3 folder references are opaque, backend-neutral inputs. Native
    // destination paths or incomplete/mismatched types never reach a backend.
    assert(parser.parsePublic(
        "{\"backendId\":\"backend-a\","
        "\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"action\":\"MOVE\","
        "\"targetFolderId\":\"fld1_0123456789abcdef0123456789abcdef\"}",
        publicRequest));
    assert(publicRequest.parameters.at("publicTargetFolderId") ==
        "fld1_0123456789abcdef0123456789abcdef");
    assert(publicRequest.parameters.count("targetPath") == 0U);
    assert(!parser.parsePublic(
        "{\"backendId\":\"backend-a\","
        "\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"action\":\"MOVE\"}", publicRequest));
    assert(!parser.parsePublic(
        "{\"backendId\":\"backend-a\","
        "\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"action\":\"MOVE\","
        "\"targetFolderId\":\"fld1_0000000000000000000000000000000X\"}",
        publicRequest));
    assert(!parser.parsePublic(
        "{\"backendId\":\"backend-a\","
        "\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"action\":\"DELETE\","
        "\"targetFolderId\":\"fld1_0123456789abcdef0123456789abcdef\"}",
        publicRequest));

    assert(parser.parsePublic(
        "{\"backendId\":\"backend-a\","
        "\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"action\":\"DELETE\",\"dryRun\":false,\"confirmDelete\":true}",
        publicRequest));
    assert(publicRequest.parameters.at("publicDeleteConfirmed") == "1");
    assert(!parser.parsePublic(
        "{\"backendId\":\"backend-a\","
        "\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"action\":\"DELETE\",\"confirmDelete\":\"true\"}",
        publicRequest));
    assert(!parser.parsePublic(
        "{\"backendId\":\"backend-a\","
        "\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"action\":\"RENAME\",\"newName\":\"X\",\"confirmDelete\":true}",
        publicRequest));

    // Authoritative SafeMutation provider preview counters become an opaque,
    // operation-scoped public ETag without any backend-native path.
    RecordingActionExecutionResult acceptedProvider;
    acceptedProvider.upstreamHttpStatus = 200;
    acceptedProvider.upstreamEndpoint = "/recordings/rename/preview.json";
    acceptedProvider.upstreamResponseBody =
        "{\"executable\":true,\"revision_recordings_state\":\"310\","
        "\"revision_timers_state\":28}";
    PublicRecordingActionProviderPreflight provider;
    assert(PublicRecordingActionProviderPreflight::read(
        acceptedProvider, provider));
    assert(provider.recordingsState == "310");
    assert(provider.timersState == "28");
    RecordingActionRequest renamePreview;
    renamePreview.backendId = "backend-a";
    renamePreview.recordingId = "native-never-expose";
    renamePreview.type = RecordingActionType::Rename;
    renamePreview.parameters["newName"] = "Broadcast";
    const std::string recPublicId =
        "rec_0123456789abcdef0123456789abcdef";
    const std::string publicTag =
        PublicRecordingActionProviderPreflight::tag(
            renamePreview, recPublicId, provider);
    assert(publicTag.rfind("\"vsr-", 0U) == 0U);
    assert(publicTag.find("native-never-expose") == std::string::npos);
    assert(vdrsuite::http::publicEvaluateIfMatch(publicTag, publicTag) ==
        vdrsuite::http::PublicEntityTagConditionResult::matched);
    provider.recordingsState = "311";
    assert(PublicRecordingActionProviderPreflight::tag(
        renamePreview, recPublicId, provider) != publicTag);
    provider.recordingsState = "310";
    renamePreview.parameters["newName"] = "Other";
    assert(PublicRecordingActionProviderPreflight::tag(
        renamePreview, recPublicId, provider) != publicTag);
    acceptedProvider.upstreamResponseBody =
        "{\"executable\":false,\"revision_recordings_state\":310,"
        "\"revision_timers_state\":28}";
    assert(!PublicRecordingActionProviderPreflight::read(
        acceptedProvider, provider));
    acceptedProvider.upstreamResponseBody =
        "{\"executable\":true,\"revision_recordings_state\":310,"
        "\"revision_recordings_state\":311,\"revision_timers_state\":28}";
    assert(!PublicRecordingActionProviderPreflight::read(
        acceptedProvider, provider));
    acceptedProvider.upstreamHttpStatus = 500;
    assert(!PublicRecordingActionProviderPreflight::read(
        acceptedProvider, provider));

    std::cout
        << "test_recording_action_validation_request_parser passed"
        << std::endl;

    return 0;
}
