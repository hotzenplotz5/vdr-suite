#ifndef SNAPSHOT_CHANGE_FEED_ENTRY_H
#define SNAPSHOT_CHANGE_FEED_ENTRY_H

#include <string>
#include <vector>

class SnapshotChangeFeedEntry {
public:
    SnapshotChangeFeedEntry(
        int sequenceNumber,
        int snapshotGeneration,
        const std::vector<std::string>& changedDomains,
        const std::string& backendId = "default",
        const std::vector<std::string>& recordingNativeIds = {});

    int sequenceNumber() const;
    int snapshotGeneration() const;
    const std::string& backendId() const;
    const std::vector<std::string>& changedDomains() const;
    const std::vector<std::string>& recordingNativeIds() const;
    bool hasChanges() const;

private:
    int sequenceNumber_;
    int snapshotGeneration_;
    std::string backendId_;
    std::vector<std::string> changedDomains_;
    std::vector<std::string> recordingNativeIds_;
};

#endif
