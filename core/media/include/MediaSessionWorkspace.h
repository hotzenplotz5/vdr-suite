#pragma once

#include <string>
#include <vector>

struct MediaSessionWorkspaceResult
{
    bool ready = false;
    std::string reasonCode;
};

class MediaSessionWorkspace
{
public:
    explicit MediaSessionWorkspace(std::string rootDirectory);
    ~MediaSessionWorkspace();

    MediaSessionWorkspace(const MediaSessionWorkspace&) = delete;
    MediaSessionWorkspace& operator=(const MediaSessionWorkspace&) = delete;

    MediaSessionWorkspaceResult prepare(
        const std::string& workspaceId,
        const std::vector<std::string>& sourceSegments);

    void cleanup();

    const std::string& directory() const;
    std::string concatPath() const;
    std::string logPath() const;

private:
    std::string rootDirectory_;
    std::string directory_;
};
