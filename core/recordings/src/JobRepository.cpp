#include "JobRepository.h"
#include "Database.h"

#include <sqlite3.h>

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

    sqlite3_bind_int(
        stmt,
        1,
        job.recordingId);

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
        Job job;

        job.id =
            sqlite3_column_int(stmt, 0);

        job.recordingId =
            sqlite3_column_int(stmt, 1);

        const unsigned char* type =
            sqlite3_column_text(stmt, 2);

        const unsigned char* status =
            sqlite3_column_text(stmt, 3);

        if (type)
            job.jobType =
                reinterpret_cast<const char*>(type);

        if (status)
            job.status =
                reinterpret_cast<const char*>(status);

        result.push_back(job);
    }

    sqlite3_finalize(stmt);

    return result;
}
