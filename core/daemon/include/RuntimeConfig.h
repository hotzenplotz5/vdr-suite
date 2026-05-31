#pragma once

#include <string>

class RuntimeConfig
{
public:
    RuntimeConfig();

    const std::string& databasePath() const;

private:
    std::string databasePath_;
};
