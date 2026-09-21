#include "JobRepository.h"
#include "Database.h"

#include <sqlite3.h>

namespace
{
    std::string columnText(sqlite3_stmt* stmt, int column)
    {
        const unsigned char* text =
            sqlite3_column_text(stmt, column);

        if (!text)
        {
            return {};
        }

        return reinterpret_cast<const char*>(text);
    }

    Job readJob(sqlite3_stmt* stmt)
    {
        Job job;

        job.id = sqlite3_column_int(stmt, 0);
        job.recordingId = sqlite3_column_int(stmt, 1);
        job.jobType = columnText(stmt, 2);
        job.status = columnText(stmt, 3);

        return job;
    }
}

JobRepository::JobRepository(Database& database)
    : database_(database)
{
}

bool JobRepository::insertJob(const Job& job)
{
    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "INSERT INTO jobs "
        "(recording_id, job_type, status) "
        "VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, job.recordingId);

    sqlite3_bind_text(
        stmt,
        2,
        job.jobType.c_str(),
        -1,
        SQLITE_TRANSIENT);

    sqlite3_bind_text(
        stmt,
        3,
        job.status.c_str(),
        -1,
        SQLITE_TRANSIENT);

    bool success =
        sqlite3_step(stmt) == SQLITE_DONE;

    sqlite3_finalize(stmt);

    return success;
}

std::vector<Job> JobRepository::getAllJobs()
{
    std::vector<Job> result;

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT id, recording_id, job_type, status "
        "FROM jobs "
        "ORDER BY id;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return result;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result.push_back(readJob(stmt));
    }

    sqlite3_finalize(stmt);

    return result;
}

bool JobRepository::updateJobStatus(
    int jobId,
    const std::string& status)
{
    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "UPDATE jobs "
        "SET status = ? "
        "WHERE id = ?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(
        stmt,
        1,
        status.c_str(),
        -1,
        SQLITE_TRANSIENT);

    sqlite3_bind_int(stmt, 2, jobId);

    bool success =
        sqlite3_step(stmt) == SQLITE_DONE;

    sqlite3_finalize(stmt);

    return success;
}

Job JobRepository::getNextPendingJob()
{
    Job job;

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT id, recording_id, job_type, status "
        "FROM jobs "
        "WHERE status = 'PENDING' "
        "ORDER BY id "
        "LIMIT 1;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return job;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        job = readJob(stmt);
    }

    sqlite3_finalize(stmt);

    return job;
}
