#pragma once

#include <string>
#include <vector>

class SearchTimerDiscoveryExtendedEpgInfo {
public:
    static SearchTimerDiscoveryExtendedEpgInfo create(
        int id,
        const std::string& name,
        const std::vector<std::string>& values,
        const std::string& config)
    {
        SearchTimerDiscoveryExtendedEpgInfo info;
        info.id_ = id;
        info.name_ = name;
        info.values_ = values;
        info.config_ = config;
        return info;
    }

    int id() const
    {
        return id_;
    }

    const std::string& name() const
    {
        return name_;
    }

    const std::vector<std::string>& values() const
    {
        return values_;
    }

    const std::string& config() const
    {
        return config_;
    }

    bool hasValues() const
    {
        return !values_.empty();
    }

    bool hasConfig() const
    {
        return !config_.empty();
    }

private:
    int id_ = 0;
    std::string name_;
    std::vector<std::string> values_;
    std::string config_;
};

class SearchTimerDiscoveryChannelGroup {
public:
    static SearchTimerDiscoveryChannelGroup create(
        const std::string& name)
    {
        SearchTimerDiscoveryChannelGroup group;
        group.name_ = name;
        return group;
    }

    const std::string& name() const
    {
        return name_;
    }

    bool isValid() const
    {
        return !name_.empty();
    }

private:
    std::string name_;
};

class SearchTimerDiscoveryBlacklist {
public:
    static SearchTimerDiscoveryBlacklist create(
        int id,
        const std::string& search)
    {
        SearchTimerDiscoveryBlacklist blacklist;
        blacklist.id_ = id;
        blacklist.search_ = search;
        return blacklist;
    }

    int id() const
    {
        return id_;
    }

    const std::string& search() const
    {
        return search_;
    }

    bool isValid() const
    {
        return id_ >= 0 && !search_.empty();
    }

private:
    int id_ = 0;
    std::string search_;
};

class SearchTimerDiscoveryRecordingDirectory {
public:
    static SearchTimerDiscoveryRecordingDirectory create(
        const std::string& path)
    {
        SearchTimerDiscoveryRecordingDirectory directory;
        directory.path_ = path;
        return directory;
    }

    const std::string& path() const
    {
        return path_;
    }

    bool isValid() const
    {
        return !path_.empty();
    }

private:
    std::string path_;
};

class SearchTimerDiscoveryCatalog {
public:
    void setBackendId(
        const std::string& backendId)
    {
        backendId_ = backendId;
    }

    const std::string& backendId() const
    {
        return backendId_;
    }

    void addExtendedEpgInfo(
        const SearchTimerDiscoveryExtendedEpgInfo& info)
    {
        extendedEpgInfos_.push_back(info);
    }

    void addChannelGroup(
        const SearchTimerDiscoveryChannelGroup& group)
    {
        channelGroups_.push_back(group);
    }

    void addBlacklist(
        const SearchTimerDiscoveryBlacklist& blacklist)
    {
        blacklists_.push_back(blacklist);
    }

    void addRecordingDirectory(
        const SearchTimerDiscoveryRecordingDirectory& directory)
    {
        recordingDirectories_.push_back(directory);
    }

    const std::vector<SearchTimerDiscoveryExtendedEpgInfo>& extendedEpgInfos() const
    {
        return extendedEpgInfos_;
    }

    const std::vector<SearchTimerDiscoveryChannelGroup>& channelGroups() const
    {
        return channelGroups_;
    }

    const std::vector<SearchTimerDiscoveryBlacklist>& blacklists() const
    {
        return blacklists_;
    }

    const std::vector<SearchTimerDiscoveryRecordingDirectory>& recordingDirectories() const
    {
        return recordingDirectories_;
    }

    bool empty() const
    {
        return extendedEpgInfos_.empty()
            && channelGroups_.empty()
            && blacklists_.empty()
            && recordingDirectories_.empty();
    }

    int extendedEpgInfoCount() const
    {
        return static_cast<int>(extendedEpgInfos_.size());
    }

    int channelGroupCount() const
    {
        return static_cast<int>(channelGroups_.size());
    }

    int blacklistCount() const
    {
        return static_cast<int>(blacklists_.size());
    }

    int recordingDirectoryCount() const
    {
        return static_cast<int>(recordingDirectories_.size());
    }

private:
    std::string backendId_;
    std::vector<SearchTimerDiscoveryExtendedEpgInfo> extendedEpgInfos_;
    std::vector<SearchTimerDiscoveryChannelGroup> channelGroups_;
    std::vector<SearchTimerDiscoveryBlacklist> blacklists_;
    std::vector<SearchTimerDiscoveryRecordingDirectory> recordingDirectories_;
};
