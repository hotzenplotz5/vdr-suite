#include "RuntimeConfig.h"

RuntimeConfig::RuntimeConfig()
    : databasePath_("/var/lib/vdr-suite/vdr-suite.db")
{
}

const std::string& RuntimeConfig::databasePath() const
{
    return databasePath_;
}
