#include "CapabilityReportJsonSerializer.h"

#include <sstream>

std::string CapabilityReportJsonSerializer::serialize(
    const CapabilityReport& report) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"backendId\":\"" << report.backendId() << "\","
        << "\"capabilities\":[";

    for (std::size_t index = 0;
         index < report.capabilities().size();
         ++index)
    {
        if (index > 0)
        {
            json << ",";
        }

        json << stateSerializer_.serialize(report.capabilities().at(index));
    }

    json << "]}";

    return json.str();
}
