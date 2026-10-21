#include "ApiRouter.h"

#include "PublicProblemDetails.h"
#include "PublicRecordingActionProviderPreflight.h"
#include "PublicResourcePreconditions.h"
#include "RecordingActionExecutionController.h"
#include "RecordingActionValidationController.h"
#include "RecordingActionValidationRequestParser.h"
#include "RecordingActionValidationResult.h"
#include "RecordingActionValidationResultJsonSerializer.h"

#include <algorithm>
#include <mutex>
#include <string>

ApiResponse ApiRouter::handlePublicRecordingAction(
    const std::string& body, bool execute, const std::string& ifMatch,
    const std::string& requestId, const std::string& correlationId)
{
    // The HTTP security gate has checked the actor, exact backend-scoped
    // recording grant and browser CSRF. Legacy routes stay independent.
    const auto problem = [execute, &requestId, &correlationId](
                            int status, const char* title,
                            const char* detail) {
        PublicProblemDetails issue;
        issue.statusCode = status;
        issue.title = title;
        issue.detail = detail;
        issue.code = status == 400 ? "invalid_request" :
                     status == 404 ? "recording_not_found" :
                     status == 412 ? "recording_revision_conflict" :
                     status == 428 ? "precondition_required" :
                     status == 503 ? "recording_service_unavailable" :
                     "recording_mutation_conflict";
        issue.requestId = requestId;
        issue.correlationId = correlationId;
        issue.instance = execute
            ? "/api/v1/recordings/actions/execute"
            : "/api/v1/recordings/actions/validate";
        ApiResponse response;
        response.statusCode = status;
        response.contentType = PublicProblemDetails::contentType();
        response.headers["Cache-Control"] = "no-store";
        response.body = issue.serialize();
        return response;
    };

    RecordingActionValidationRequestParser parser;
    RecordingActionRequest request;
    if (!parser.parsePublic(body, request))
        return problem(400, "Bad Request",
            "A valid backendId, public recordingId and action are required.");

    // Serialize Public-v1 writes within one Suite daemon. Independent VDR
    // mutations are fenced again by provider revision validation.
    std::unique_lock<std::mutex> writeLock(
        publicRecordingWriteMutex_, std::defer_lock);
    if (execute && !request.dryRun)
        writeLock.lock();

    // R3: map only an existing backend-scoped fld1_ ID to a server-side
    // logical folder using the same Recording browse cache as clients.
    // The native targetPath is never read from the public JSON.
    if (request.type == RecordingActionType::Move)
    {
        if (!publicRecordingFolderResolver_)
            return problem(503, "Service Unavailable",
                "Public recording folder resolution is unavailable.");

        const auto supplied =
            request.parameters.find("publicTargetFolderId");
        if (supplied == request.parameters.end())
            return problem(400, "Bad Request",
                "A public targetFolderId is required for MOVE.");

        std::string folderPath;
        if (!publicRecordingFolderResolver_(
                request.backendId, supplied->second, folderPath))
            return problem(404, "Not Found",
                "Target folder not available for this backend.");

        // Even a cache-derived path must never become an absolute/escaping
        // backend path through a faulty backend inventory.
        if (folderPath.size() > 2048U ||
            (!folderPath.empty() && folderPath.front() == '/') ||
            folderPath.find('\\') != std::string::npos ||
            folderPath.find('~') != std::string::npos ||
            folderPath.find("//") != std::string::npos ||
            std::any_of(folderPath.begin(), folderPath.end(),
                [](unsigned char c) { return c < 0x20 || c == 0x7f; }))
            return problem(409, "Conflict",
                "Target folder cannot be safely resolved.");
        std::size_t offset = 0;
        while (offset < folderPath.size())
        {
            const std::size_t end = folderPath.find('/', offset);
            const std::string segment = folderPath.substr(
                offset, end == std::string::npos
                    ? std::string::npos : end - offset);
            if (segment.empty() || segment == "." || segment == "..")
                return problem(409, "Conflict",
                    "Target folder cannot be safely resolved.");
            if (end == std::string::npos)
                break;
            offset = end + 1U;
        }
        // Keep the public folder token for the operation-scoped ETag.
        request.parameters["targetPath"] =
            folderPath.empty() ? "/" : folderPath;
    }

    if (!execute && !request.dryRun)
        return problem(409, "Conflict",
            "Mutation preflight is not enabled before R2.");

    if (execute && !request.dryRun)
    {
        if (ifMatch.empty())
            return problem(428, "Precondition Required",
                "A strong If-Match is required for recording mutation.");

        // Parse only: we cannot yet verify an authoritative write revision
        // using the cached recording alone. Never dispatch on a parsed tag.
        std::string revision;
        if (ifMatch.size() > 4096U ||
            !vdrsuite::http::publicStrongEntityTagResourceRevision(
                ifMatch, revision))
            return problem(400, "Bad Request",
                "If-Match must be one valid strong public entity tag.");

        // Syntax is checked before resolving the public resource; authority
        // is checked against an independent provider preview below.
    }

    if (!publicRecordingActionResolver_)
        return problem(503, "Service Unavailable",
            "Public recording identity resolution is unavailable.");

    VdrRecording recording;
    if (!publicRecordingActionResolver_(
            request.backendId, request.recordingId, recording) ||
        recording.backendId != request.backendId ||
        recording.id.empty() || recording.backendNativeId.empty() ||
        recording.title.empty())
        return problem(404, "Not Found",
            "Recording not available for this backend.");

    const std::string publicId = request.recordingId;
    request.recordingId = recording.id;
    request.parameters["backendNativeId"] = recording.backendNativeId;
    request.parameters["recordingTitle"] = recording.title;

    RecordingActionValidationResult validation =
        recordingActionValidationController_.validateResult(request);

    // Public responses contain only public IDs; legacy guard errors can
    // include backend-native diagnostics.
    validation.recordingId = publicId;
    if (!validation.valid)
        validation.errors = {"Recording action validation failed"};

    RecordingActionValidationResultJsonSerializer serializer;
    const std::string safeValidation = serializer.serialize(validation);

    std::string providerTag;
    PublicRecordingActionProviderPreflight provider;
    if (validation.valid)
    {
        RecordingActionRequest previewRequest = request;
        previewRequest.dryRun = true;
        if (PublicRecordingActionProviderPreflight::read(
                recordingActionExecutionController_.previewBackend(
                    previewRequest), provider))
        {
            providerTag = PublicRecordingActionProviderPreflight::tag(
                request, publicId, provider);
        }
    }

    // No provider-proven revision means no public write. The same preflight
    // tag covers the exact public ID, action and public target/name. The
    // SafeMutation adapter must also fence preview->validate->execute.
    if (execute && !request.dryRun)
    {
        if (providerTag.empty())
            return problem(409, "Conflict",
                "Authoritative provider preflight is unavailable.");
        if (ifMatch != providerTag)
            return problem(412, "Precondition Failed",
                "Recording action revision is stale.");
        if (!validation.valid)
            return problem(409, "Conflict",
                "Recording action validation failed.");
        if (request.type == RecordingActionType::Delete &&
            request.parameters.count("publicDeleteConfirmed") == 0U)
            return problem(400, "Bad Request",
                "Explicit confirmDelete:true is required for DELETE.");

        // Prevent a source-identity swap between the local preflight and
        // the only actual invocation of the existing mutation executor.
        VdrRecording current;
        if (!publicRecordingActionResolver_(
                request.backendId, publicId, current) ||
            current.backendId != recording.backendId ||
            current.backendNativeId != recording.backendNativeId ||
            current.id != recording.id ||
            current.title != recording.title)
            return problem(412, "Precondition Failed",
                "Recording identity changed during preflight.");

        request.parameters["publicV1Action"] = "1";
        request.parameters["publicExpectedRecordingsState"] =
            provider.recordingsState;
        request.parameters["publicExpectedTimersState"] =
            provider.timersState;

        // Same RecordingActionExecutionController and SafeMutation adapter
        // as legacy. The daemon's Public-v1 callback requires a separate
        // native inventory readback before any rec_ lifecycle update.
        const RecordingActionExecutionResult outcome =
            recordingActionExecutionController_.executeResult(request);
        if (!outcome.success)
            return problem(409, "Conflict",
                "Recording mutation was not confirmed; do not retry automatically.");
        if (!outcome.snapshotRefreshed)
            return problem(409, "Conflict",
                "Recording mutation readback is inconclusive; do not retry automatically.");

        ApiResponse committed;
        committed.statusCode = 200;
        committed.contentType = "application/json";
        committed.headers["Cache-Control"] = "no-store";
        const char* action = request.type == RecordingActionType::Rename
            ? "RENAME" : request.type == RecordingActionType::Move
                ? "MOVE" : "DELETE";
        committed.body = std::string("{\"success\":true,\"executed\":true,") +
            "\"readbackVerified\":true,\"action\":\"" + action +
            "\",\"backendId\":\"" + request.backendId +
            "\",\"recordingId\":\"" + publicId + "\"}";
        return committed;
    }

    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    if (!providerTag.empty())
        response.headers["ETag"] = providerTag;
    if (!execute)
        response.body = safeValidation;
    else
        response.body = std::string("{\"success\":") +
            (validation.valid ? "true" : "false") +
            ",\"dryRun\":true,\"executed\":false,\"validation\":" +
            safeValidation + "}";
    return response;
}
