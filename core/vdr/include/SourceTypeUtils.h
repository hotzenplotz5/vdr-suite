#pragma once

#include "SourceType.h"

#include <string>

inline std::string toString(SourceType type)
{
    switch (type)
    {
        case SourceType::LocalVdr:
            return "local-vdr";

        case SourceType::RemoteVdr:
            return "remote-vdr";

        case SourceType::RemoteSuite:
            return "remote-suite";

        case SourceType::Archive:
            return "archive";

        case SourceType::NasImport:
            return "nas-import";

        case SourceType::Mock:
            return "mock";

        default:
            return "unknown";
    }
}

inline SourceType sourceTypeFromString(const std::string& value)
{
    if (value == "local-vdr")
        return SourceType::LocalVdr;

    if (value == "remote-vdr")
        return SourceType::RemoteVdr;

    if (value == "remote-suite")
        return SourceType::RemoteSuite;

    if (value == "archive")
        return SourceType::Archive;

    if (value == "nas-import")
        return SourceType::NasImport;

    if (value == "mock")
        return SourceType::Mock;

    return SourceType::Unknown;
}
