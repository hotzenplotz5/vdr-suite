#include "RuntimeConfig.h"

RuntimeConfig::RuntimeConfig()
    : databasePath_("/tmp/vdr-suite-test.db"),
      vdrMode_("restfulapi"),
      vdrHost_("127.0.0.1"),
      vdrPort_(8002),
      httpListenHost_("0.0.0.0"),
      httpListenPort_(18080)
{
}

const std::string& RuntimeConfig::databasePath() const
{
    return databasePath_;
}

const std::string& RuntimeConfig::vdrMode() const
{
    return vdrMode_;
}

const std::string& RuntimeConfig::vdrHost() const
{
    return vdrHost_;
}

int RuntimeConfig::vdrPort() const
{
    return vdrPort_;
}

const std::string& RuntimeConfig::httpListenHost() const
{
    return httpListenHost_;
}

int RuntimeConfig::httpListenPort() const
{
    return httpListenPort_;
}
