#pragma once

#include "DashboardController.h"
#include "Database.h"

#include <mutex>
#include <string>

class RecordingSeriesHierarchyApiRuntime
{
public:
    static RecordingSeriesHierarchyApiRuntime& instance();

    bool configure(Database& database);
    bool configured() const;
    void reset();

    bool tryHandleGet(
        const std::string& requestTarget,
        ApiResponse& response) const;

    bool tryHandlePost(
        const std::string& requestTarget,
        const std::string& body,
        const std::string& actorRef,
        ApiResponse& response) const;

private:
    RecordingSeriesHierarchyApiRuntime() = default;

    Database* database() const;

    mutable std::mutex mutex_;
    Database* database_ = nullptr;
};
