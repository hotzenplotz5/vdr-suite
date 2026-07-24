#pragma once

#include <sqlite3.h>

#include <atomic>

class DaemonSqliteShutdownCancellation final
{
public:
    explicit DaemonSqliteShutdownCancellation(sqlite3* database) noexcept
        : database_(database)
    {
        if (database_ != nullptr)
        {
            sqlite3_progress_handler(
                database_,
                100,
                &DaemonSqliteShutdownCancellation::cancelOnce,
                this);
        }
    }

    ~DaemonSqliteShutdownCancellation()
    {
        if (database_ != nullptr)
        {
            sqlite3_progress_handler(database_, 0, nullptr, nullptr);
        }
    }

    DaemonSqliteShutdownCancellation(
        const DaemonSqliteShutdownCancellation&) = delete;
    DaemonSqliteShutdownCancellation& operator=(
        const DaemonSqliteShutdownCancellation&) = delete;

    bool cancellationDelivered() const noexcept
    {
        return cancellationDelivered_.load();
    }

private:
    static int cancelOnce(void* context) noexcept
    {
        auto* cancellation =
            static_cast<DaemonSqliteShutdownCancellation*>(context);
        if (cancellation == nullptr)
        {
            return 0;
        }

        bool expected = false;
        return cancellation->cancellationDelivered_.compare_exchange_strong(
                   expected,
                   true)
            ? 1
            : 0;
    }

    sqlite3* database_ = nullptr;
    std::atomic<bool> cancellationDelivered_{false};
};
