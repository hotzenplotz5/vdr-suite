#include "RuntimeConfig.h"

RuntimeConfig::RuntimeConfig()
    : databasePath_("/tmp/vdr-suite-test.db")
{
}

const std::string& RuntimeConfig::databasePath() const
{
    return databasePath_;
}
