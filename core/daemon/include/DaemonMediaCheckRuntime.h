#pragma once

#include "MediaCheck.h"
#include "MediaCheckFfmpeg.h"
#include "MediaCheckApiRuntime.h"
#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <thread>

// A single bounded, read-only, backend-local Media Check. No generic worker
// platform, filesystem mutation, browser-supplied paths or CLI shell commands.
// Until Agent transport is integrated this is ONLY enabled for explicit local
// backend video-root mounts. Core SQLite stores reports, not a second job queue.
class DaemonMediaCheckRuntime final
{
public:
    using ResolveRecording = std::function<std::optional<std::filesystem::path>(
        const std::string&, const std::string&)>;
    using BackendAvailable = std::function<bool(const std::string&)>;

    DaemonMediaCheckRuntime(
        std::map<std::string, std::string> videoRoots,
        ResolveRecording resolve, BackendAvailable available,
        Database& database)
        : videoRoots_(std::move(videoRoots)),
          resolve_(std::move(resolve)),
          available_(std::move(available)), database_(database) {}

    ~DaemonMediaCheckRuntime()
    {
        if (worker_.joinable()) worker_.join();
    }

    DaemonMediaCheckRuntime(const DaemonMediaCheckRuntime&) = delete;
    DaemonMediaCheckRuntime& operator=(const DaemonMediaCheckRuntime&) = delete;

    bool ensurePersistence()
    {
        auto lease = database_.acquireTransactionLease();
        // This table is a reporting read model in the existing Suite Core
        // SQLite file. It never reserves or executes Agent operations.
        return database_.execute(
            "CREATE TABLE IF NOT EXISTS media_check_reports ("
            "check_id TEXT PRIMARY KEY,actor_id TEXT NOT NULL,"
            "backend_id TEXT NOT NULL,recording_id TEXT NOT NULL,"
            "mode TEXT NOT NULL,state TEXT NOT NULL,"
            "report TEXT NOT NULL,created_at INTEGER NOT NULL,"
            "CHECK(state IN ('running','completed')));") &&
            database_.execute(
            "UPDATE media_check_reports SET state='completed',"
            "report='{\"schemaVersion\":1,\"status\":\"incomplete\","
            "\"readOnly\":true,\"diagnostic\":"
            "\"Media Check interrupted by daemon restart.\"}'"
            " WHERE state='running';");
    }

    ApiResponse capability(const std::string& backend) const
    {
        const bool enabled = videoRoots_.count(backend) &&
            available_ && available_(backend);
        return MediaCheckApiRuntime::json(200,
            "{\"backendId\":\"" + backend +
            "\",\"capability\":\"addons.media.check\",\"available\":" +
            (enabled ? "true" : "false") + "}");
    }

    ApiResponse start(const std::string& actor, const std::string& backend,
                      const std::string& publicId, const std::string& mode)
    {
        const auto root = videoRoots_.find(backend);
        if (root == videoRoots_.end() || !available_ ||
            !available_(backend))
            return MediaCheckApiRuntime::problem(503,
                "Media Check is not available on this local backend.");
        if (!resolve_)
            return MediaCheckApiRuntime::problem(503,
                "Recording resolver is not available.");
        const auto recording = resolve_(backend, publicId);
        if (!recording)
            return MediaCheckApiRuntime::problem(404,
                "Recording not found in the current backend snapshot.");

        namespace fs = std::filesystem;
        const fs::path videoRoot(root->second);
        const fs::path absolute(*recording);
        // Never trust a browser pathname. The root and recording are supplied
        // only by administrator configuration and the live VDR snapshot.
        if (!videoRoot.is_absolute() ||
            videoRoot.lexically_normal() != videoRoot ||
            !absolute.is_absolute() ||
            absolute.lexically_normal() != absolute)
            return MediaCheckApiRuntime::problem(503,
                "Backend recording location is not valid.");
        const fs::path relative = absolute.lexically_relative(videoRoot);
        if (relative.empty() || relative.is_absolute() ||
            relative.extension() != ".rec" ||
            std::any_of(relative.begin(), relative.end(),
                [](const fs::path& p) { return p == "." || p == ".."; }))
            return MediaCheckApiRuntime::problem(503,
                "Recording is outside the configured local video root.");

        std::lock_guard<std::mutex> lock(mutex_);
        if (job_.running)
            return MediaCheckApiRuntime::problem(409,
                "A Media Check is already running.");
        // Completed thread no longer needs the mutex, so join is bounded.
        if (worker_.joinable()) worker_.join();

        std::string id;
        try {
            std::random_device random;
            constexpr char hex[] = "0123456789abcdef";
            id = "chk_";
            for (int i = 0; i != 16; ++i) {
                const unsigned char byte = static_cast<unsigned char>(random());
                id += hex[byte >> 4];
                id += hex[byte & 15];
            }
            job_ = {id, actor, backend, publicId, mode, true, std::string()};
            if (!storeJob(job_)) {
                job_ = {};
                return MediaCheckApiRuntime::problem(503,
                    "Media Check persistent store unavailable.");
            }
            worker_ = std::thread([this, videoRoot, relative, mode, publicId]() {
                std::string serialized;
                try {
                    vdrsuite::mediatools::MediaCheckReport report;
                    vdrsuite::mediatools::FfprobeMediaCheckProber prober;
                    if (mode == "decode") {
                        vdrsuite::mediatools::FfmpegMediaCheckDecoder decoder;
                        report = vdrsuite::mediatools::checkRecordingWithProber(
                            videoRoot, relative, prober, &decoder);
                    } else {
                        report = vdrsuite::mediatools::checkRecordingWithProber(
                            videoRoot, relative, prober);
                    }
                    // Public reports expose the opaque identity, never a path.
                    report.recording = publicId;
                    serialized = report.json();
                } catch (...) {
                    serialized =
                        "{\"schemaVersion\":1,\"status\":\"incomplete\","
                        "\"readOnly\":true,\"diagnostic\":"
                        "\"The recording could not be fully inspected.\"}";
                }
                std::lock_guard<std::mutex> update(mutex_);
                Job completed = job_;
                completed.report = std::move(serialized);
                completed.running = false;
                if (!storeJob(completed)) {
                    completed.report =
                        "{\"schemaVersion\":1,\"status\":\"incomplete\","
                        "\"readOnly\":true,\"diagnostic\":"
                        "\"Cannot persist Media Check result.\"}";
                }
                job_ = std::move(completed);
            });
        } catch (...) {
            // Thread-start failures are recorded as incomplete on the next
            // daemon boot; never report a false completed success.
            Job failed = job_;
            failed.running = false;
            failed.report =
                "{\"schemaVersion\":1,\"status\":\"incomplete\","
                "\"readOnly\":true,\"diagnostic\":"
                "\"Cannot start Media Check worker.\"}";
            (void)storeJob(failed);
            job_ = {};
            return MediaCheckApiRuntime::problem(503,
                "Cannot start Media Check.");
        }
        return MediaCheckApiRuntime::json(202, snapshotJson(job_));
    }

    ApiResponse status(const std::string& actor, const std::string& backend,
                       const std::string& checkId) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (job_.id == checkId && job_.actor == actor &&
            job_.backend == backend)
            return MediaCheckApiRuntime::json(200, snapshotJson(job_));
        // Re-read only completed reports for this exact actor and backend;
        // the API gate has independently rechecked the current playback right.
        const auto saved = readCompleted(actor, backend, checkId);
        if (!saved)
            return MediaCheckApiRuntime::problem(404,
                "Media Check not found.");
        return MediaCheckApiRuntime::json(200, snapshotJson(*saved));
    }

private:
    struct Job {
        std::string id, actor, backend, recordingId, mode;
        bool running = false;
        std::string report;
    };
    const std::map<std::string, std::string> videoRoots_;
    const ResolveRecording resolve_;
    const BackendAvailable available_;
    Database& database_;
    mutable std::mutex mutex_;
    Job job_;
    std::thread worker_;

    bool storeJob(const Job& job) const
    {
        auto lease = database_.acquireTransactionLease();
        sqlite3_stmt* statement = nullptr;
        const char* sql = job.running
            ? "INSERT INTO media_check_reports(check_id,actor_id,backend_id,"
              "recording_id,mode,state,report,created_at)"
              " VALUES(?,?,?,?,?,'running','',strftime('%s','now'));"
            : "UPDATE media_check_reports SET state='completed',report=?"
              " WHERE check_id=? AND actor_id=? AND backend_id=? AND state='running';";
        if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement,
                               nullptr) != SQLITE_OK)
            return false;
        const auto bind = [&](int index, const std::string& value) {
            return sqlite3_bind_text(statement, index, value.c_str(),
                                      -1, SQLITE_TRANSIENT) == SQLITE_OK;
        };
        const bool bound = job.running
            ? bind(1, job.id) && bind(2, job.actor) &&
              bind(3, job.backend) && bind(4, job.recordingId) &&
              bind(5, job.mode)
            : bind(1, job.report) && bind(2, job.id) &&
              bind(3, job.actor) && bind(4, job.backend);
        const bool ok = bound && sqlite3_step(statement) == SQLITE_DONE &&
            sqlite3_changes(database_.handle()) == 1;
        sqlite3_finalize(statement);
        return ok;
    }

    std::optional<Job> readCompleted(
        const std::string& actor, const std::string& backend,
        const std::string& checkId) const
    {
        auto lease = database_.acquireTransactionLease();
        sqlite3_stmt* statement = nullptr;
        const char* sql =
            "SELECT recording_id,mode,report FROM media_check_reports "
            "WHERE check_id=? AND actor_id=? AND backend_id=?"
            " AND state='completed' LIMIT 1;";
        if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement,
                               nullptr) != SQLITE_OK)
            return std::nullopt;
        const auto bind = [&](int index, const std::string& value) {
            return sqlite3_bind_text(statement, index, value.c_str(),
                                      -1, SQLITE_TRANSIENT) == SQLITE_OK;
        };
        std::optional<Job> found;
        if (bind(1, checkId) && bind(2, actor) && bind(3, backend) &&
            sqlite3_step(statement) == SQLITE_ROW) {
            const auto column = [&](int index) -> std::string {
                const unsigned char* value =
                    sqlite3_column_text(statement, index);
                return value
                    ? reinterpret_cast<const char*>(value) : "";
            };
            Job job;
            job.id = checkId;
            job.actor = actor;
            job.backend = backend;
            job.recordingId = column(0);
            job.mode = column(1);
            job.report = column(2);
            if (!job.recordingId.empty() && !job.report.empty())
                found = std::move(job);
        }
        sqlite3_finalize(statement);
        return found;
    }

    static std::string snapshotJson(const Job& job)
    {
        // Identifiers and mode are restricted to safe ASCII by the API gate
        // and the canonical PublicRecordingIdentityRepository.
        return "{\"checkId\":\"" + job.id + "\",\"backendId\":\"" +
            job.backend + "\",\"recordingId\":\"" + job.recordingId +
            "\",\"mode\":\"" + job.mode + "\",\"state\":\"" +
            (job.running ? "running" : "completed") + "\",\"report\":" +
            (job.running ? "null" : job.report) + "}";
    }
};
