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
    MediaSessionWorkspace(MediaSessionWorkspace&&) = default;
    MediaSessionWorkspace& operator=(MediaSessionWorkspace&&) = default;

    MediaSessionWorkspaceResult prepare(
        const std::string& workspaceId,
        const std::vector<std::string>& sourceSegments,
        const std::vector<double>& segmentDurationsSeconds = {});

    MediaSessionWorkspaceResult activateSeekTimeline(
        const std::vector<std::string>& sourceSegments,
        const std::vector<double>& segmentDurationsSeconds);

    MediaSessionWorkspaceResult prepareLive(
        const std::string& workspaceId);

    void cleanup();

    const std::string& directory() const;
    std::string concatPath() const;
    std::string logPath() const;
    std::string liveStreamPath() const;

private:
    std::string rootDirectory_;
    std::string directory_;

    MediaSessionWorkspaceResult prepareDirectory(
        const std::string& workspaceId);
};
