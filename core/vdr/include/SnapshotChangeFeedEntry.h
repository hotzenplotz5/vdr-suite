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
        const std::string& backendId = "default");

    int sequenceNumber() const;
    int snapshotGeneration() const;
    const std::string& backendId() const;
    const std::vector<std::string>& changedDomains() const;
    bool hasChanges() const;

private:
    int sequenceNumber_;
    int snapshotGeneration_;
    std::string backendId_;
    std::vector<std::string> changedDomains_;
};

#endif
