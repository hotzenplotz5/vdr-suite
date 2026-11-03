#pragma once

#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

// The existing backend-scoped presentation change handoff can carry exact
// native Recording identities when the mutation already knows them. A broad
// hint wins over narrow hints: never advertise incomplete scope as complete.
class RecordingPresentationChangeQueue final
{
public:
    struct Change {
        std::string backendId;
        std::vector<std::string> recordingNativeIds;
    };

    void request(const std::string& backendId, const std::string& nativeId = "")
    {
        if (backendId.empty()) return;
        std::lock_guard<std::mutex> lock(mutex_);
        auto& entry = pending_[backendId];
        if (nativeId.empty()) {
            entry.unscoped = true;
            entry.nativeIds.clear();
        } else if (!entry.unscoped) {
            entry.nativeIds.insert(nativeId);
        }
    }

    std::vector<Change> takePendingChanges()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<Change> result;
        for (const auto& item : pending_) {
            Change change;
            change.backendId = item.first;
            if (!item.second.unscoped) {
                change.recordingNativeIds.assign(
                    item.second.nativeIds.begin(), item.second.nativeIds.end());
            }
            result.push_back(std::move(change));
        }
        pending_.clear();
        return result;
    }

    // Keep existing backend-only consumers and their tests compatible.
    std::set<std::string> takePending()
    {
        std::set<std::string> result;
        for (const auto& change : takePendingChanges())
            result.insert(change.backendId);
        return result;
    }

private:
    struct Pending {
        bool unscoped = false;
        std::set<std::string> nativeIds;
    };

    std::mutex mutex_;
    std::map<std::string, Pending> pending_;
};
