#include "BackendRegistryJsonSerializer.h"

#include <sstream>

static const char* backendBoolToJson(
    bool value)
{
    return value ? "true" : "false";
}

std::string BackendRegistryJsonSerializer::serializeBackend(
    const BackendNode& backend) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"backendId\":\"" << backend.backendId << "\","
        << "\"backendName\":\"" << backend.backendName << "\","
        << "\"backendType\":\"" << backend.backendType << "\","
        << "\"accessMode\":\"" << backend.accessMode << "\","
        << "\"readOnly\":" << backendBoolToJson(backend.readOnly()) << ","
        << "\"canWrite\":" << backendBoolToJson(!backend.readOnly()) << ","
        << "\"canWriteRecordings\":" << backendBoolToJson(!backend.readOnly()) << ","
        << "\"canWriteTimers\":" << backendBoolToJson(!backend.readOnly()) << ","
        << "\"canWriteSearchTimers\":" << backendBoolToJson(!backend.readOnly()) << ","
        << "\"enabled\":" << backendBoolToJson(backend.enabled) << ","
        << "\"online\":" << backendBoolToJson(backend.online)
        << "}";

    return json.str();
}

std::string BackendRegistryJsonSerializer::serializeBackends(
    const std::vector<BackendNode>& backends) const
{
    std::ostringstream json;

    json << "{\"backends\":[";

    for (std::size_t index = 0;
         index < backends.size();
         ++index)
    {
        if (index > 0)
        {
            json << ",";
        }

        json << serializeBackend(backends[index]);
    }

    json << "]}";

    return json.str();
}
