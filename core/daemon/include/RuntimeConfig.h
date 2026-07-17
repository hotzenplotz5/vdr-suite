#pragma once

#include <map>
#include <string>

class RuntimeConfig
{
public:
    RuntimeConfig();

    const std::string& databasePath() const;
    const std::string& vdrMode() const;
    const std::string& vdrHost() const;
    int vdrPort() const;
    const std::string& httpListenHost() const;
    int httpListenPort() const;
    const std::map<std::string, std::string>& recordingArtworkRoots() const;

private:
    std::string databasePath_;
    std::string vdrMode_;
    std::string vdrHost_;
    int vdrPort_;
    std::string httpListenHost_;
    int httpListenPort_;
    std::map<std::string, std::string> recordingArtworkRoots_;
};
