#include "RecordingActionSafetyResultJsonSerializer.h"

#include <cassert>
#include <string>

int main()
{
    RecordingActionSafetyResultJsonSerializer serializer;

    {
        RecordingActionSafetyResult result =
            RecordingActionSafetyResult::allowed(true);

        const std::string json = serializer.serialize(result);

        assert(json.find("\"canExecute\":true") != std::string::npos);
        assert(json.find("\"dryRun\":true") != std::string::npos);
        assert(json.find("\"readOnlyBlocked\":false") != std::string::npos);
        assert(json.find("\"executionDisabled\":false") != std::string::npos);
        assert(json.find("\"backendUnavailable\":false") != std::string::npos);
        assert(json.find("\"recordingInUse\":false") != std::string::npos);
        assert(json.find("\"missingCapability\":false") != std::string::npos);
        assert(json.find("\"unsupportedAction\":false") != std::string::npos);
        assert(json.find("\"blockers\":[]") != std::string::npos);
        assert(json.find("\"warnings\":[\"dry-run only\"]") != std::string::npos);
    }

    {
        RecordingActionSafetyResult result =
            RecordingActionSafetyResult::blocked(
                false,
                "recording is currently in use");

        result.recordingInUse = true;
        result.readOnlyBlocked = true;
        result.blockers.push_back(
            "recording action execution is blocked by read-only backend config");
        result.warnings.push_back(
            "message with \"quotes\" and backslash \\");

        const std::string json = serializer.serialize(result);

        assert(json.find("\"canExecute\":false") != std::string::npos);
        assert(json.find("\"dryRun\":false") != std::string::npos);
        assert(json.find("\"readOnlyBlocked\":true") != std::string::npos);
        assert(json.find("\"recordingInUse\":true") != std::string::npos);
        assert(json.find("\"blockers\":[\"recording is currently in use\",\"recording action execution is blocked by read-only backend config\"]")
               != std::string::npos);
        assert(json.find("message with \\\"quotes\\\" and backslash \\\\")
               != std::string::npos);
    }

    return 0;
}
