#ifndef LIVE_UPDATE_EVENT_H
#define LIVE_UPDATE_EVENT_H

#include "SnapshotChangeFeedEntry.h"

#include <string>
#include <vector>

class LiveUpdateEvent {
public:
    LiveUpdateEvent(
        int sequenceNumber,
        int snapshotGeneration,
        const std::vector<std::string>& changedDomains,
        const std::string& backendId = "default");

    explicit LiveUpdateEvent(
        const SnapshotChangeFeedEntry& entry);

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
